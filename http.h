#ifndef __HTTP_H_
#define __HTTP_H_

#include "Socket.h"
#include <ctype.h>

ssize_t start_http_response(int sockfd, int code);
ssize_t send_http_header(int sockfd, char* key, char* value);
ssize_t end_http_header(int sockfd);
ssize_t send_error(int clientfd, int code, char* text);
ssize_t send_directory_view(int clientfd, char* dir);
ssize_t serve_http_request(int clientfd, char* r);


#endif