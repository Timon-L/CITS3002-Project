#include "rake-c.h"

struct local{
    char cmd[TXT_LEN];
    char requires[TXT_LEN];
};

struct remote{
    char cmd[TXT_LEN];
    char requires[TXT_LEN];
};

struct action{
    struct local locals[CMD_NO];
    struct remote remotes[CMD_NO];
    int loc_count;
    int rem_count;
    int req_state; //switch between 1 or 0, 0 for local commands and 1 for remote commands.
};

struct action actions[100];
int act_count = 0;
char *lines[R_NO];
int rows = -1;
int verbose = 0;

/*
*   Read file by line by line
*   Ignore line starting with '#' and '\r'(return carriage).
*   allocate memory for line and store into array
*/
int readfile(char *filename){
    size_t bufsize = 0;
    ssize_t read;
    char *line;
    FILE *fp = fopen(filename, "r");

    if(!fp){
        fprintf(stderr, "Fail to open file '%s'\n", filename);
        return EXIT_FAILURE;
    }

    while((read = getline(&line, &bufsize, fp)) != -1){
        if(line[0] != '#' && line[0] != '\r'){
            ++rows;
            line[strlen(line) - 2] = '\0';
            lines[rows] = malloc(strlen(line) + 1);
            strcpy(lines[rows], line);
        }
    }
    free(line);
    fclose(fp);
    return EXIT_SUCCESS;
}

/*  Break line into words, and put it in an array.
*   Break each line at the given delim.
*   Return the array of words.
*/  
char **split(const char *str, int *w_count, char *delim){
    char **words = NULL;
    char *copy = strdup(str);
    char *word = strtok(copy, delim);
    *w_count = 0;

    if(copy != NULL){
        while(word != NULL){
            if(strcmp(word, "PORT") != 0 && strcmp(word, "HOSTS") != 0 && strcmp(word, "=") != 0){
                words = realloc(words, (*w_count+1)*sizeof(words[0]));
                words[*w_count] = malloc(sizeof(word));
                strcpy(words[*w_count],word);
                *w_count += 1;
            }
            word = strtok(NULL, delim);
        }
        free(word);
    }
    return words;
}

/*
*   Sort command into remote or local.
*/
void action_check(const char *line, char **words, int w_count){
    char *arg1;
    //Checking for single tab character or double tab.
    if(line[1] != '\t'){
        arg1 = malloc(strlen(words[0]));
        strcpy(arg1, &words[0][1]);
        if(strstr(arg1, "remote") != NULL){
            strcpy(actions[act_count].remotes[actions[act_count].rem_count].cmd,&line[1]);
            actions[act_count].rem_count++;
            actions[act_count].req_state = 1;
        }
        else{
            strcpy(actions[act_count].locals[actions[act_count].loc_count].cmd,&line[1]);
            actions[act_count].loc_count++;
            actions[act_count].req_state = 0;
        }
    }
    //Decide where to allocate required file req_state 0 = local, 1 = remote;
    else{
        if(actions[act_count].req_state == 0){
            strcpy(actions[act_count].locals[actions[act_count].loc_count].requires,&line[2]);
        }
        else{
            strcpy(actions[act_count].remotes[actions[act_count].rem_count].requires,&line[2]);
        }
    }
}

/*
*   Split line and generate local or remote using action_check.
*/
void populate(const char *line){
    char *space = " ";
    if(line[0] == '\t'){
        int w_count;
        char **word_arr = split(line, &w_count, space);
        action_check(line, word_arr, w_count);
    }
    else{
        act_count++;   
    }
}

/*  Attempt to connect to host on the given port.
*   return socket number.
*/
int communicate(char *hostname, char *port){
    int sockfd;
    struct addrinfo *servinfo, *copy, hints;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_CANONNAME;

    if ((getaddrinfo(hostname, port, &hints, &servinfo)) == -1){
        fprintf(stderr,"getaddrinfo:%s\n", strerror(errno));
    }

    for(copy = servinfo; copy != NULL; copy = copy->ai_next){
        if((sockfd = socket(copy->ai_family, copy->ai_socktype, copy->ai_protocol)) == -1){
            fprintf(stderr, "sockfd:%s\n", strerror(errno));
            continue;
        }

        if(connect(sockfd, copy->ai_addr, copy->ai_addrlen) == -1){
            fprintf(stderr, "connect:%s\n", strerror(errno));
            close(sockfd);
            continue;
        }

        break;
    }

    if(copy == NULL){
        fprintf(stderr, "Client failed to connect\n");
        return -1;
    }
    printf("Client socket:%i\n", sockfd);
    printf("Hostname: %s\n", copy->ai_canonname);
    freeaddrinfo(servinfo);
    return sockfd;
}

/*  Takes the given existing list of sockets and counts of socket
*   use select() to check if any of the sockets are ready for read.
*   If nothing can be read, close the socket and remove from list.
*/
int read_block(int *fd_list, int fd_count){
    int fd_active = fd_count;
    char msg[TXT_LEN];
    int nbytes;
    while(fd_active > 0){
        fd_set read_fds;
        FD_ZERO(&read_fds);
        int fdmax = -1;

        for(int i = 0; i < fd_count; i++){
            if(fd_list[i] >= 0){
                FD_SET(fd_list[i], &read_fds);
                if(fd_list[i] > fdmax){
                    fdmax = fd_list[i];
                }
            }
        }

        if(fdmax == -1){
            printf("No socket found\n");
            return EXIT_FAILURE;
        }

        if(select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1){
            fprintf(stderr, "select:%s\n", strerror(errno));
            return EXIT_FAILURE;
        }

        for(int j = 0; j <= fd_count; j++){
            if(fd_list[j] >= 0 && FD_ISSET(fd_list[j], &read_fds)){
                nbytes = recv(fd_list[j], msg, TXT_LEN-1, 0);
                if(nbytes > 0){
                    msg[nbytes] = '\0';
                    printf("%s\n", msg);
                }
                if(nbytes <= 0){
                    shutdown(fd_list[j], 2);
                    close(fd_list[j]);
                    fprintf(stdout, "Socket %i: closed\n", fd_list[j]);
                    fd_list[j] = -1;
                    fd_active--;     
                }     
            }
        }
    }
    return EXIT_SUCCESS;
}

/*  Takes the given existing list of sockets and counts of socket
*   use select() to check if any of the sockets are ready for write.
*   If socket if not available, close the socket and remove from list.
*   Return the final active socket count.
*/
int write_block(int *fd_list, int fd_count){
    int fd_active = 0;
    char *hosts[2] = {"localhost", "remote"};
    int h_index = 0;
    while(fd_active != fd_count){
        fd_set write_fds;
        FD_ZERO(&write_fds);
        int fdmax = -1;

        for(int i = 0; i < fd_count; i++){
            if(fd_list[i] >= 0){
                FD_SET(fd_list[i], &write_fds);
                if(fd_list[i] > fdmax){
                    fdmax = fd_list[i];
                }
            }
        }

        if(fdmax == -1){
            printf("No socket found\n");
            return -1;
        }

        if(select(fdmax+1, NULL, &write_fds, NULL, NULL) == -1){
            fprintf(stderr, "select:%s\n", strerror(errno));
            return -1;
        }

        for(int j = 0; j <= fd_count; j++){
            if(fd_list[j] >= 0 && FD_ISSET(fd_list[j], &write_fds)){
                if(send(fd_list[j], hosts[h_index], TXT_LEN-1, 0) == -1){
                    fprintf(stderr, "send:%s\n", strerror(errno));
                    close(fd_list[j]);
                    fprintf(stdout, "%2i: closed\n", fd_list[j]);
                    fd_list[j] = -1;   
                }
                h_index++;
                fd_active++;
            }
        }
    }
    return fd_active;
}

/*  Check if hosts has a preferred port
*   else assign the default port to host.
*   Establish commuication with host and return sock number.
*/
int sock_assign(char *hosts, char* default_port){
    int sock_no;
    int arr_count;
    char *colon = ":";
    if(strstr(hosts, ":") != NULL){
        char **name_n_port = split(hosts, &arr_count, colon);
        sock_no = communicate(name_n_port[0], name_n_port[1]);
    }
    else{
        sock_no = communicate(hosts, default_port);
    }
    return sock_no;
}

int main(int argc, char **argv){ 
    int *fd_list;
    int fd_count;
    int port_count;
    int fd_max;
    char tmp_str[128];
    int sock_no;
    char *space = " ";
    if(argc == 3){
        if(strcmp(argv[1], "-v") == 0){
            verbose = 1;
            readfile(argv[2]);
        }
    }
    else if(argc > 1){
        readfile(argv[1]);
    }
    for(int i = 2; i <= rows; i++){
        populate(lines[i]); //populate structs.
    }

    strcpy(tmp_str,lines[0]);
    char *port = strdup(split(tmp_str, &port_count, space)[0]); //defaut port number.
    memset(tmp_str, '\0', sizeof(tmp_str));
    strcpy(tmp_str,lines[1]);
    char **hosts = split(tmp_str, &fd_count, space); //list of host name.

    fd_max = fd_count;
    for(int i = 0; i < fd_max; i++){
        sock_no = sock_assign(hosts[i], port);
        if(sock_no != -1){
            fd_list = realloc(fd_list, sizeof(int) * (i+1));
            fd_list[i] = sock_no;
        }
        else{
            fd_count--;
        }
    }
    fd_count = write_block(fd_list, fd_count);
    if(fd_count != -1){
        read_block(fd_list, fd_count);
    }
    else{
        printf("Read error\n");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}