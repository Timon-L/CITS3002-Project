#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <netdb.h>

#define MYPORT "1299"
#define TXTLEN 128
#define LISTEN_BACKLOG 20
#define FILENAME "filename"
#define TEMPLATE "tmp/mt-XXXXXX"

int writeToFile(char *filename, char *msg){
    FILE *fp = fopen(filename,"w");

    if(!fp){
        fprintf(stderr, "Fail to open file '%s'\n", strerror(errno));
        return EXIT_FAILURE;
    }

    if(fputs(msg, fp) == EOF){
        fprintf(stderr,"Error:%s\n", strerror(errno));
        return EXIT_FAILURE;
    }
    fclose(fp);
    return EXIT_SUCCESS;
}

int make_tmp(){
    char *tmp;
    tmp = malloc(sizeof(TEMPLATE));
    strcpy(tmp, TEMPLATE);
    if(mkdtemp(tmp) == NULL){
       fprintf(stderr, "Error:%s\n", strerror(errno));
       return EXIT_FAILURE; 
    }
    printf("%s", tmp);
    return EXIT_SUCCESS;
}

void hostname(){
    char hostname[TXTLEN];
    hostname[TXTLEN-1] = '\0';
    if(gethostname(hostname, TXTLEN) == -1){
        fprintf(stderr, "gethostname:%s\n", strerror(errno));
    }
    printf("My name is:%s\n", hostname);
}

int main(int argc, char *argv[]){
    int sockfd, clientfd;
    struct addrinfo *servinfo, *copy, my_addr;
    struct sockaddr_storage peer_addr;
    socklen_t peer_address_size;
    char msg[TXTLEN];
    
    int bytes;
    int reuse_addr = 1;

    memset(&my_addr, 0, sizeof(my_addr));
    my_addr.ai_family = AF_UNSPEC;
    my_addr.ai_socktype = SOCK_STREAM;
    my_addr.ai_flags = AI_PASSIVE;

    if(getaddrinfo(NULL, MYPORT, &my_addr, &servinfo) == -1){
        fprintf(stderr,"getaddrinfo:%s\n", strerror(errno));
    }

    for(copy = servinfo; copy != NULL; copy = copy -> ai_next){
            if((sockfd = socket(copy->ai_family, copy->ai_socktype, copy->ai_protocol)) == -1){
                fprintf(stderr, "sockfd:%s\n", strerror(errno));
                continue; 
            }

            if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse_addr, sizeof(int)) == -1){
                fprintf(stderr, "setsockopt:%s\n", strerror(errno));
                return EXIT_FAILURE;
            }

            if(bind(sockfd, copy->ai_addr, copy->ai_addrlen) == -1){
                fprintf(stderr, "bind:%s\n", strerror(errno));
                close(sockfd);
                continue;
            }

            break;
    }

    freeaddrinfo(servinfo);
    
    if(copy == NULL){
        fprintf(stderr, "Socket bind failed\n");
        return EXIT_FAILURE;
    }
    
    if(listen(sockfd, LISTEN_BACKLOG) == -1){
        fprintf(stderr, "listen:%s\n", strerror(errno));
        return EXIT_FAILURE;
    }
    hostname();
    printf("Listening on port:%s, socket:%i\n", MYPORT, sockfd);
    peer_address_size = sizeof(peer_addr);
    printf("Waiting on client\n");
    clientfd = accept(sockfd, (struct sockaddr *) &peer_addr, &peer_address_size);
    if(clientfd == -1){
        fprintf(stderr, "accept:%s\n", strerror(errno));
        return EXIT_FAILURE;
    }

    printf("Client accepted on socket %i\n", clientfd);
    if((bytes = recv(clientfd, msg, TXTLEN-1, 0)) == -1){
        fprintf(stderr, "recv:%s\n", strerror(errno));
        return EXIT_FAILURE;
    }
    msg[bytes] = '\0';
    printf("%s\n", msg);
    if(send(clientfd, msg, bytes, 0) == -1){
        fprintf(stderr, "send:%s\n", strerror(errno));
    }
    writeToFile(FILENAME, msg);
    close(sockfd);
    return EXIT_SUCCESS;
}