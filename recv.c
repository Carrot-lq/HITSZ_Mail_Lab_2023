#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

#define MAX_SIZE 65535

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

void recv_mail()
{
    const char* host_name = "pop.qq.com"; // TODO: Specify the mail server domain name
    const unsigned short port = 110; // POP3 server port
    const char* user = "FILL YOUR INFO@qq.com"; // TODO: Specify the user
    const char* pass = "FILL YOUR INFO"; // TODO: Specify the password
    char dest_ip[16];
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

    // TODO: Create a socket,return the file descriptor to s_fd, and establish a TCP connection to the POP3 server
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

    // TODO: Send user and password and print server response
    char USER[] = "USER ";
    strcat(USER, user);
    strcat(USER, "\r\n");
    send(s_fd, USER, strlen(USER), 0);
    printf("%s",USER);
    print_resp(s_fd, buf);

    char PASS[] = "PASS ";
    strcat(PASS, pass);
    strcat(PASS, "\r\n");
    send(s_fd, PASS, strlen(PASS), 0);
    printf("%s",PASS);
    print_resp(s_fd, buf);

    // TODO: Send STAT command and print server response
    char STAT[] = "STAT\r\n";
    send(s_fd, STAT, strlen(STAT), 0);
    printf("%s",STAT);
    print_resp(s_fd, buf);

    // TODO: Send LIST command and print server response
    char LIST[] = "LIST\r\n";
    send(s_fd, LIST, strlen(LIST), 0);
    printf("%s",LIST);
    print_resp(s_fd, buf);

    // TODO: Retrieve the first mail and print its content
    char RETR[] = "RETR 1\r\n";
    send(s_fd, RETR, strlen(RETR), 0);
    printf("%s",RETR);
    print_resp(s_fd, buf);

    // TODO: Send QUIT command and print server response
    char QUIT[] = "QUIT\r\n";
    send(s_fd, QUIT, strlen(QUIT), 0);
    printf("%s",QUIT);
    print_resp(s_fd, buf);

    close(s_fd);
}

int main(int argc, char* argv[])
{
    recv_mail();
    exit(0);
}
