#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define r_no 100
char *array[r_no];
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
        if(line[0] != '#'){
            array[row] = malloc(strlen(line) + 1);
            strcpy(array[row], line);
            ++row;
        }
    }
    free(line);
    fclose(fp);
    return EXIT_SUCCESS;
}

int main(int argc, char **argv){ 
    if(argc > 1){
        readfile(argv[1]);
    }
    for (int i = 0; i < row; i++){
        printf("%s\n", array[i]);
    }
    return EXIT_SUCCESS;
}
