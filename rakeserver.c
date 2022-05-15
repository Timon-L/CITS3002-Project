#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <netdb.h>

//#define MY_SOCK_PATH "localhost"
#define MYPORT "1299"
#define LISTEN_BACKLOG 20

int main(int argc, char *argv[]){
    int sockfd, clientfd;
    struct addrinfo *servinfo, my_addr;
    struct sockaddr_storage peer_addr;
    socklen_t peer_address_size;

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
    //if(remove(MY_SOCK_PATH) != 0){
        //fprintf(stderr, "Error:%s\n", strerror(errno));
    //}
    return EXIT_SUCCESS;
}