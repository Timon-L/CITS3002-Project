#include "rake-c.h"

struct local{
    char *file;
    char *args[r_no];
    int arg_count;
};

struct remote{
    char *path;
    char *args[r_no];
    int arg_count;
};

char *lines[r_no];
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
            //printf("%s\n", words[*w_count]);
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
Execute actionsets
*/
int execute(char* type){
    //char *binaryPath = "/bin/echo";
    //char *args[] = {binaryPath, "starting", "actionset1", NULL};
 
    if(strcmp(type, "local") == 0){
        execv(locals[0].file,locals[0].args);
        if(errno != 0){
            fprintf(stderr, "exe failed\n");
        }
    }
    return EXIT_SUCCESS;
}

int main(int argc, char **argv){ 
    if(argc > 1){
        readfile(argv[1]);
        //for(int i = 3; i < rows-1; i++){
            //populate(lines[i]);
        //}
        populate(lines[3]);
        
    }
    //execute(array);
    return EXIT_SUCCESS;
}
