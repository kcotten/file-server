/**
 * Copyright (C) 2018 David C. Harrison - All Rights Reserved.
 * You may not use, distribute, or modify this code without
 * the express written permission of the copyright holder.
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

#define SALT_LEN              2
#define BYTES              2048
#define BADREQUEST          400
#define UNAUTHORIZED        401
#define FORBIDDEN           403
#define NOTFOUND            404
#define PAYLOADTOOLARGE     413
#define INTERNALSERVERERROR 500
#define EXIT exit(3)
#define BADREQUESTMSG          "HTTP/1.1 400: Bad Request\n"
#define UNAUTHORIZEDMSG        "HTTP/1.1 401: Access Unauthorized\n"
#define FORBIDDENMSG           "HTTP/1.1 403: Access Forbidden\n"
#define NOTFOUNDMSG            "HTTP/1.1 404: Requested resource not found\n"
#define PAYLOADTOOLARGEMSG     "HTTP/1.1 413: Payload is too large for the recipient buffer\n"
#define INTERNALSERVERERRORMSG "HTTP/1.1 500: Internal server error\n"

#define R0(v,w,x,y,z,i) z+=((w&(x^y))^y)+blk0(i)+0x5A827999+rol(v,5);w=rol(w,30);
#define R1(v,w,x,y,z,i) z+=((w&(x^y))^y)+blk(i)+0x5A827999+rol(v,5);w=rol(w,30);
#define R2(v,w,x,y,z,i) z+=(w^x^y)+blk(i)+0x6ED9EBA1+rol(v,5);w=rol(w,30);
#define R3(v,w,x,y,z,i) z+=(((w|x)&y)|(w&x))+blk(i)+0x8F1BBCDC+rol(v,5);w=rol(w,30);
#define R4(v,w,x,y,z,i) z+=(w^x^y)+blk(i)+0xCA62C1D6+rol(v,5);w=rol(w,30);

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
    
    if(( fd = open(path, O_CREAT | O_RDWR)) == -1) {
		error(INTERNALSERVERERROR, sock);        
	}
    char* bound;
    while( received < sizeOfPost) {
        bytes = read(sock, buffr, BYTES);
        received += bytes;
        printf("Bytes read: %d\n", received);
        if(headerDump == 0) {
            //strncpy(buf, (char*)buffr, strlen((char*)buffr));
            //test = strchr(buf, '-');
            headerDump++;
        } else {
            bound = strstr((char*)buffr, boundary);
            if(bound) {
                break;
            }
            write(fd, buffr, bytes);
            memset(buffr, 0, BYTES);
        }
    }
    close(fd);
    snprintf(buffer, BYTES, "HTTP/1.1 200 OK\nServer: server.c\nContent-Length: %d\nConnection: close\r\n\r\n", 0);
    write(sock, buffer, strlen(buffer));
}


void getPath(int sock, char* path) {
    char localPath[BYTES];
    if (getcwd(localPath, sizeof(localPath)) == NULL) {
        error(INTERNALSERVERERROR, sock);
    }
    strncpy(path, localPath, strlen(localPath));
}


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


char* hash_function (char* s) {
    int m = 16;
    int l = 0;
    size_t size = strlen(s) - 1;
    char* seed = "\a\a\n !)(./.\n\n,+  ";
    static char t[16];
    for(int i = 0; i <= m; i++) {
        //printf("seed is: %c and password is: %c  :  ", seed[i], s[l]);
        char temp = seed[i] ^ s[l];
        if(!isprint(temp)) {
            temp = '1';
        }
        //  ?=&
        if( temp == ' '|| temp == '?' || temp == '=' || temp == '&') {
            temp = '2';
        }
        t[i] = temp;
        //printf("s[] is now: %c\n", t[i]);
        if(l == size) {
            l = 0;
            continue;
        }            
        l++;
    }
    t[m] = '\0';
    return t;
}


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
    //printf("The file is open...\n");
    size_t usize, psize;
    usize = psize = 0;
    while (fgets(s, BYTES, in) != NULL) {
        char* tu = strtok(s, ":");
        char* tp = strtok(NULL, "\n");
        usize = strlen(tu);
        psize = strlen(tp);
        if(usersize == usize) {
            if(strncmp(u, tu, usize) == 0) {
                if(strncmp(p, tp, psize) == 0) {
                    // generate token
                    //char* tsalt = strndup(tu, SALT_LEN);
                    //unsigned long n = strlen(tp);
                    char* hash = NULL;
                    hash = hash_function(tp);
                    printf("The value of the hash is: %s\n", hash);
                    //SHA1((unsigned char*)tp, n, (unsigned char*)hash);
                    //free(tsalt);

                    memset(localPath, 0, BYTES);
                    getPath(sock, localPath);
                    strcat(localPath, "/\0");
                    strcat(localPath, u);
                    dir(sock, localPath);
                    snprintf(buffer, BYTES, "HTTP/1.1 200 OK\nServer: server.c\nSet-Cookie: %s\nConnection: close\r\n\r\n", hash);
                    write(sock, buffer, strlen(buffer));
                    EXIT;
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
    int sizeOfPost = 0; int i = 0; int j = 0;
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
    while(strncmp("Set-Cookie", rq[j], 10) != 0) {
        j++;
    }
    cookie = rq[j+1];
    j = 0;
    if(cookie == NULL) {
        while(strncmp("Cookie", rq[j], 10) != 0) {
            j++;
        }
        cookie = rq[j+1];
        j = 0;
        if(cookie == NULL) {
            while(strncmp("cookie", rq[j], 10) != 0) {
                j++;
            }
            cookie = rq[j+1];
            j = 0;
            if(cookie == NULL) {
                //write(sock,"Must send token in key:value pair Set-Cookie, Cookie, or cookie\n", 64);
                error(BADREQUEST, sock);
            }
        }
    }
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
            // generate token and then check against provided one
            /*
            char* tsalt = strndup(tu, SALT_LEN);
            struct crypt_data *data = NULL;
            char* thash = crypt_r(tp, tsalt, data);
            free(tsalt);
            */
            char* hash = hash_function(tp);
            printf("The hash is: %s\n", hash);
            if(strncmp(hash, cookie, strlen(cookie)) == 0) {
                break;
            } else
                error(UNAUTHORIZED, sock);
        }
    }
    fclose(in);
    // we are authorized now submit a get or post
    memset(localPath, 0, BYTES);
    if (getcwd(localPath, sizeof(localPath)) == NULL) {
            error(INTERNALSERVERERROR, sock);
    }
    strncat(localPath, path, strlen(path));
    char* qry = NULL;
    // one last thing get the boundary!!!!    
    if(type == 0) {
        while(strncmp("Content-Length", rq[j], 10) != 0) {
            j++;
        }
        sizeOfPost = (int)strtol(rq[j+1], &rq[j+1], 10);
        j = 0;
        while((strncmp("boundary", rq[j], 10) != 0)) {
            j++;
        }
        char* boundary = rq[j+1];
        if(boundary == NULL) {
            j = 0;
            while((strncmp("Boundary", rq[j], 10) != 0)) {
                j++;
            }
            boundary = rq[j+1];
            if(boundary == NULL) {
                write(sock, "Must send boundary key/value pair\n", 34);
                error(BADREQUEST, sock);
            }
        }
        post(sock, localPath, qry, buffer, sizeOfPost, boundary);
    } else {
        get(sock, localPath, qry, buffer);
    }
    EXIT;
}


char* cleanup(char* request) {
    char* clean = request;
    for(int i=0; i < strlen(clean);i++) {
	    if(clean[i] == '\r' || clean[i] == '\n')
	        clean[i]='*';    
    }
    for(int i=0; i < strlen(clean);i++) {
	    if(clean[i] == '*')
	        clean[i]='\0';    
    }
    return clean;
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

