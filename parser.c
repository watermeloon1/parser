#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

void usage(FILE *file, const char *program)
{
    fprintf(file, "Usage: %s [-flag] [input_file]\nFlags:\n\
     -ciff  provide a {.ciff} file format\n\
     -caff  provide a {.caff} file format\n",
    program);
}

bool check_flag(const char *flag){
    if (strcmp(flag, "-ciff") == 0 || strcmp(flag, "-caff") == 0){
        return true;
    }
    return false;
}

bool check_extension(const char *file_path){
    char *file = strdup(file_path);
    char *token = strtok(file, ".");
    char *extension = NULL;

    while (token != NULL){
        extension = token;
        token = strtok(NULL, ".");
    }

    if (extension == NULL){
        free(file);
        file = NULL;
        return false;
    }
    assert(extension != NULL);

    if (strcmp(extension, "ciff") == 0 || strcmp(extension, "caff") == 0){
        free(file);
        file = NULL;
        return true;
    }
    free(file);
    file = NULL;
    return false;
}

#define ERR_SET "\033[0;31m"
#define ERR_RESET "\033[0m"
int main(int argc, char const *argv[])
{
    // check all arguments
    (void) argc;
    assert(*argv != NULL);
    const char *program = *argv++;

    // check the flag
    if (*argv == NULL){
        fprintf(stderr, "%sERROR%s: no arguments provided\n", ERR_SET, ERR_RESET);
        usage(stderr, program);
        exit(-1);
    }
    const char *flag = *argv++;
    if (!check_flag(flag)){
        fprintf(stderr, "%sERROR%s: foreign flag \"%s\"\n", ERR_SET, ERR_RESET, flag);
        usage(stderr, program);
        exit(-1);
    }

    // check the input file
    if (*argv == NULL){
        fprintf(stderr, "%sERROR%s: no input file provided\n", ERR_SET, ERR_RESET);
        usage(stderr, program);
        exit(-1);
    }
    const char *file_path = *argv++;
    if (!check_extension(file_path)){
        fprintf(stderr, "%sERROR%s: equivocal extension in \"%s\"\n", ERR_SET, ERR_RESET, file_path);
        usage(stderr, program);
        exit(-1);
    }

    // check for overflow
    if (*argv != NULL){
        fprintf(stderr, "%sERROR%s: too many argumetns\n", ERR_SET, ERR_RESET);
        usage(stderr, program);
        exit(-1);
    }
    assert(*argv == NULL);

    // parsing

    return 0;
}
