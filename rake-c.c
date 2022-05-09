#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define r_no 100
char *lines[r_no];
int row = 0;

/*
Read file by line by line
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
        if(line[0] != '#' && line[0] != '\n'){
            lines[row] = malloc(strlen(line) + 1);
            strcpy(lines[row], line);
            ++row;
        }
    }
    free(line);
    fclose(fp);
    return EXIT_SUCCESS;
}

char **split(const char *str){
    char **words = NULL;
    char *copy = strdup(str);
    char *word = strtok(copy," ");
    int w_count = 0;
    while(word != NULL){
        words = realloc(words, (w_count+1)*sizeof(words[0]));
        words[w_count] = word;
        word = strtok(NULL, " ");
        printf("%s\n", words[w_count]);
    }
    free(copy);
    return words;
}

int main(int argc, char **argv){ 
    if(argc > 1){
        readfile(argv[1]);
        split(lines[0]);
    }
    return EXIT_SUCCESS;
}
