#include "http.h"
#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>

#define MAXLINE 1024

ssize_t start_http_response(int sockfd, int code)
{
    char buf[MAXLINE], buf2[MAXLINE];
    FILE* awkf;
    int i;

    snprintf(buf, MAXLINE,
        "awk -F: '$1==\"%d\"{print $2}' status-code",
        code);
    awkf = popen(buf, "r");
    fgets(buf, MAXLINE, awkf);
    pclose(awkf);
    snprintf(buf2, MAXLINE, "HTTP/1.1 %d %s", code, buf);
    return Write(sockfd, buf2, strlen(buf2));
}

ssize_t send_http_header(int sockfd, char* key, char* value)
{
    char buf[MAXLINE];
    snprintf(buf, MAXLINE, "%s: %s\r\n", key, value);
    return Write(sockfd, buf, strlen(buf));
}

ssize_t end_http_header(int sockfd)
{
    return Write(sockfd, "\r\n", 2);
}

ssize_t send_error(int clientfd, int code, char* text)
{
    char buf[MAXLINE];
    snprintf(buf, MAXLINE, "<html><body><h1>%s</h1><a href='/'>Index</a></body></html>\r\n", text);
    start_http_response(clientfd, code);
    send_http_header(clientfd, "Content-Type", "text/html");
    end_http_header(clientfd);
    WriteStr(clientfd, buf);
    return 0;
}

ssize_t send_directory_view(int clientfd, char* dir)
{
    char parent[MAXLINE];
    struct dirent* dp;
    DIR* dfd;
    int last_slash = 0;
    int i = 0;

    if ((dfd = opendir(dir)) == NULL) {
        printf("%s [403]\n", dir);
        perror(dir);
        return -1;
    }

    while (dir[i]) {
        if (dir[i] == '/' && dir[i + 1] != '\0')
            last_slash = i;
        parent[i] = dir[i];
        i++;
    }
    if (last_slash)
        parent[last_slash + 1] = '\0';
    else
        parent[i] = '\0';

    printf("%s [200]\n", dir);
    start_http_response(clientfd, 200);
    send_http_header(clientfd, "Content-Type", "text/html");
    end_http_header(clientfd);
    WriteStr(clientfd, "<html><meta http-equiv=\"Content-Type\" content=\"text/html;charset=utf-8\" /><body><h1>");
    WriteStr(clientfd, dir + 1);
    WriteStr(clientfd, "</h1><br>");
    WriteStr(clientfd, "<table><tr><td><a href='");
    WriteStr(clientfd, ((char*)parent) + 1);
    WriteStr(clientfd, "'>parent</a></td></tr>");

    while ((dp = readdir(dfd)) != NULL) {
        if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0)
            continue; /* skip self and parent */

        WriteStr(clientfd, "<tr><td><a href='");
        WriteStr(clientfd, dir + 1);
        WriteStr(clientfd, dp->d_name);
        WriteStr(clientfd, "'>");
        WriteStr(clientfd, dp->d_name);
        WriteStr(clientfd, "</a></td></tr>");
    }
    WriteStr(clientfd, "</table></body></html>");
    closedir(dfd);
}

ssize_t serve_http_request(int clientfd, char* r)
{
    char path[1024], buf[MAXLINE];
    char hex[3] = { 0 };
    struct stat stbuf;
    int fd, n, i;
    FILE* awkf;

    if (strncmp(r, "GET", 3) != 0)
        return send_error(clientfd, 403, "Forbidden");
    r += 4; // point to path
    path[0] = '.';
    i = 1;
    // get path form path and with escape characters.
    while (*r != ' ' && *r != '\r' && *r != '\n') {
        if (*r == '%') {
            strncpy(hex, r + 1, 2);
            path[i] = (char)strtol(r, NULL, 16);
            r += 3;
        } else {
            path[i] = *r;
            r++;
        }
        i++;
    }
    path[i] = '\0';

    if (stat(path, &stbuf) == -1) {
        printf("%s [404]\n", path);
        perror(path);
        return send_error(clientfd, 404, "404 Not Found.");
    }

    if (S_ISREG(stbuf.st_mode)) {
        // request a regular file
        if ((fd = open(path, O_RDONLY)) == -1) {
            printf("%s [403]\n", path);
            perror(path);
            return send_error(clientfd, 403, "Access Forbidden."); // File exists but can't open.
        }
        printf("%s %d [200]\n", path, stbuf.st_size);
        start_http_response(clientfd, 200);

        // get type
        // awk '$1==".mp412"{print $2;a+=1;} END{if(a==0)print "text/plain"}' content-type
        n = i;
        while (n >= 0 && path[n] != '.')
            n--;
        snprintf(buf, MAXLINE,
            "awk '$1==\"%s\"{print $2;a+=1;} END{if(a==0)print \" text/plain\"}' content-type",
            path + n);
        awkf = popen(buf, "r");
        fgets(buf, MAXLINE, awkf);
        i = 0;
        while (buf[i] != '\n')
            i++;
        buf[i] = '\0';
        send_http_header(clientfd, "Content-Type", buf);
        snprintf(buf, MAXLINE, "%d", stbuf.st_size);
        send_http_header(clientfd, "Content-Length", buf);
        pclose(awkf);
        end_http_header(clientfd);
        while ((n = read(fd, buf, MAXLINE)) > 0)
            Write(clientfd, buf, n);
        return 0;
    }

    if (S_ISDIR(stbuf.st_mode)) {
        // directory.
        if (path[i - 1] != '/') {
            path[i] = '/';
            path[i + 1] = '\0';
        }
        return send_directory_view(clientfd, path);
    }

    start_http_response(clientfd, 200);
    send_http_header(clientfd, "Content-Type", "text/html");
    end_http_header(clientfd);
    WriteStr(clientfd, "<html><body>OK</body></html>\r\n");
    return 0;
}