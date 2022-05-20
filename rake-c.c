#include "rake-c.h"
#define FILE_NAME "event.txt"

struct local{
    char *file;
    char *args[R_NO];
    int arg_count;
};

struct remote{
    char *path;
    char *args[R_NO];
    int arg_count;
};

char *lines[R_NO];
struct local *locals = NULL;
struct remote *remotes = NULL;
int rows = 0;
int loc_count = 0;
int rem_count = 0;

/*
Read file by line by line
Ignore line starting with '#' and '\r'(return carriage).
allocate memory for line and store into array
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
            lines[rows] = malloc(strlen(line) + 1);
            strcpy(lines[rows], line);
            ++rows;
        }
    }
    free(line);
    fclose(fp);
    return EXIT_SUCCESS;
}

char **split(const char *str, int *w_count){
    char **words = NULL;
    char *copy = strdup(str);
    char *word = strtok(copy," ");
    *w_count = 0;

    if(copy != NULL){
        while(word != NULL){
            words = realloc(words, (*w_count+1)*sizeof(words[0]));
            words[*w_count] = word;
            word = strtok(NULL, " ");
            *w_count += 1;
        }
        free(word);
    }
    return words;
}
/*
Sort command into remote or local.
TODO: look into remote format.
*/
char *action_check(char **line, int w_count){
    char *arg1;
    char bin[32] = "/bin/";
    //Checking for single tab character or double tab.
    if(line[0][1] != '\t'){
        arg1 = malloc(strlen(line[0])-1);
        strncpy(arg1, &line[0][1], strlen(line[0]));
    }
    else{
        arg1 = malloc(strlen(line[0])-2);
        strncpy(arg1, &line[0][2], strlen(line[0]));
    }
    //Remote commands.
    if(strcmp(arg1, "remote-cc") == 0 || strcmp(arg1, "requires") == 0){
        struct remote *r = malloc(sizeof(*r) + sizeof(r->args[0]));
        strcpy(r->path,arg1);
        strcpy(r->args[0],arg1);
        for(int i = 1; i < w_count; i++){
            r->args[i] = malloc(strlen(line[i]) + 1);
            r->args[i] = line[i];
        }
        r->args[w_count] = malloc(sizeof(0));
        r->args[w_count] = NULL;
        r->arg_count = w_count;
        remotes = realloc(remotes, (rem_count+1)*sizeof(remotes[0]));
        remotes[rem_count] = *r;
        ++rem_count;
        return "remote";
    }
    //Local commands.
    else{
        strcat(bin,arg1);
        printf("%s\n", bin);
        struct local *l = malloc(sizeof(*l) + sizeof(l->args[0]));
        strcpy(l->file,bin);
        strcpy(l->args[0],bin);
        for(int i = 1; i < w_count; i++){
            l->args[i] = malloc(strlen(line[i] + 1));
            l->args[i] = line[i];
        }
        l->args[w_count] = malloc(sizeof(0));
        l->args[w_count] = NULL;
        l->arg_count = w_count;
        locals = realloc(locals, (loc_count+1)*sizeof(locals[0]));
        locals[loc_count] = *l;
        ++loc_count;
        return "local";
    }
}
/*
Split line and generate local or remote using action_check.
*/
char *populate(const char *line){
    int w_count;
    char **word_arr = split(line, &w_count);
    char *type = action_check(word_arr, w_count);
    if(strcmp(type,"local") == 0){
        printf("Local no.:%i\n", loc_count);
        printf("Path:%s\n", locals[loc_count-1].file);
        printf("Options:%i\n", locals[loc_count-1].arg_count);
        for(int i = 0; i < locals[loc_count-1].arg_count; i++){
            printf("%s\n", locals[loc_count-1].args[i]);
        }
    }
    else{
        printf("Remote no.:%i\n", rem_count);
        printf("Path:%s\n", remotes[rem_count-1].path);
        printf("Options:%i\n", remotes[rem_count-1].arg_count);
        for(int i = 0; i < remotes[rem_count-1].arg_count; i++){
            printf("%s\n", remotes[rem_count-1].args[i]);
        }
    }
    return type;
}
/*
Check for IPv4 or IPv6
Return the appropriate sock address.
*/
void *sockaddr_type(struct sockaddr *sockaddr){
    if(sockaddr->sa_family == AF_INET){
        return &(((struct sockaddr_in*)sockaddr)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sockaddr)->sin6_addr);
}

int communicate(char *hostname){
    int sockfd, bytes;
    char msg[TXT_LEN];
    struct addrinfo *servinfo, *copy, my_addr;
    char dst[INET6_ADDRSTRLEN];

    memset(&my_addr, 0, sizeof(my_addr));
    my_addr.ai_family = AF_INET;
    my_addr.ai_socktype = SOCK_STREAM;

    if ((getaddrinfo(hostname, PORT, &my_addr, &servinfo)) == -1){
        fprintf(stderr,"getaddrinfo:%s\n", strerror(errno));
        return EXIT_FAILURE;
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
        return EXIT_FAILURE;
    }
    printf("Client socket:%i\n", sockfd);
    inet_ntop(copy->ai_family, copy->ai_addr, dst, sizeof(dst));
    printf("Connected to %s\n", dst);

    freeaddrinfo(servinfo);
    
    if(send(sockfd, "Client to server success", TXT_LEN-1, 0) == -1){
        fprintf(stderr, "send:%s\n", strerror(errno));
    }

    if((bytes = recv(sockfd, msg, TXT_LEN-1, 0)) == -1){
         fprintf(stderr, "recv:%s\n", strerror(errno));
    }
    msg[bytes] = '\0';
    printf("%s\n", msg);

    close(sockfd);
    return EXIT_SUCCESS;
}
/*
Execute actionsets
*/
int execute(char* type){
    int client = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in my_addr, my_addr1;
    my_addr.sin_family = AF_INET;
    my_addr.sin_addr.s_addr = INADDR_ANY;
    my_addr.sin_port = htons(1299); ///?
    
    
    // Two buffer are for message communication
    char buffer1[256], buffer2[256];
 
    /*
    execv(locals[0].file,locals[0].args);
    if(errno != 0){
        fprintf(stderr, "exe failed\n");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
    */
    
    FILE* events = fopen(FILE_NAME, "w"); //errors
    if(FILE_NAME==NULL) {
        perror("Error opening file");}

    if (client < 0) {
        fprintf(FILE_NAME, "Client creation failed\n");
        printf("ERROR: Client creation failed\n");
        return 1;
    }

    if (bind(client, (struct sockaddr*) &my_addr1, sizeof(struct sockaddr_in)) == 0)
        printf("Binded Correctly\n");
    else
        fprintf(FILE_NAME, "Unable to bind\n");
        printf("Unable to bind\n");
     
    socklen_t addr_size = sizeof my_addr;
    int con = connect(client, (struct sockaddr*) &my_addr, sizeof my_addr);
    if (con == 0)
        printf("Client Connected\n");
    else
        fprintf(FILE_NAME, "Error in Connection\n");
        printf("Error in Connection\n");
 
    strcpy(buffer2, "Hello");
    send(client, buffer2, 256, 0);
    recv(client, buffer1, 256, 0);
    printf("Server : %s\n", buffer1);
    return 0;
    
    
    fclose(FILE_NAME);

    printf("Connected to %s\n", type);

    return EXIT_SUCCESS;
}

int main(int argc, char **argv){ 
    if(argc > 1){
        readfile(argv[1]);
    }
    communicate("127.0.1.1");
    return EXIT_SUCCESS;
}


//reads and stores the contents of a Rakefile, 
//executes actions (locally),
//captures and report actions' output, and 
//performs correctly based on whether the action(s) were successful or not

//actions of the nc command (allocate a socket, and connect to an already running rakeserver). just hardcode the network name or address, and port-number of your server into your client's code, but eventually replace these with information read and stored from your Rakefile.
