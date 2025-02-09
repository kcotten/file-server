/**
 * Copyright (C) 2018 David C. Harrison - All Rights Reserved.
 * You may not use, distribute, or modify this code without
 * the express written permission of the copyright holder.
 */
 
 /* 
    Login - Be sure to save the token and then use Set-Cookie: 
    
    Use:
    
    curl -vX POST "localhost:<port>/login?username=user&password=pass"
    
    GET - Important to not forget the pipe at the end so the file is downloaded
    
    Use:
    
    curl -vb 'token=<Token>' "localhost:<port>/<user>/<file>" > <desired filename>
    
    POST - supports any transfer with 'Expected: 100-continue' in the header
        
    Use:
    
    curl -vb 'token=<Token>' -X POST "localhost:<port>/<user>/<file>" --data-binary "@<filename>"
        
    or
    
    curl -vb 'token=<Token>' -F 'file=@<file>' "localhost:<port>/<user>/<file>"
    
    or
    
    curl -vb 'token=<Token>' -X POST "localhost:<port>/<user>/<file>" --data-binary @<file>.txt --header 'Expect: 100-continue'
        *Will ensure delivery
    
 */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <netinet/in.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>

#define BYTES              2048
#define BADREQUEST          400
#define UNAUTHORIZED        401
#define FORBIDDEN           403
#define NOTFOUND            404
#define FORMAT              405
#define PAYLOADTOOLARGE     413
#define INTERNALSERVERERROR 500
#define EXIT exit(3)
#define BADREQUESTMSG          "HTTP/1.1 400: Bad Request\nContent-Length: 0\nConnection: close\r\n\r\n"
#define UNAUTHORIZEDMSG        "HTTP/1.1 401: Access Unauthorized\nContent-Length: 0\nConnection: close\r\n\r\n"
#define FORBIDDENMSG           "HTTP/1.1 403: Access Forbidden\nContent-Length: 0\nConnection: close\r\n\r\n"
#define NOTFOUNDMSG            "HTTP/1.1 404: Requested resource not found\nContent-Length: 0\nConnection: close\r\n\r\n"
#define PAYLOADTOOLARGEMSG     "HTTP/1.1 413: Payload is too large for the recipient buffer\nContent-Length: 0\nConnection: close\r\n\r\n"
#define INTERNALSERVERERRORMSG "HTTP/1.1 500: Internal server error\nContent-Length: 0\nConnection: close\r\n\r\n"
#define FORMATERRORMSG         "HTTP/1.1 400: Bad Request\nAttempt to transfer file without 'Expect: 100-continue'\nContent-Length: 0\nConnection: close\r\n\r\n"
/*
static void binary(int sock, char *fname) {
    int fd;
    int bytes;
    void *buffer[BYTES];
    if ((fd = open(fname, O_RDONLY)) != -1) {
        while ((bytes = read(fd, buffer, BYTES)) > 0)
            write(sock, buffer, bytes);
   }
}
*/

/* Error handler */
void error(int etype, int sock) {
    switch (etype) {
        case BADREQUEST:
            write(sock, BADREQUESTMSG, strlen(BADREQUESTMSG));
            break;
        case UNAUTHORIZED:
            write(sock, UNAUTHORIZEDMSG, strlen(UNAUTHORIZEDMSG));
            break;
        case FORBIDDEN:
            write(sock, FORBIDDENMSG, strlen(FORBIDDENMSG));
            break;
        case NOTFOUND:
            write(sock, NOTFOUNDMSG, strlen(NOTFOUNDMSG));
            break;
        case FORMAT:
            write(sock, FORMATERRORMSG, strlen(FORMATERRORMSG));
            break;
        case PAYLOADTOOLARGE:
            write(sock, PAYLOADTOOLARGEMSG, strlen(PAYLOADTOOLARGEMSG));
            break;
        default:
            write(sock, INTERNALSERVERERRORMSG, strlen(INTERNALSERVERERRORMSG));
            break;
    }
    
    EXIT;
}

/* GET handler */
void get(int sock, char* path, char* query, char* buffer) {
    int bytes, fd;
    if(( fd = open(path, O_RDONLY)) == -1) {
		error(NOTFOUND, sock);
	}
    while (	(bytes = read(fd, buffer, BYTES)) > 0 ) {
		write(sock, buffer, bytes);
	}    
    sleep(1);
}

/* POST handler */
void post(int sock, char* path, char* query, char* buffer, int sizeOfPost, char* boundary) {
    int bytes, fd;
    int received = 0;
    int headerDump = 0;
    void* buffr[BYTES];    
    if(( fd = open(path, O_CREAT | O_RDWR, 0666)) == -1) {
		error(INTERNALSERVERERROR, sock);        
	}
    char* bound;
    bytes = 1;
    while( (received < sizeOfPost) && (bytes > 0)) {
        bytes = read(sock, buffr, BYTES);
        received += bytes;
        // Is transmission form encoded? Dump/remove the header/footer
        if(headerDump == 0 && boundary != NULL) {
            headerDump++;
            continue;
        } else if ( boundary != NULL ) {
            bound = strstr((char*)buffr, boundary);
            if(bound) {
                bound -= 4;
                *bound = '\0';
                strncpy(buffer, (char*)buffr, strlen((char*)buffr));
                write(fd, buffr, strlen((char*)buffr));
                headerDump++;
                break;
            }           
        }
        write(fd, buffr, bytes);
        memset(buffr, 0, BYTES);
    }
    memset(buffer, 0, BYTES);
    close(fd);
    snprintf(buffer, BYTES, "HTTP/1.1 200 OK\nServer: Totally Secure HTTP Server\nContent-Length: %d\nConnection: close\r\n\r\n", 0);
    write(sock, buffer, strlen(buffer));
}

/* Return local path */
void getPath(int sock, char* path) {
    char localPath[BYTES];
    if (getcwd(localPath, sizeof(localPath)) == NULL) {
        error(INTERNALSERVERERROR, sock);
    }
    strncpy(path, localPath, strlen(localPath));
}

/* Create user directory during authentication if necessary */
void dir(int sock, char* path) {
    if( access( path, F_OK ) != -1 ) {
        // exists do nothing
        return;
    } else {
        // folder doesn't exist
        if( mkdir(path, 0777) == -1) {
            error(INTERNALSERVERERROR, sock);
        }
    }
}

/* Token generator */
char* tokenFetcher (char* s1, char* s2) {
    time_t date = time(NULL);
    struct tm dm = *localtime(&date);
    int d = dm.tm_mday;
    int m = 16;
    int l = 0;
    size_t size1 = strlen(s1) - 1;
    size_t size2 = strlen(s2) - 1;
    size_t slen  = size1;
    size_t size;
    if( size1 < size2 ) {
        size = size1;
    } else {
        size = size2;
    }
    char* seed = "\a\a\n !)(./.\n\n,+  ";
    static char t[16];
    char temp;
    for(int i = 0; i <= m; i++) {
        if( (i % 2) == 0 ) {
            temp = seed[i] ^ s2[l];
        } else {
            if((slen % 2) == 0) {
                temp = seed[i] ^ s1[slen];
                slen--;
                if(slen <= 0) slen = size1;
            } else {
                temp = seed[i] ^ s1[l];
            }
        }
        temp ^= d;
        if(!isalnum(temp)) {
            temp -= 50;
            while(!isalnum(temp))
                temp -= 1;
        }
        if(temp == 'z') {
            temp ^= i;
        }
        while(!isalnum(temp))
            temp ^= i;
        t[i] = temp;
        if(l == size) {
            l = 0;
            continue;
        }            
        l++;
    }
    t[m] = '\0';
    return t;
}

/* Login handler, return token */
void authenticate(int sock, char* request, char* buffer) {    
    char* q[BYTES];
    FILE *in;
    char s[BYTES];
    char localPath[BYTES];
    char* u = NULL;
    char* p = NULL;
    char fp[BYTES];
    int i = 0;
    size_t usersize, passsize;
    usersize = passsize = 0;
    char ustr[] = "/users";
    q[i] = strtok(request, " ?=&");    
    while( q[i] != NULL ) {
        i++;
        q[i] = strtok(NULL, "=");
    }
    u = strtok(q[2], "&");
    p = strtok(q[3], " ");
    usersize = strlen(u);
    passsize = strlen(p);
    if (getcwd(localPath, sizeof(localPath)) == NULL) {
        error(INTERNALSERVERERROR, sock);
    }    
    strncat(localPath, ustr, BYTES);
    strncpy(fp, localPath, strlen(localPath));    
    if( (in = fopen(fp, "r"))== NULL ){
        error(INTERNALSERVERERROR, sock);
    }
    size_t usize, psize;
    usize = psize = 0;
    while (fgets(s, BYTES, in) != NULL) {
        char* tu = strtok(s, ":");
        if(tu[0] == '\n' || tu[0] == '\0') break;
        char* tp = strtok(NULL, "\n");
        usize = strlen(tu);
        psize = strlen(tp);
        if(usersize == usize) {
            if(strncmp(u, tu, usize) == 0) {
                if(passsize == psize) {
                    if(strncmp(p, tp, psize) == 0) {
                        // generate token
                        char* hash = NULL;
                        hash = tokenFetcher(tu, tp);
                        memset(localPath, 0, BYTES);
                        getPath(sock, localPath);
                        strcat(localPath, "/\0");
                        strcat(localPath, u);
                        dir(sock, localPath);
                        snprintf(buffer, BYTES, "HTTP/1.1 200 OK\nServer: Totally Secure HTTP Server\nSet-Cookie: token=%s; Expires=Midnight\nContent-Length: 0\nConnection: close\r\n\r\n", hash);
                        write(sock, buffer, strlen(buffer));
                        EXIT;
                    }
                }
            }
        }
    }
    fclose(in);
    error(UNAUTHORIZED, sock);
}

/* Santize the input and extract variables*/
void parse(int sock, char* request, int type, char* buffer) {
    FILE *in;
    int sizeOfPost = 0; int i = 0; int j = 0; int found = 0;
    char* rq[BYTES];
    char buf[BYTES];
    char fp[BYTES];    
    char* cookie = NULL;
    char s[BYTES];
    char localPath[BYTES];
    char ustr[] = "/users";
    
    strncpy(buf, request, BYTES);
    rq[i] = strtok(buf, " =&\r\n:");
    while( rq[i] != NULL ) {
        i++;        
        rq[i] = strtok(NULL, " =&\r\n:");
        
    }
    while(strncmp("Expect", rq[j], 6) != 0) {
        j++;
        if(j == i) error(FORMAT, sock);
    }
    j = 0;
    while(strncmp("Cookie", rq[j], 10) != 0) {
        j++;
        if(j == i) error(BADREQUEST, sock);
    }
    cookie = rq[j+2];    
    j = 0;
    char* path = rq[1];
    char* userStart = strchr(rq[1], '/') + 1;
    char* userEnd   = strchr(userStart, '/');
    char user[userEnd - userStart];
    strncpy(user, userStart, userEnd - userStart);
    user[sizeof(user)] = '\0';
    if (getcwd(localPath, sizeof(localPath)) == NULL) {
        error(INTERNALSERVERERROR, sock);
    }    
    strncat(localPath, ustr, BYTES);
    strncpy(fp, localPath, strlen(localPath));
    if( (in = fopen(fp, "r"))== NULL ){
        error(INTERNALSERVERERROR, sock);
    }
    while (fgets(s, BYTES, in) != NULL) {
        char* tu = strtok(s, ":");
        char* tp = strtok(NULL, "\n");
        if(strncmp(user, tu, strlen(user)) == 0) {
            char* hash = tokenFetcher(tu, tp);
            if(strncmp(hash, cookie, strlen(cookie)) == 0) {
                found++;
                break;
            } else
                error(UNAUTHORIZED, sock);
        }
    }
    if(found == 0)
        error(UNAUTHORIZED, sock);
    fclose(in);
    // authorized now submit get or post
    memset(localPath, 0, BYTES);
    if (getcwd(localPath, sizeof(localPath)) == NULL) {
            error(INTERNALSERVERERROR, sock);
    }
    strncat(localPath, path, strlen(path));
    char* qry = NULL;
    char* boundary = NULL;
    if(type == 0) {
        while(strncmp("Content-Length", rq[j], 10) != 0) {
            j++;
        }
        sizeOfPost = (int)strtol(rq[j+1], &rq[j+1], 10);
        if( sizeOfPost == 0 )
            error(BADREQUEST, sock);
        j = 0;
        while(strncmp("Content-Type", rq[j], 10) != 0) {
            j++;
        }
        if(strncmp("multipart/form-data", rq[j+1], 19) == 0){
            j = 0;
            while((strncmp("boundary", rq[j], 10) != 0)) {
                j++;
            }
            boundary = rq[j+1];
            if(boundary == NULL) {
                j = 0;
                while((strncmp("Boundary", rq[j], 10) != 0)) {
                    j++;
                }
                boundary = rq[j+1];
                if(boundary == NULL) {
                    error(BADREQUEST, sock);
                }
            }            
        }
        post(sock, localPath, qry, buffer, sizeOfPost, boundary);
    } else {
        get(sock, localPath, qry, buffer);
    }
    EXIT;
}


/* 
   Recieve an http request string and socket from the server process and 
   determine the type of request
*/
void httpRequest(int sock, char *request) {
    size_t len = strlen(request);
    int type;
    static char buffer[BYTES];
    static char newRequest[BYTES];
    if( len  > BYTES ) {
        error(PAYLOADTOOLARGE, sock);
    }
    if( len <= 0 ) {
        error(BADREQUEST, sock);
    }
    size_t size = strlen(request);
    for( int i = 0; i < size; i++ ) {
		if(request[i] == '.' && request[i+1] == '.') {
            error(UNAUTHORIZED, sock);
        }
    }
    strncpy(buffer, request, BYTES);
    if( strncmp(request,"GET ", 4) && strncmp(request,"get ", 4) && strncmp(request, "POST ", 5) && strncmp(request, "post ", 5) ) {
        error(BADREQUEST, sock);
    } else if(strncmp(request,"GET ", 4) && strncmp(request,"get ", 4)) {
        type = 0; // post        
        char* s = strtok(request, " ");
        s = strtok(NULL, "?");
        if(strncmp("/login", s, 6) == 0) {
            authenticate(sock, buffer, newRequest);
        }        
    } else {
        type = 1; // get        
    }
    strncpy(newRequest, buffer, BYTES);
    parse(sock, newRequest, type, buffer);
}

