#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

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

char *get_file_name(const char *file_path){
    char *file = strdup(file_path);
    char *token = strtok(file, "/");
    char *file_name = NULL;

    while (token != NULL){
        file_name = token;
        token = strtok(NULL, "/");
    }

    file_name = strtok(file_name, ".");
    return file_name;
}

#define ERR_SET "\033[0;31m"
#define WARN_SET "\033[0;33m"
#define RESET "\033[0m"
// TODO: make ana argument that specifies what is read from memory for better error handling
void read_bytes(FILE *file, void *buff, const size_t buff_cap){
    if (buff_cap == 0){
        fprintf(stderr, "%sWARNING%s: an element is not defined in file\n",
                WARN_SET, RESET);
        return;
    }
    assert(buff_cap != 0);

    size_t bytes_read = fread(buff, buff_cap, 1, file);
    if (bytes_read != 1) {
        if (ferror(file)) {
            fprintf(stderr, "%sERROR%s: could not read %zu bytes from file: %s\n",
                    ERR_SET, RESET, buff_cap, strerror(errno));
            exit(-1);
        } else if (feof(file)) {
            fprintf(stderr, "%sERROR%s: could not read %zu bytes from file: reached the end of file\n",
                    ERR_SET, RESET,
                    buff_cap);
            exit(-1);
        } else {
            fprintf(stderr, "%sERROR%s: could not read %zu bytes from file: fread returned with %zu\n",
                    ERR_SET, RESET, buff_cap, bytes_read);
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

void print_ascii(uint8_t *buff, const size_t buff_cap){
    for (size_t i = 0; i < buff_cap; ++i){
        printf("%c", (char)buff[i]);
    }
    printf("\n");
}

void print_tags(uint8_t *buff, const size_t buff_cap){
    printf("#");
    for (size_t i = 0; i < buff_cap; ++i){
        if (buff[i] == 0){
            printf(" #");
        } else { printf("%c", (char)buff[i]); }
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
// reads the next byte and returns the value of it
uint8_t read_id(FILE *file){
    uint8_t id[S_ID];
    read_bytes(file, id, S_ID);
    assert(id != NULL);

    if (id[0] != 1 && id[0] != 2 && id[0] != 3){
        fprintf(stderr,
            "%sERROR%s: file format is not supported {block_id: %u}\n",
             ERR_SET, RESET, id[0]);
        exit(-1);
    }
    return id[0];
}

#define S_CAP 8
size_t _8bytes_size(FILE *file){
    uint8_t buff[S_CAP];
    read_bytes(file, buff, S_CAP);
    size_t size = capsize(buff);
    return size;
}

#define S_ANM 8
#define S_MAGIC 4
const uint8_t magic_caff[S_MAGIC] = {67, 65, 70, 70};
// returns the number of animations in the caff
size_t read_caff_hdr(FILE *file){
    uint8_t f_magic[S_MAGIC];
    read_bytes(file, f_magic, S_MAGIC);
    if (memcmp(magic_caff, f_magic, S_MAGIC) != 0){
        fprintf(stderr,
            "%sERROR%s: file format is not supported {magic: %c %c %c %c}\n",
            ERR_SET, RESET,
            (char)f_magic[0],
            (char)f_magic[1],
            (char)f_magic[2],
            (char)f_magic[3]);
        exit(-1);
    }

    /* header length is also predefined
    so there is no need to use this
    information, just read it to confirm */
    const size_t s_header = _8bytes_size(file);
    assert(s_header == 20);

    /* not capacity but still a size value
    and still ocupies 8-bytes */
    const size_t n_anim = _8bytes_size(file);
    return n_anim;
}

// TODO: error checking
#define S_DATE 6
void read_caff_crd(FILE *file){

    /* Y - year (2 bytes)
       M - month (1 byte)
       D - day (1 byte)
       h - hour (1 byte)
       m - minute (1 byte2) */
    uint8_t date[S_DATE];
    read_bytes(file, date, S_DATE);
    // printf("date: ");
    // print_date(date, S_DATE);

    const size_t s_creator = _8bytes_size(file);
    uint8_t creator[s_creator];
    read_bytes(file, creator, s_creator);
    // TODO: error handling in read_bytes, when 0 byte should be read
     // printf("creator: ");
     // print_ascii(creator, s_creator);
}

void create_jpg(const char *file_name, uint8_t *rgb_pixels, size_t s_pixels, size_t width, size_t height){
    int quality = 90; // set JPEG quality to 90%
    int stride_in_bytes = 3 * width; // RGB stride is 3 bytes per pixel
    stbi_write_jpg((char *)file_name, width, height, 3, rgb_pixels, quality);
}
#define S_WIDTH 8
#define S_HEIGHT 8
#define ESCAPE 10
const uint8_t magic_ciff[S_MAGIC] = {67, 73, 70, 70};
void read_ciff(FILE *file, const char *file_name, bool save){
    uint8_t f_magic[S_MAGIC];
    read_bytes(file, f_magic, S_MAGIC);
    if (memcmp(magic_ciff, f_magic, S_MAGIC) != 0){
        fprintf(stderr,
                "%sERROR%s: file format is not supported {magic: %c %c %c %c}\n",
                ERR_SET, RESET,
                (char)f_magic[0],
                (char)f_magic[1],
                (char)f_magic[2],
                (char)f_magic[3]);
        exit(-1);
    }

    // S_CAP size buffer
    const size_t s_header = _8bytes_size(file);

    /* 8-byte long integer,
    its value is the size of the image
	pixels located at the end of the file.
    Its value must be width*heigth*3 */
    size_t s_pixels = _8bytes_size(file);
    size_t s_width = _8bytes_size(file);
    size_t s_height = _8bytes_size(file);

    if (s_pixels != (s_width * s_height * 3)){
        fprintf(stderr,
                "%sERROR%s: pixel size is not equal to size defined in header\n",
                ERR_SET, RESET);
        exit(-1);
    }
    assert(s_pixels == (s_width * s_height * 3));

    // bytes for caption and tags
    const size_t s_caption_tags = s_header - S_MAGIC    // 4-byte magic
                                           - S_CAP      // 8-byte header size
                                           - S_CAP      // 8-byte content size
                                           - S_WIDTH    // 8-byte width of image
                                           - S_HEIGHT;  // 8-byte height of image

    // caption
    uint8_t t_caption[s_caption_tags];
    size_t iter = 0;
    uint8_t buff[1];
    read_bytes(file, buff, 1);
    while (buff[0] != ESCAPE) {
        t_caption[iter] = buff[0];
        read_bytes(file, buff, 1);
        ++iter;
        if (iter == s_caption_tags){
            fprintf(stderr,
                    "%sERROR%s: file format is not supported: captivate caption please\n",
                    ERR_SET, RESET);
            exit(-1);
        }
    }
    // escaped caption
    size_t s_caption = iter + 1;
    uint8_t caption[s_caption];
    for (int i = 0; i < s_caption; ++i){
        caption[i] = t_caption[i];
    }
    // printf("caption: ");
    // print_ascii(caption, s_caption);

    // tags
    size_t s_tags = s_caption_tags - s_caption;
    uint8_t tags[s_tags];
    if (s_tags != 0){
        read_bytes(file, tags, s_tags);
        for (size_t i = 0; i < s_tags; ++i){
            if (tags[i] == ESCAPE){
                fprintf(stderr,
                        "%sERROR%s: file contains too many escapes: escapes in tags\n",
                        ERR_SET, RESET);
                exit(-1);
            }
        }
    } else {
        assert(s_tags == 0);
        fprintf(stderr, "%sWARNING%s: no tags are added to the file\n",
                WARN_SET, RESET);
    }

    // printf("tags: ");
    // print_tags(tags, s_tags);

    uint8_t pixels[s_pixels];
    read_bytes(file, pixels, s_pixels);

    if (save){ create_jpg(file_name, pixels, s_pixels, s_width, s_height); }
}

#define D_ANM 8
void read_caff_anm(FILE *file, const char *file_name, bool save){
    uint8_t *duration[D_ANM];
    read_bytes(file, duration, D_ANM);

    read_ciff(file, file_name, save);
}

void read_caff(FILE *file, const char *file_name){
    // HEADER
    uint8_t h_id = read_id(file);
    if (h_id != 1){
        fprintf(stderr,
                "%sERROR%s: file does not start with a header\n",
                ERR_SET, RESET);
        exit(-1);
    }
    assert(h_id == 1);

    /* cap is no needed for hdr
    because all chunks lengths
    are predefined in the format */
    const size_t h_length = _8bytes_size(file);
    assert(h_length == 20);

    /* the only valuable information
    in the header block is the
    number of animations in the CAFF */
    const size_t n_anim = read_caff_hdr(file);
    // printf("number of animations: %zu\n", n_anim);

    /* read all blocks from file
    + 1 for the credits block */
    bool save_first = true;
    for (size_t i = 0; i < n_anim + 1; ++i){
        uint8_t b_id = read_id(file);
        size_t b_size = _8bytes_size(file);
        if (b_id == 2){
            // CREDITS
            read_caff_crd(file);
        } else if (b_id == 3){
            // ANIMATION
            read_caff_anm(file, file_name, save_first);
            save_first = false;
        } else {
            fprintf(stderr,
                    "%sERROR%s: file contains unknows block type\n",
                    ERR_SET, RESET);
            exit(-1);
        }
    }
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
            "%sERROR%s: no arguments provided\n", ERR_SET, RESET);
        usage(stderr, program);
        exit(-1);
    }
    const char *flag = *argv++;
    if (!check_flag(flag)){
        fprintf(stderr,
            "%sERROR%s: foreign flag \"%s\"\n", ERR_SET, RESET, flag);
        usage(stderr, program);
        exit(-1);
    }

    // check the input file
    if (*argv == NULL){
        fprintf(stderr,
            "%sERROR%s: no input file provided\n", ERR_SET, RESET);
        usage(stderr, program);
        exit(-1);
    }
    const char *file_path = *argv++;
    if (!check_extension(file_path)){
        fprintf(stderr,
            "%sERROR%s: equivocal extension in \"%s\"\n", ERR_SET, RESET, file_path);
        usage(stderr, program);
        exit(-1);
    }

    const char *file_name = get_file_name(file_path);
    if (file_name == NULL){
        fprintf(stderr,
            "%sERROR%s: filename was not provided\n", ERR_SET, RESET);
        usage(stderr, program);
        exit(-1);
    }
    assert(file_name != NULL);
    strcat((char *)file_name, ".jpg");

    // check for overflow
    if (*argv != NULL){
        fprintf(stderr,
            "%sERROR%s: too many argumetns\n", ERR_SET, RESET);
        usage(stderr, program);
        exit(-1);
    }
    assert(*argv == NULL);

#if 1
    // open file
    FILE *file = fopen(file_path, "rb");
    if (file == NULL){
        fprintf(stderr,
            "%sERROR%s: could not open file %s: %s\n",
                ERR_SET, RESET, file_path, strerror(errno));
        exit(-1);
    }

    if (strcmp(flag, "-caff") == 0){
        read_caff(file, file_name);
    } else if (strcmp(flag, "-ciff") == 0){
        read_ciff(file, file_name, true);
    }

#endif

    return 0;
}

// TODO: file_name handling could be simpler
