#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <getopt.h>
#include "base64_utils.h"

#define MAX_SIZE 4095

char buf[MAX_SIZE+1];

void print_resp(int m_fd, char* m_buf){
    int recv_size;
    if ((recv_size = recv(m_fd, m_buf, MAX_SIZE, 0)) == -1) {
        perror("recv");
        exit(EXIT_FAILURE);
    }
    m_buf[recv_size] = '\0';
    printf("%s", m_buf);
}

char* get_str_of_encoded_file(const char* file_path) {
    FILE* src = fopen(file_path, "rb");
    if (src == NULL) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }
    FILE* dst = fopen("tmp", "wb+");
    if (dst == NULL) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }
    encode_file(src, dst);
    fclose(src);
    fseek(dst, 0, SEEK_END);
    int flen = ftell(dst);
    rewind(dst);
    char* result = (char*) malloc((flen + 1) * sizeof(char));
    fread(result, 1, flen, dst);
    result[flen] = '\0';
    fclose(dst);
    return result;
}

// receiver: mail address of the recipient
// subject: mail subject
// msg: content of mail body or path to the file containing mail body
// att_path: path to the attachment
void send_mail(const char* receiver, const char* subject, const char* msg, const char* att_path)
{
    const char* end_msg = "\r\n.\r\n";
    const char* host_name = "smtp.qq.com"; // TODO: Specify the mail server domain name
    const unsigned short port = 25; // SMTP server port
    const char* user = "FILL YOUR INFO@qq.com"; // TODO: Specify the user
    const char* pass = "FILL YOUR INFO"; // TODO: Specify the password
    const char* from = "FILL YOUR INFO@qq.com"; // TODO: Specify the mail address of the sender
    char dest_ip[16]; // Mail server IP address
    int s_fd; // socket file descriptor
    struct hostent *host;
    struct in_addr **addr_list;
    int i = 0;
    int r_size;

    // Get IP from domain name
    if ((host = gethostbyname(host_name)) == NULL)
    {
        herror("gethostbyname");
        exit(EXIT_FAILURE);
    }

    addr_list = (struct in_addr **) host->h_addr_list;
    while (addr_list[i] != NULL)
        ++i;
    strcpy(dest_ip, inet_ntoa(*addr_list[i-1]));

    // TODO: Create a socket, return the file descriptor to s_fd, and establish a TCP connection to the mail server
    if ((s_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    // addr
    struct sockaddr_in *addr = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));
    addr->sin_addr.s_addr = inet_addr(dest_ip);
    addr->sin_port = htons(port);
    addr->sin_family = AF_INET;
    // connect
    if ((connect(s_fd, (struct sockaddr *)addr, sizeof(struct sockaddr_in))) == -1) {
        perror("connect");
        exit(EXIT_FAILURE);
    }

    // Print welcome message
    if ((r_size = recv(s_fd, buf, MAX_SIZE, 0)) == -1)
    {
        perror("recv");
        exit(EXIT_FAILURE);
    }
    buf[r_size] = '\0'; // Do not forget the null terminator
    printf("%s", buf);

    // Send EHLO command and print server response
    char EHLO[] = "EHLO "; // TODO: Enter EHLO command here
    strcat(EHLO, user);
    strcat(EHLO, "\r\n");
    send(s_fd, EHLO, strlen(EHLO), 0);
    // TODO: Print server response to EHLO command
    printf("%s",EHLO);
    print_resp(s_fd, buf);
    // TODO: Authentication. Server response should be printed out.
    char AUTH[] = "AUTH login\r\n";
    send(s_fd, AUTH, strlen(AUTH), 0);
    printf("%s",AUTH);
    print_resp(s_fd, buf);
    // encoded user
    char* en_user = encode_str(user);
    strcat(en_user, "\r\n");
    send(s_fd, en_user, strlen(en_user), 0);
    printf("%s",en_user);
    print_resp(s_fd, buf);
    free(en_user);

    char* en_pass = encode_str(pass);
    strcat(en_pass, "\r\n");
    send(s_fd, en_pass, strlen(en_pass), 0);
    printf("%s",en_pass);
    print_resp(s_fd, buf);
    free(en_pass);

    // TODO: Send MAIL FROM command and print server response
    char MAIL[] = "MAIL FROM:<";
    strcat(MAIL, from);
    strcat(MAIL, ">\r\n");
    send(s_fd, MAIL, strlen(MAIL), 0);
    printf("%s",MAIL);
    print_resp(s_fd, buf);

    // TODO: Send RCPT TO command and print server response
    char RCPT[] = "RCPT TO:<";
    strcat(RCPT, receiver);
    strcat(RCPT, ">\r\n");
    send(s_fd, RCPT, strlen(RCPT), 0);
    printf("%s",RCPT);
    print_resp(s_fd, buf);

    // TODO: Send DATA command and print server response
    char DATA[] = "DATA\r\n";
    send(s_fd, DATA, strlen(DATA), 0);
    printf("%s",DATA);
    print_resp(s_fd, buf);

    // TODO: Send message data
    char MESSAGE[] = "From: ";
    strcat(MESSAGE,from);
    strcat(MESSAGE,"\r\nTo: ");
    strcat(MESSAGE,receiver);
    strcat(MESSAGE,"\r\nMIME-Version: 1.0\r\n");
    strcat(MESSAGE,"Content-Type: multipart/mixed; boundary=qwertyuiopasdfghjklzxcvbnm\r\n");
    send(s_fd, MESSAGE, strlen(MESSAGE), 0);
    printf("%s",MESSAGE);
    if (subject != NULL) {
        char SUBJECT[] = "Subject: ";
        strcat(SUBJECT, subject);
        strcat(SUBJECT, "\r\n\r\n");
        send(s_fd, SUBJECT, strlen(SUBJECT), 0);
        printf("%s",SUBJECT);
    }
    if (msg != NULL) {
        char BOUNDARY[] = "--qwertyuiopasdfghjklzxcvbnm\r\n";
        strcat(BOUNDARY,"Content-Type:text/plain\r\n");
        strcat(BOUNDARY,"Content-Transfer-Encoding: base64\r\n\r\n");
        send(s_fd, BOUNDARY, strlen(BOUNDARY), 0);
        printf("%s",BOUNDARY);
        if(access(msg, F_OK) == 0){
            // path to the file containing mail body
            char* content = get_str_of_encoded_file(msg);
            strcat(content,"\r\n\r\n");
            send(s_fd, content, strlen(content), 0);
            printf("%s",content);
            free(content);
        } else {
            // content of mail body
            send(s_fd, msg, strlen(msg), 0);
            send(s_fd, "\r\n\r\n", strlen("\r\n\r\n"), 0);
        }
    }
    if (att_path != NULL) {
        char BOUNDARY[] = "--qwertyuiopasdfghjklzxcvbnm\r\n";
        strcat(BOUNDARY,"Content-Type:application/octet-stream\r\n");
        strcat(BOUNDARY,"Content-Transfer-Encoding: base64\r\n");
        strcat(BOUNDARY,"Content-Disposition: attachment; name=\"");
        strcat(BOUNDARY,att_path);
        strcat(BOUNDARY,"\"\r\n\r\n");
        send(s_fd, BOUNDARY, strlen(BOUNDARY), 0);
        printf("%s",BOUNDARY);
        char* content = get_str_of_encoded_file(att_path);
        strcat(content,"\r\n\r\n");
        send(s_fd, content, strlen(content), 0);
        printf("%s",content);
        free(content);
    }
    char BOUNDARY[] = "--qwertyuiopasdfghjklzxcvbnm\r\n";
    send(s_fd, BOUNDARY, strlen(BOUNDARY), 0);
    printf("%s",BOUNDARY);
    // TODO: Message ends with a single period
    send(s_fd, end_msg, strlen(end_msg), 0);
    printf("%s",end_msg);
    print_resp(s_fd, buf);

    // TODO: Send QUIT command and print server response
    char QUIT[] = "QUIT\r\n";
    send(s_fd, QUIT, strlen(QUIT), 0);
    printf("%s",QUIT);
    print_resp(s_fd, buf);

    printf("close fd\n");
    close(s_fd);
}

int main(int argc, char* argv[])
{
    int opt;
    char* s_arg = NULL;
    char* m_arg = NULL;
    char* a_arg = NULL;
    char* recipient = NULL;
    const char* optstring = ":s:m:a:";
    while ((opt = getopt(argc, argv, optstring)) != -1)
    {
        switch (opt)
        {
        case 's':
            s_arg = optarg;
            break;
        case 'm':
            m_arg = optarg;
            break;
        case 'a':
            a_arg = optarg;
            break;
        case ':':
            fprintf(stderr, "Option %c needs an argument.\n", optopt);
            exit(EXIT_FAILURE);
        case '?':
            fprintf(stderr, "Unknown option: %c.\n", optopt);
            exit(EXIT_FAILURE);
        default:
            fprintf(stderr, "Unknown error.\n");
            exit(EXIT_FAILURE);
        }
    }

    if (optind == argc)
    {
        fprintf(stderr, "Recipient not specified.\n");
        exit(EXIT_FAILURE);
    }
    else if (optind < argc - 1)
    {
        fprintf(stderr, "Too many arguments.\n");
        exit(EXIT_FAILURE);
    }
    else
    {
        recipient = argv[optind];
        send_mail(recipient, s_arg, m_arg, a_arg);
        exit(0);
    }
}
