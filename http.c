/**
 * Copyright (C) 2018 David C. Harrison - All Rights Reserved.
 * You may not use, distribute, or modify this code without
 * the express written permission of the copyright holder.
 */
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <netinet/in.h>
#include <string.h>
#include <ctype.h>
#include <crypt.h>
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
    //lseek(fd, 0, SEEK_END);
    //lseek(fd, 0, SEEK_SET);
    while (	(bytes = read(fd, buffer, BYTES)) > 0 ) {
		write(sock, buffer, bytes);
	}
    
    sleep(1);
}

/* POST handler */
void post(int sock, char* path, char* query, char* buffer, int sizeOfPost, char* boundary) {
    int bytes, fd;
    int received = 0; int i = 0;
    int headerDump = 0;
    void* buffr[BYTES];
    char tbuf[BYTES+1];
    char buf[BYTES];
    char* p = NULL;
    char* r[BYTES];
    
    if(( fd = open(path, O_CREAT | O_RDWR)) == -1) {
        //perror(buffer);
		error(INTERNALSERVERERROR, sock);        
	}
    char* test;
    char* bound;
    char* test3;
    while( received < sizeOfPost) {
        bytes = read(sock, buffr, BYTES);
        //printf("Current contents of buffr are: %s\n", (char*)buffr);        
        received += bytes;
        printf("Bytes read: %d\n", received);
        if(headerDump == 0) {
            //strncpy(buf, (char*)buffr, strlen((char*)buffr));
            //test = strchr(buf, '-');
            headerDump++;
        } else {
            bound = strstr((char*)buffr, boundary);
            if(bound) {
                printf("Boundary found. Dumping remainder!\n");
                break;
            }
            write(fd, buffr, bytes);
            memset(buffr, 0, BYTES);
        }
    }
    //write(fd, (void*)"\n", 1);
    close(fd);
    snprintf(buffer, BYTES, "HTTP/1.1 200 OK\nServer: server.c\nContent-Length: %d\nConnection: close\r\n\r\n", 0);
    write(sock, buffer, strlen(buffer));
}


void getPath(int sock, char* path) {
    char localPath[BYTES];
    if (getcwd(localPath, sizeof(localPath)) == NULL) {
        error(INTERNALSERVERERROR, sock);
    }    
    //strcat(localPath, path);
    strncpy(path, localPath, strlen(localPath));
    //printf("Path in function: %s\n", path);
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

void authenticate(int sock, char* request, char* buffer) {    
    char* q[BYTES];
    FILE *in;
    char s[BYTES];
    char* u = NULL;
    char* p = NULL;
    //char* t = NULL;
    char fp[BYTES];
    int i = 0;
    q[i] = strtok(request, " ?=&");    
    while( q[i] != NULL ) {
        i++;
        q[i] = strtok(NULL, "=");
    }
    u = strtok(q[2], "&");
    //printf("User: %s\n", u);
    p = strtok(q[3], " ");
    //printf("Password: %s\n", p);

    char localPath[BYTES];
    if (getcwd(localPath, sizeof(localPath)) == NULL) {
        error(INTERNALSERVERERROR, sock);
    }
    char ustr[] = "/users";    
    //printf("File string: %s\n", ustr);
    strncat(localPath, ustr, BYTES);
    //printf("Test\n");
    strncpy(fp, localPath, strlen(localPath));
    //printf("Filepath: %s\n", fp);
    //printf("User test: %s\n", u);
    
    if( (in = fopen(fp, "r"))== NULL ){
        error(INTERNALSERVERERROR, sock);
    }
    //printf("The file is open...\n");
    while (fgets(s, BYTES, in) != NULL) {
        char* tu = strtok(s, ":");
        char* tp = strtok(NULL, "\n");
        if(strcmp(p, tp) == 0) {
            // generate token
            puts("Generating Token\n");
            char* tsalt = strndup(tu, SALT_LEN);
            char* thash = crypt(tp, tsalt);
            puts(thash);
            free(tsalt);
            memset(localPath, 0, BYTES);
            getPath(sock, localPath);
            strcat(localPath, "/\0");
            strcat(localPath, u);
            dir(sock, localPath);
            snprintf(buffer, BYTES, "HTTP/1.1 200 OK\nServer: server.c\nSet-Cookie: name = %s\nConnection: close\n\n\n", thash);
            write(sock, buffer, strlen(buffer));
            EXIT;
        }
    }
    
    //printf("Exiting function!\n");
    fclose(in);
    error(UNAUTHORIZED, sock);
}

/* Santize the input and extract variables*/
void parse(int sock, char* request, int type, char* buffer) {
    //printf("Received: %s\n", request);
    FILE *in;
    int sizeOfPost = 0; int i = 0;
    char* rq[BYTES];
    char buf[BYTES];
    char fp[BYTES];    
    char* cookie = NULL;
    char s[BYTES];
    char localPath[BYTES];
    char ustr[] = "/users";
    
    strncpy(buf, request, BYTES);
    //puts(request);
    rq[i] = strtok(buf, " =&\r\n:");
    while( rq[i] != NULL ) {        
        //printf("%dth string is: %s\n", i, rq[i]);
        i++;
        rq[i] = strtok(NULL, " =&\r\n:");
    }
    int j = 0;
    while(strncmp("Set-Cookie", rq[j], 10) != 0) {
        j++;
    }
    printf("%s", rq[j+1]);
    cookie = rq[j+1];
    j = 0;
    char* path = rq[1];
    char* userStart = strchr(rq[1], '/') + 1;
    char* userEnd   = strchr(userStart, '/');
    char user[userEnd - userStart];
    strncpy(user, userStart, userEnd - userStart);
    user[sizeof(user)] = '\0';
    printf("User: %s\n", user);        
    
    if (getcwd(localPath, sizeof(localPath)) == NULL) {
        error(INTERNALSERVERERROR, sock);
    }
    
    strncat(localPath, ustr, BYTES);
    printf("File string: %s\n", localPath);
    //printf("Test\n");
    strncpy(fp, localPath, strlen(localPath));
    if( (in = fopen(fp, "r"))== NULL ){
        error(INTERNALSERVERERROR, sock);
    }
    printf("The file is open...\n");
    while (fgets(s, BYTES, in) != NULL) {
        char* tu = strtok(s, ":");
        char* tp = strtok(NULL, "\n");
        if(strcmp(user, tu) == 0) {
            // generate token and then check agianst provided one
            puts("Generating Token\n");
            char* tsalt = strndup(tu, SALT_LEN);
            char* thash = crypt(tp, tsalt);
            puts(thash);
            free(tsalt);
            if(strcmp(thash, cookie) == 0) {
                printf("Authorized user...\n");
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
    char* qry = NULL; // later functionality?
    // one last thing get the boundary!!!!
    
    if(type == 0) {
        while(strncmp("Content-Length", rq[j], 10) != 0) {
            j++;
        }
        //sizeOfPost = (int)rq[j+1];
        sizeOfPost = (int)strtol(rq[j+1], &rq[j+1], 10);
        j = 0;
        printf("%d\n", sizeOfPost);
        char* boundary = rq[i-1];
        printf("The boundary is: %s\n", boundary);
        post(sock, localPath, qry, buffer, sizeOfPost, boundary);
    } else {
        printf("Heading to GET...\n");
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
    int type; //sizeOfPost;
    static char buffer[BYTES];
    static char newRequest[BYTES];
    //printf("\n%s\n", request);
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
        //puts(s);
        if(strncmp("/login", s, 6) == 0) {
            authenticate(sock, buffer, newRequest);
        }        
    } else {
        type = 1; // get        
    }
    //printf("About to copy the request...\n");
    strncpy(newRequest, buffer, BYTES);
    /*
    for(int i=0; i < strlen(newRequest);i++) {
	    if(newRequest[i] == '\r' || newRequest[i] == '\n')
	        newRequest[i]='*';    
    }
    for(int i=5; i < strlen(newRequest);i++) {
	    if(newRequest[i] == ' ')
	        newRequest[i]='\0';    
    }
    */
    //printf("Request being sent: %s\n", newRequest);
    //memset(buffer, 0, BYTES);
    parse(sock, newRequest, type, buffer);
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

