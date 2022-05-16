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
#define FILENAME "output.txt"
#define COMMAND "/bin/nc"
#define HOSTNAME "localhost"

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
    return EXIT_SUCCESS;
}

int sendFile(){
    char *env[] = {HOSTNAME, MYPORT, FILENAME, NULL};
    if(execle("/bin/echo", "/bin/echo", "CITS3002", "| nc \"$HOSTNAME $MYPORT > $FILENAME\"", NULL, env) == -1){
        fprintf(stderr,"Error:%s\n", strerror(errno));
    }
    return EXIT_SUCCESS;
}

int main(int argc, char *argv[]){
    int sockfd, clientfd;
    struct addrinfo *servinfo, my_addr;
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

    sockfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
    if(errno != 0 ){
      fprintf(stderr, "%s\n", strerror(errno));
    }
    printf("Socket no:%i\n", sockfd);
    
    if(bind(sockfd, servinfo->ai_addr, servinfo->ai_addrlen) == -1){
        fprintf(stderr,"Error:%s\n", strerror(errno));
    }

    if(listen(sockfd, LISTEN_BACKLOG) == -1){
        fprintf(stderr, "Error:%s\n", strerror(errno));
    };

    peer_address_size = sizeof(peer_addr);
    printf("Waiting\n");
    clientfd = accept(sockfd, (struct sockaddr *) &peer_addr, &peer_address_size);
    if(clientfd == -1){
        fprintf(stderr, "Error:%s\n", strerror(errno));
    }

    printf("Client connected\n");
    if((bytes = recvfrom(clientfd, msg, TXTLEN-1, 0, (struct sockaddr *)&peer_addr, &peer_address_size)) == -1){
        fprintf(stderr, "Error:%s\n", strerror(errno));
    }
    msg[bytes] = '\0';
    printf("%s\n", msg);
    writeToFile(FILENAME, msg);
    sendFile();
    close(sockfd);
    return EXIT_SUCCESS;
}