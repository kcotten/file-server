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

#define SALT_LEN              2
#define BYTES              2048
#define BADREQUEST          400
#define UNAUTHORIZED        401
#define FORBIDDEN           403
#define NOTFOUND            404
#define PAYLOADTOOLARGE     413
#define INTERNALSERVERERROR 500
#define EXIT exit(3)
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
    
    EXIT;
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
    char* test2;
    char* test3;
    while( received < sizeOfPost) {
        bytes = read(sock, buffr, BYTES);
        //printf("Current contents of buffr are: %s\n", (char*)buffr);
        received += bytes;
        if(occurence == 0) {
            test = strstr((char*)buffr, "\r\n\r\n");
            if(test) {
                // case 1. first linebreak
                strncpy(buf, (char*)buffr, strlen((char*)buffr));
                size_t len = (test + 4) - (char*)buffr;
                strncpy(tbuf, (char*)buffr + len, strlen((char*)buffr) - len);
                //printf("First buffer: %s\n", tbuf);
                occurence++;
                tbuf[BYTES+1] = '\0';
                write(fd, tbuf, strlen(tbuf));
                memset(tbuf, 0, sizeof(tbuf));
                memset(buffr, 0, strlen((char*)buffr));
                continue;
            }
        } else {
            // case 2, second linebreak
            test2 = strstr((char*)buffr, "\n\r\n");
            if(test2) {
                strncpy(buf, (char*)buffr, strlen((char*)buffr));
                size_t len = (test2) - (char*)buffr;
                
                //printf("Length is: %zu\n",len);
                //printf("Test is: %d\n", (int)*test2);
                //printf("Current contents of buffr are: %s\n", (char*)buffr);
                strncpy(tbuf, (char*)buffr, len);
                tbuf[strlen((char*)buffr) - len + 1] = '\0';
                //printf("Second buffer: %s\n", tbuf);
                
                write(fd, tbuf, strlen(tbuf));
                break;
            }
            test3 = strstr((char*)buffr, "\r\n\r\n");
            if(test3) {
                strncpy(buf, (char*)buffr, strlen((char*)buffr));
                size_t len = (test3) - (char*)buffr;
                
                //printf("Length is: %zu\n",len);
                //printf("Test is: %d\n", (int)*test2);
                //printf("Current contents of buffr are: %s\n", (char*)buffr);
                strncpy(tbuf, (char*)buffr, len);
                tbuf[strlen((char*)buffr) - len + 1] = '\0';
                //printf("Second buffer: %s\n", tbuf);
                
                write(fd, tbuf, strlen(tbuf));
                break;
            }
        }
        
        write(fd, buffr, bytes);              
    }
    write(fd, (void*)"\n", 1);
    close(fd);
    snprintf(buffer, BYTES, "HTTP/1.1 200 OK\nServer: server.c\nContent-Length: %d\nConnection: close\n\n\n", 0);
    write(sock, buffer, strlen(buffer));
}


void getCwd(int sock, char path[BYTES]) {
    char localPath[BYTES];
    if (getcwd(localPath, sizeof(localPath)) == NULL) {
        error(INTERNALSERVERERROR, sock);
    }    
    strcat(localPath, path);
    strncpy(path, localPath, strlen(localPath));
    printf("Path in function: %s\n", path);
}


void authenticate(int sock, char* request) {    
    //printf("%s\n", request);
    /*
    char buffer[strlen(request) + 1];
    strncpy(buffer, request, strlen(request));
    buffer[sizeof(buffer)] = '\0';    
    */
    char* q[BYTES];
    FILE *in;
    char s[BYTES];
    char* u = NULL;
    char* p = NULL;
    char* t = NULL;
    char fp[BYTES];
    int i = 0;
    /*
    q[i] = strchr(request, '?');
    printf("Str: %s\n", q);
    */
    q[i] = strtok(request, " ?=&");    
    while( q[i] != NULL ) {
        //printf("Str: %s\n", q[i]);
        i++;
        q[i] = strtok(NULL, "=");
    }
    //printf("Out of loop...\n");
    //printf("q[2] == %s\n", q[2]);
    u = strtok(q[2], "&");
    //u[strlen(u) + 1] = 0;
    printf("User: %s\n", u);
    p = strtok(q[3], " ");
    printf("Password: %s\n", p);
    
    char* salt = strndup(u, SALT_LEN);
    char localPath[BYTES];
    if (getcwd(localPath, sizeof(localPath)) == NULL) {
        error(INTERNALSERVERERROR, sock);
    }
    char ustr[] = "/users";
    printf("File string: %s\n", ustr);
    t = strncat(localPath, ustr, BYTES);
    printf("Test\n");
    strncpy(fp, localPath, strlen(localPath));
    printf("Filepath: %s\n", fp);
    printf("User test: %s\n", u);
    
    /*
    if( access( fp, F_OK ) != -1 ) {
        // file exists
        printf("All ok\n");
    } else {
        // file doesn't exist
        printf("File DNE\n");
    }
    */
    
    if( (in = fopen(fp, "r"))== NULL ){
        error(INTERNALSERVERERROR, sock);
    }
    printf("The file is open...\n");
    while (fgets(s, BYTES, in) != NULL) {
        //printf("%s", s);
        puts(s);
    }
    printf("Exiting function!\n");
    /*
    printf("The query is: %s\n", q);
    s = strtok(request, "&");
    p = strtok(s, "=");
    u = strtok(request, "=");
    printf("User/Password are: %s / %s\n", u, p);
    */
    fclose(in);
    free(salt);
    EXIT;
}

/* Santize the input */
void parse(int sock, char* request, int type, char* buffer, int sizeOfPost) {
    //printf("Received: %s %d\n", request, sizeOfPost);
    char* pathStart = strchr(request, ' ') + 1;
    
    // check size error
    char* qryStart = strchr(pathStart, '?');
    if(qryStart) {
        char* qryEnd = strchr(qryStart, ' ');
        printf("Here");
        char path[qryStart - pathStart];
        char qry[qryEnd - qryStart];   
        strncpy(path, pathStart, qryStart - pathStart);
        strncpy(qry, qryStart + 1, qryEnd - qryStart);
        path[sizeof(path)] = 0;
        qry[sizeof(qry)] = 0;
        //authenticate(sock, path, qry);
        getCwd(sock, path);
        printf("The path is: %s\nand the query is: %s\n", path, qry);
        if( type == 0 ) {
        post(sock, path, qry, buffer, sizeOfPost);
        } else {
            get(sock, path, qry, buffer);
        }
    } else {
        char* pathEnd   = strchr(pathStart, '\0');
        char path[pathEnd - pathStart];
        strncpy(path, pathStart, pathEnd - pathStart);
        path[sizeof(path)] = '\0';
        char* qry = "";
        
        getCwd(sock, path);
        if( type == 0 ) {
        post(sock, path, qry, buffer, sizeOfPost);
        } else {
            get(sock, path, qry, buffer);
        }
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
    printf("\n%s\n", request);
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
        strncpy(buffer, request, BYTES);
        char* s = strtok(request, " ");
        if(strncmp("login", s, 5)) {
            authenticate(sock, buffer);
        }
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

