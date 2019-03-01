#include "Socket.h"
#include "http.h"
#include <arpa/inet.h>
#include <ctype.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define MAXLINE 1024
#define SERV_PORT 8000
#define MAX(a, b) ((a) >= (b) ? (a) : (b))

int main(int argc, char* argv[])
{
    struct sockaddr_in servaddr, cliaddr;
    socklen_t cliaddr_len;
    int listenfd, connfd, sockfd;
    char buf[MAXLINE];
    char str[INET_ADDRSTRLEN];
    int i, n;

    int client[FD_SETSIZE];
    fd_set rset, allset; // fd set for select
    int maxfd, maxi = -1, nready;

    // IPv4, TCP
    listenfd = Socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET; // IPv4
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); // listen address
    servaddr.sin_port = htons(SERV_PORT); // listen port

    Bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
    Listen(listenfd, 5);

    for (i = 0; i < FD_SETSIZE; i++)
        client[i] = -1;
    FD_ZERO(&allset);
    FD_SET(listenfd, &allset); // add listen to select set
    maxfd = listenfd;

    printf("Accepting connections ...\n");
    while (1) {
        rset = allset;
        nready = select(maxfd + 1, &rset, NULL, NULL, NULL);
        if (nready < 0)
            perr_exit("select error.\n");
        if (FD_ISSET(listenfd, &rset)) { // new connection
            cliaddr_len = sizeof(cliaddr);
            connfd = Accept(listenfd, (struct sockaddr*)&cliaddr, &cliaddr_len);
            for (i = 0; i < FD_SETSIZE; i++)
                if (client[i] < 0) {
                    client[i] = connfd; // save descriptor
                    break;
                }
            if (i == FD_SETSIZE) {
                // too many clinets, send 503 and close.
                start_http_response(connfd, 503);
                Close(connfd);
            } else {
                FD_SET(connfd, &allset); // add new descriptor to set
                maxfd = MAX(maxfd, connfd);
                maxi = MAX(maxi, i);
                if (--nready == 0)
                    continue; // no more readable descriptors
            }
        }
        // process client.
        for (i = 0; i <= maxi && nready > 0; i++) {
            if ((sockfd = client[i]) < 0)
                continue;
            if (FD_ISSET(sockfd, &rset)) {
                n = Read(sockfd, buf, MAXLINE);
                // Write(1, buf, n);  // print to console.
                if (n != 0) {
                    serve_http_request(sockfd, buf);
                }
                // disconnect.
                Close(sockfd);
                FD_CLR(sockfd, &allset);
                client[i] = -1;
                nready--;
            }
        }
    }
    return 0;
}