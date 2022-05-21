#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>

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
    /*struct hostent *host_detail;
    char hostname[TXTLEN];
    char *IP;
    hostname[TXTLEN-1] = '\0';
    if(gethostname(hostname, TXTLEN) == -1){
        fprintf(stderr, "gethostname:%s\n", strerror(errno));
    }
    if((host_detail = gethostbyname(hostname)) == NULL){
        fprintf(stderr, "gethostbyname:%s\n", strerror(errno));
    }
    if((IP = inet_ntoa(*((struct in_addr*)host_detail->h_addr_list[0]))) == NULL){
        perror("inet_ntoa\n");
    }
    printf("My name is:%s\n", hostname);
    */
    int sockn;
    struct ifreq ifr;
    char array[] = "eth0";

    sockn = socket(AF_INET, SOCK_DGRAM, 0);
    ifr.ifr_addr.sa_family = AF_INET;
    strncpy(ifr.ifr_name, array, IFNAMSIZ-1);
    ioctl(sockn, SIOCGIFADDR, &ifr);
    close(sockn);

    printf("My IP is:%s\n", inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));
}

// Function to return output of client communication
int return_output(int client,char *msg){
    if(strcmp(msg, "REQUEST QUOTE") == 0){
        char cost[3];
        int random_number = rand() % 100 + 1;
        sprintf(cost, "%d", random_number);
        if(send(client, cost, sizeof(int), 0) == -1){
            fprintf(stderr, "send quote:%s\n", strerror(errno));
        }
        exit(EXIT_SUCCESS);
    }
    else if(strstr(msg, "requires") != NULL){
        strtok(msg, ",");
        FILE * fp;
        fp = fopen(strtok(NULL, ","), "w");
        fputs(strtok(NULL, ","), fp);
        
        if(send(client, "RECEIVED FILE.", sizeof(char) * strlen("RECEIVED FILE."), 0) == -1){
            fprintf(stderr, "send requires:%s\n", strerror(errno));
        }
        fclose(fp);
        exit(EXIT_SUCCESS);
    }

    // If its not requesting quote or sending file, its giving a command, send output of that command
    else{
        FILE * fp;
        char * line = NULL;
        size_t len = 0;
        ssize_t read;
        //char * output = malloc(0);

        fp = popen(msg, "r");
        if (fp == NULL)
            exit(EXIT_FAILURE);

        while ((read = getline(&line, &len, fp)) != -1) {
            send(client, line, sizeof(char) * strlen(line), 0);
            //output = realloc(output, sizeof(char) * (strlen(output) + strlen(line)));
            //strcat(output, line);
        }

        pclose(fp);
        if (line)
            free(line);
        exit(EXIT_SUCCESS);
    }
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
    my_addr.ai_family = AF_INET;
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
    do{
        hostname();
        printf("Listening on port:%s, socket:%i\n", MYPORT, sockfd);
        peer_address_size = sizeof(peer_addr);
        printf("Waiting on client\n");
        clientfd = accept(sockfd, (struct sockaddr *) &peer_addr, &peer_address_size);
        //clientfd = accept(sockfd,0,0);
        if(clientfd == -1){
            fprintf(stderr, "accept:%s\n", strerror(errno));
            return EXIT_FAILURE;
        }
        pid_t pid = fork();
        if (pid==-1) {
            fprintf(stderr, "fork:%s\n", strerror(errno));
            return EXIT_FAILURE;
        }
        else if (pid==0) {
            close(sockfd);
            printf("Client accepted on socket %i\n", clientfd);
            if((bytes = recv(clientfd, msg, TXTLEN-1, 0)) == -1){
                fprintf(stderr, "recv:%s\n", strerror(errno));
                return EXIT_FAILURE;
            }
            msg[bytes] = '\0';
            printf("%s\n", msg);
            //instead of sending the msg that was sent to server, send output of command
            if(/*send(clientfd, msg, bytes, 0)*/return_output(clientfd, msg) == -1){
                fprintf(stderr, "send:%s\n", strerror(errno));
            }
            writeToFile(FILENAME, msg);
            close(clientfd);
            exit(EXIT_SUCCESS);
        }
        else {
            close(clientfd);
        }
    }
    while(1);
    return EXIT_SUCCESS;
}