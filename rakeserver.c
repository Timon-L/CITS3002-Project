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
#define TXTLEN 1024
#define LISTEN_BACKLOG 20
#define FILENAME "filename"
#define NC "nc"
#define ECHO "echo"
#define HOSTNAME "localhost"
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

int nc_return(char *msg){
    char *str = msg;
    str[strlen(msg) - 1] = '\0';
    char *args[] = {ECHO, str, "|", NC, HOSTNAME, MYPORT, NULL};
    if(execvp(ECHO, args) == -1){
        fprintf(stderr,"Error:%s\n", strerror(errno));
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int main(int argc, char *argv[]){
    int sockfd, clientfd;
    struct addrinfo *servinfo, *copy, my_addr;
    struct sockaddr_storage peer_addr;
    socklen_t peer_address_size;
    char msg[TXTLEN];
    int bytes;

    memset(&my_addr, 0, sizeof(my_addr));
    my_addr.ai_family = AF_UNSPEC;
    my_addr.ai_socktype = SOCK_STREAM;
    my_addr.ai_flags = AI_PASSIVE;


    if(getaddrinfo(NULL, MYPORT, &my_addr, &servinfo) == -1){
        fprintf(stderr,"Error:%s\n", strerror(errno));
    }

    for(copy = servinfo; copy != NULL; copy = copy -> ai_next){
            if((sockfd = socket(copy->ai_family, copy->ai_socktype, copy->ai_protocol)) == -1){
                continue; 
            }

            if(bind(sockfd, copy->ai_addr, copy->ai_addrlen) == -1){
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
    printf("Socket no:%i\n", sockfd);

    if(listen(sockfd, LISTEN_BACKLOG) == -1){
        fprintf(stderr, "Error:%s\n", strerror(errno));
        return EXIT_FAILURE;
    }

    printf("Listening on port:%s, socket:%i\n", MYPORT, sockfd);
    peer_address_size = sizeof(peer_addr);
    printf("Waiting on client\n");
    clientfd = accept(sockfd, (struct sockaddr *) &peer_addr, &peer_address_size);
    if(clientfd == -1){
        fprintf(stderr, "Error:%s\n", strerror(errno));
        return EXIT_FAILURE;
    }

    printf("Client accepted on socket %i\n", clientfd);
    if((bytes = recvfrom(clientfd, msg, TXTLEN-1, 0, (struct sockaddr *)&peer_addr, &peer_address_size)) == -1){
        fprintf(stderr, "Error:%s\n", strerror(errno));
        return EXIT_FAILURE;
    }
    msg[bytes] = '\0';
    send(clientfd, msg, bytes, 0);
    //writeToFile(FILENAME, msg);
    close(sockfd);
    return EXIT_SUCCESS;
}