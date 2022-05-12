#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#define MY_SOCK_PATH "/tmp/foo"
#define LISTEN_BACKLOG 20

int main(int argc, char *argv[]){
    int sockfd, clientfd;
    struct sockaddr_un my_addr;
    struct sockaddr_storage peer_addr;
    socklen_t peer_address_size;

    sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if(errno != 0 ){
      fprintf(stderr, "%s\n", strerror(errno));
    }
    printf("Socket no:%i\n", sockfd);
    memset(&my_addr, 0, sizeof(struct sockaddr_un));

    my_addr.sun_family = AF_UNIX;
    strncpy(my_addr.sun_path, MY_SOCK_PATH, sizeof(my_addr.sun_path) - 1);

    if(bind(sockfd, (struct sockaddr *) &my_addr, sizeof(struct sockaddr_un)) == -1){
        fprintf(stderr,"Error:%s\n", strerror(errno));
    }

    if(listen(sockfd, LISTEN_BACKLOG) == -1){
        fprintf(stderr, "Error:%s\n", strerror(errno));
    };

    peer_address_size = sizeof(peer_addr);
    printf("Peer size:%i\n", peer_address_size);
    printf("My size:%li\n", sizeof(my_addr));
    printf("Waiting\n");
    clientfd = accept(sockfd, (struct sockaddr *) &peer_addr, &peer_address_size);
    if(clientfd == -1){
        fprintf(stderr, "Error:%s\n", strerror(errno));
    }

    if(remove(MY_SOCK_PATH) != 0){
        fprintf(stderr, "Error:%s\n", strerror(errno));
    }
    return EXIT_SUCCESS;
}