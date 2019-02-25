/**
 * Copyright (C) 2018 David C. Harrison - All Rights Reserved.
 * You may not use, distribute, or modify this code without
 * the express written permission of the copyright holder.
 */
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <netinet/in.h>
#include <string.h>
#include<ctype.h>

#define BYTES              2048
#define BADREQUEST          400
#define UNAUTHORIZED        401
#define FORBIDDEN           403
#define NOTFOUND            404
#define PAYLOADTOOLARGE     413
#define INTERNALSERVERERROR 500
#define ABNORMALEXIT exit(3)
#define BADREQUESTMSG          "400: Bad Request\n"
#define UNAUTHORIZEDMSG        "401: Access Unauthorized\n"
#define FORBIDDENMSG           "403: Access Forbidden\n"
#define NOTFOUNDMSG            "404: Requested resource not found\n"
#define PAYLOADTOOLARGEMSG     "413: Payload is too large for the recipient buffer\n"
#define INTERNALSERVERERRORMSG "500: Internal server error\n"
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
    
    ABNORMALEXIT;
}

/* GET handler */
void get(int sock, char* path, char* query, char* buffer) {
    int bytes, fd;
    if(( fd = open(path, O_RDONLY)) == -1) {
		error(NOTFOUND, sock);
	}
    lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);
    while (	(bytes = read(fd, buffer, BYTES)) > 0 ) {
		write(sock, buffer, bytes);
	}
    
    sleep(1);
}

/* POST handler */
void post(int sock, char* path, char* query, char* buffer, int sizeOfPost) {
    int bytes, fd;
    int received = 0;
    int occurence = 0;
    void* buffr[BYTES];
    char tbuf[BYTES+1];
    char buf[BYTES];
    if(( fd = open(path, O_CREAT | O_RDWR)) == -1) {
        perror(buffer);
		error(INTERNALSERVERERROR, sock);        
	}
    char* test;
    while( received < sizeOfPost) {
        bytes = read(sock, buffr, BYTES);
        printf("Current contents of buffr are: %s\n", (char*)buffr);
        received += bytes;
        test = strstr((char*)buffr, "\r\n\r\n");
        if(test) {
            if(occurence == 0) { // case 1. first linebreak
                strncpy(buf, (char*)buffr, strlen((char*)buffr));
                size_t len = (test + 4) - (char*)buffr;
                strncpy(tbuf, (char*)buffr + len, strlen((char*)buffr) - len);
                printf("First buffer: %s\n", tbuf);
                occurence++;
                tbuf[BYTES+1] = '\0';
                write(fd, tbuf, strlen(tbuf));
                memset(tbuf, 0, sizeof(tbuf));
                memset(buffr, 0, strlen((char*)buffr));
                continue;
            } else { // case 2, second linebreak
                strncpy(buf, (char*)buffr, strlen((char*)buffr));
                size_t len = (test) - (char*)buffr;
                
                printf("Length is: %zu\n",len);
                printf("Test is: %d\n", (int)*test);
                printf("Current contents of buffr are: %s\n", (char*)buffr);
                strncpy(tbuf, (char*)buffr, len);
                tbuf[strlen((char*)buffr) - len + 1] = '\0';
                printf("Second buffer: %s\n", tbuf);
                
                write(fd, tbuf, strlen(tbuf));
                break;
            }
        }
        write(fd, buffr, bytes);              
    }
    //write(fd, (void*)"\n\0", 2);
    sleep(1);
    snprintf(buffer, BYTES, "HTTP/1.1 200 OK\nServer: server.c\nContent-Length: %d\nConnection: close\n\n\n", 0);
    write(sock, buffer, strlen(buffer));
}

/* Santize the input */
void parse(int sock, char* request, int type, char* buffer, int sizeOfPost) {
    //printf("Received: %s %d\n", request, sizeOfPost);
    char* pathStart = strchr(request, ' ') + 1;
    char* pathEnd   = strchr(pathStart, '\0');
    /*
    char* qryStart = strchr(pathStart, '?');
    char* qryEnd = strchr(qryStart, ' ');
    
    char path[qryStart - pathStart];
    char qry[qryEnd - qryStart];   
    strncpy(path, pathStart, qryStart - pathStart);
    strncpy(qry, qryStart + 1, qryEnd - qryStart);
    path[sizeof(path)] = 0;
    qry[sizeof(qry)] = 0;
    */
    char path[pathEnd - pathStart];
    strncpy(path, pathStart, pathEnd - pathStart);
    path[sizeof(path)] = '\0';
    /* Print query for testing
    char test[BYTES];
    sprintf(test, "%s\n", path);
    write(sock, test, strlen(test));
    sprintf(test, "%s\n", qry);
    write(sock, test, strlen(test));
    */
    char* qry = "";
    
    char localPath[BYTES];
    if (getcwd(localPath, sizeof(localPath)) == NULL) {
        error(INTERNALSERVERERROR, sock);
    }
    strcat(localPath, path);
    if( type == 0 ) {
        post(sock, localPath, qry, buffer, sizeOfPost);
    } else {
        get(sock, localPath, qry, buffer);
    }
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


int makeInt(char* request) {
    int t;
    while(*request) { // While there are more characters to process...
        if( isdigit(*request) ) {
            // Found a number
            t = (int)strtol(request, &request, 10); // Read number
            printf("%d\n", t); // and print it.
            break;
        } else {
            // Otherwise, move on to the next character.
            request++;
            printf("The string is currently: %s\n", request);
        }
    }
    return t;
}

/* 
   Recieve an http request string and socket from the server process and 
   determine the type of request
*/
void httpRequest(int sock, char *request) {
    size_t len = strlen(request);
    int type, sizeOfPost;
    static char buffer[BYTES];
    static char newRequest[BYTES];
    printf("%s\n", request);
    if( len  > BYTES ) {
        error(PAYLOADTOOLARGE, sock);
    }
    if( len <= 0 ) {
        error(BADREQUEST, sock);
    }
    if( strncmp(request,"GET ", 4) && strncmp(request,"get ", 4) && strncmp(request, "POST ", 5) && strncmp(request, "post ", 5) ) {
        error(BADREQUEST, sock);
    } else if(strncmp(request,"GET ", 4) && strncmp(request,"get ", 4)) {
        type = 0; // post
        
        char* getSize = request;
        char* charSize;
        for(int j = 0; j < 4;j++) {
            if( (getSize = strchr(getSize, '\n')) == 0 ) {
                break;
            }
            getSize++;
        }
        charSize = strchr(getSize, ':');
        charSize += 1;
        charSize = cleanup(charSize);
        charSize += 1;
        sizeOfPost = (int)strtol(charSize, &charSize, 10);
    } else {
        type = 1; // get        
    }
    
    strncpy(newRequest, request, BYTES);
    for(int i=0; i < strlen(newRequest);i++) {
	    if(newRequest[i] == '\r' || newRequest[i] == '\n')
	        newRequest[i]='*';    
    }
    for(int i=5; i < strlen(newRequest);i++) {
	    if(newRequest[i] == ' ')
	        newRequest[i]='\0';    
    }
    printf("Request with int: %s, %d\n", newRequest, sizeOfPost);   
    parse(sock, newRequest, type, buffer, sizeOfPost);
}

/*
void postProcess(char* path, int size) {
    
    int look = 0;
    char* buffer[size];
    if ((FILE in = fopen(path, O_RDWR)) == NULL ) {
        perror("Error opening file for post processing");
    }
    //fgets(buffer, size, in);
    while( !feof(in) ) {
        look = fgetc(in);
    }
    
    system(sed -i )
}
*/

