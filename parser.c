#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>

void usage(FILE *file, const char *program){
    fprintf(file, "Usage: %s [-flag] [path-to-file]\nFlags:\n\
     -ciff  provide a {.ciff} file\n\
     -caff  provide a {.caff} file \n",
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
// TODO: make ana argument that specifies what is read from memory for better error handling
void read_bytes(FILE *file, void *buff, const size_t buff_cap){
    if (buff_cap == 0){
        fprintf(stderr, "WARNING: an element is not defined in file\n");
        return;
    }
    assert(buff_cap != 0);

    size_t n = fread(buff, buff_cap, 1, file);
    if (n != 1) {
        if (ferror(file)) {
            fprintf(stderr, "%sERROR%s: could not read %zu bytes from file: %s\n",
                    ERR_SET, ERR_RESET, buff_cap, strerror(errno));
            exit(-1);
        } else if (feof(file)) {
            fprintf(stderr, "%sERROR%s: could not read %zu bytes from file: reached the end of file\n",
                    ERR_SET, ERR_RESET,
                    buff_cap);
            exit(-1);
        } else {
            fprintf(stderr, "%sERROR%s: could not read %zu bytes from file: fread returned with %zu\n",
                    ERR_SET, ERR_RESET, buff_cap, n);
            assert(0 && "unreachable");
        }
    }
}

void print_bytes(uint8_t *buff, const size_t buff_cap){
    for (size_t i = 0; i < buff_cap; ++i){
        printf("%u ", buff[i]);
    }
    printf("\n");
}

void print_date(uint8_t *buff, const size_t buff_cap){
    // TODO
}

void print_creator(uint8_t *buff, const size_t buff_cap){
    for (size_t i = 0; i < buff_cap; ++i){
        printf("%c", (char)buff[i]);
    }
    printf("\n");
}

size_t capsize(uint8_t *cap){
    uint64_t size = 0;
    for (int i = 0; i < 8; i++) {
        size |= (uint64_t)cap[i] << (i * 8);
    }
    return (size_t)size;
}

#define S_ID 1
uint8_t read_id(FILE *file){
    uint8_t id[S_ID];
    read_bytes(file, id, S_ID);
    assert(id != NULL);

    if (id[0] != 1 && id[0] != 2 && id[0] != 3){
        fprintf(stderr,
            "%sERROR%s: file format is not supported {block_id: %u}\n",
             ERR_SET, ERR_RESET, id[0]);
        exit(-1);
    }
    return id[0];
}

#define S_CAP 8
size_t read_size(FILE *file){
    uint8_t cap[S_CAP];
    read_bytes(file, cap, S_CAP);
    size_t size = capsize(cap);
    return size;
}

#define S_ANM 8
#define S_MAGIC 4
const uint8_t magic_caff[S_MAGIC] = {67, 65, 70, 70};
// returns the number of animations in the caff
size_t read_hdr(FILE *file){
    /* cap is no needed for hdr
    because all chunks lengths
    are predefined in the format */
    const size_t s_block = read_size(file);
    assert(s_block == 20);

    uint8_t f_magic[S_MAGIC];
    read_bytes(file, f_magic, S_MAGIC);
    if (memcmp(magic_caff, f_magic, S_MAGIC) != 0){
        fprintf(stderr,
            "%sERROR%s: file format is not supported {magic: %c %c %c %c}\n",
            ERR_SET, ERR_RESET,
            (char)f_magic[0],
            (char)f_magic[1],
            (char)f_magic[2],
            (char)f_magic[3]);
        exit(-1);
    }

    /* header length is also predefined
    so there is no need to use this
    information, just read it to confirm */
    const size_t s_header = read_size(file);
    assert(s_header == s_block);

    uint8_t n_animB[S_ANM];
    read_bytes(file, n_animB, S_ANM);
    size_t n_anim = capsize(n_animB);

    return n_anim;
}


// TODO: error checking
#define S_DATE 6
void read_crd(FILE *file){
    const size_t s_block = read_size(file);
    (void) s_block;

    /* Y - year (2 bytes)
       M - month (1 byte)
       D - day (1 byte)
       h - hour (1 byte)
       m - minute (1 byte2) */
    uint8_t date[S_DATE];
    read_bytes(file, date, S_DATE);
    print_date(date, S_DATE);

    const size_t s_creator = read_size(file);
    uint8_t creator[s_creator];
    read_bytes(file, creator, s_creator);
    // TODO: error handling in read_bytes, when 0 byte should be read
    //print_creator(creator, s_creator);
}

#define S_WIDTH 8
#define S_HEIGHT 8
const uint8_t magic_ciff[S_MAGIC] = {67, 73, 70, 70};
void read_CIFF(FILE *file){
    uint8_t f_magic[S_MAGIC];
    read_bytes(file, f_magic, S_MAGIC);
    if (memcmp(magic_ciff, f_magic, S_MAGIC) != 0){
        fprintf(stderr,
            "%sERROR%s: file format is not supported {magic: %c %c %c %c}\n",
            ERR_SET, ERR_RESET,
            (char)f_magic[0],
            (char)f_magic[1],
            (char)f_magic[2],
            (char)f_magic[3]);
        exit(-1);
    }

    const size_t s_header = read_size(file);
    (void) s_header;

    /* 8-byte long integer,
    its value is the size of the image
	pixels located at the end of the file.
     Its value must be width*heigth*3 */
    const size_t s_content = read_size(file);
    (void) s_content;

    uint8_t width[S_WIDTH];
    read_bytes(file, width, S_WIDTH);

    uint8_t height[S_HEIGHT];
    read_bytes(file, height, S_HEIGHT);

    // TODO: caption
    // TODO: tags

    // TODO: pixels
}

#define D_ANM 8
void read_anm(FILE *file){
    const size_t s_block = read_size(file);
    (void) s_block;

    uint8_t *duration[D_ANM];
    read_bytes(file, duration, D_ANM);

    read_CIFF(file);
}

int main(int argc, char const *argv[])
{
    // check all arguments
    (void) argc;
    assert(*argv != NULL);
    const char *program = *argv++;

    // check the flag
    if (*argv == NULL){
        fprintf(stderr,
            "%sERROR%s: no arguments provided\n", ERR_SET, ERR_RESET);
        usage(stderr, program);
        exit(-1);
    }
    const char *flag = *argv++;
    if (!check_flag(flag)){
        fprintf(stderr,
            "%sERROR%s: foreign flag \"%s\"\n", ERR_SET, ERR_RESET, flag);
        usage(stderr, program);
        exit(-1);
    }

    // check the input file
    if (*argv == NULL){
        fprintf(stderr,
            "%sERROR%s: no input file provided\n", ERR_SET, ERR_RESET);
        usage(stderr, program);
        exit(-1);
    }
    const char *file_path = *argv++;
    if (!check_extension(file_path)){
        fprintf(stderr,
            "%sERROR%s: equivocal extension in \"%s\"\n", ERR_SET, ERR_RESET, file_path);
        usage(stderr, program);
        exit(-1);
    }

    // check for overflow
    if (*argv != NULL){
        fprintf(stderr,
            "%sERROR%s: too many argumetns\n", ERR_SET, ERR_RESET);
        usage(stderr, program);
        exit(-1);
    }
    assert(*argv == NULL);

    // open file
    FILE *file = fopen(file_path, "rb");
    if (file == NULL){
        fprintf(stderr,
            "ERROR: could not open file %s: %s\n",
                file_path, strerror(errno));
        exit(-1);
    }

    // PARSING
    uint8_t h_id = read_id(file);
    size_t n_anim = read_hdr(file);
    //printf("number of animations: %zu\n", n_anim);

    uint8_t c_id = read_id(file);
    read_crd(file);

    uint8_t a_id = read_id(file);
    read_anm(file);

    return 0;
}

/*
// jump to func based on id
switch (id) {
    case 1:
        read_hdr(file);
        break;
    case 2:
        read_crd(file);
        break;
    case 3:
        read_anm(file);
        break;
    default:
        fprintf(stderr,
            "ERROR: file format is unsupported {block_id: %s}\n",id);
        exit(-1);
}
*/
