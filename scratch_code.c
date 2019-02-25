// scratch code

char* buf = malloc(sizeof(char));
char* body = malloc(sizeOfPost + 1);
//printf("The int I recieved in POST is: %d\n", sizeOfPost);
    
// read and discard until blankline
while( (discard = read(sock, buf, sizeof(buf)) > 0)) {
    if(*buf == '\r') {
        if(*buf == '\n') {
            if(*buf == '\r') {
                if(*buf == '\n') {
                    break;
                }
            }
        }
    }
}

while( (discard = read(sock, buffer, bytes)) > 0 ) {
    // do nothing
}


while( (bytes = read(sock, buffer, BYTES)) > 0 ) {
		location = strstr(buffer, "\r\n\r\n");
        if( location) {
            write(fd, location, strlen(buf));
        }
        



        write(fd, buffer, bytes);
        //recieved += bytes;        
	}
    
    
// deprecated recieved -> (recieved < sizeOfPost) && 
    // read and write to file the size located in sizeOfPost
    while( !found ) {
        if( (discard = read(sock, bufr, 1)) > 0) {
            if(*bufr == '\r') {
                if( (discard = read(sock, bufr, 1)) > 0 ) {
                    if(*bufr == '\n') {
                        if( (discard = read(sock, bufr, 1)) > 0 ) {
                            if(*bufr == '\r') {
                                if( (discard = read(sock, bufr, 1)) > 0 ) {
                                    if(*bufr == '\n') {
                                        found = 1;
                                        break;
                                    }
                                } else {
                                    error(BADREQUEST, sock);
                                }
                            }
                        } else {
                            error(BADREQUEST, sock);
                        }
                    }
                } else {
                    error(BADREQUEST, sock);
                }
            }
        } else {
            error(BADREQUEST, sock);
        }
	}