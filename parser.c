#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>

#define LOG 1
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

void usage(FILE *file, const char *program){
    fprintf(file, "Usage: %s [-flag] [path-to-file]\nFlags:\n\
     -ciff  provide a {.ciff} file\n\
     -caff  provide a {.caff} file \n",
    program);
}

bool check_extension(const char *file_path) {
    const char *extension = strrchr(file_path, '.');
    if (extension == NULL) {
        return false; // No extension found
    }
    ++extension;
    return (strcmp(extension, "ciff") == 0 || strcmp(extension, "caff") == 0);
}

// TODO::
char* get_file_name(const char* file_path) {
    char *separator = strrchr(file_path, '/');
    if (separator == NULL) {
        separator = (char *)file_path;
    }
    char *file_name = strdup(++separator);
    char *ext = strrchr(file_name, '.');
    if (ext != NULL) {
        *ext = '\0';
    }
    return file_name;
}

#define ERR_SET "\033[0;31m"
#define WARN_SET "\033[0;33m"
#define RESET "\033[0m"
void read_bytes(FILE *file, void *buff, const size_t buff_cap){
    assert(buff_cap != 0);
    size_t bytes_read = fread(buff, buff_cap, 1, file);
    if (bytes_read != 1) {
        if (ferror(file)) {
            fprintf(stderr, "%sERROR%s: could not read %zu bytes from file: %s\n",
                    ERR_SET, RESET, buff_cap, strerror(errno));
            exit(-1);
        } else if (feof(file)) {
            fprintf(stderr, "%sERROR%s: could not read %zu bytes from file: end of file\n",
                    ERR_SET, RESET, buff_cap);
            exit(-1);
        } else {
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
    for (size_t i = 0; i < buff_cap; ++i){
        if (buff[i] < 10 && i != 0 && i != 1){
            printf("0%u", buff[i]);
        } else {
            printf("%u", buff[i]);
        }

        if (i == 4){
            printf(":");
        } else if (i == 1 || i == 2){
            printf(".");
        } else if (i == 3){
            printf(". ");
        }
    }
    printf("\n");
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
        if (buff[i] == 0 && i != buff_cap - 1){
            printf(" #");
        } else { printf("%c", (char)buff[i]); }
    }
    printf("\n");
}

size_t translate_bytes(uint8_t *buff; const size_t buff_cap){
    uint64_t size = 0;
    for (int i = 0; i < buff_cap; i++) {
        size |= (uint64_t)buff[i] << (i * 8);
    }
    return (size_t)size;
}

size_t _read_bytes(FILE *file, const size_t quantity){
    uint8_t bytes[quantity];
    read_bytes(file, bytes, quantity);
    size_t value = translate_bytes(bytes, quantity);
}

#define MGC 4
#define CAP 8
#define ANM 8
const uint8_t magic_caff[MGC] = {67, 65, 70, 70};
size_t read_caff_hdr(FILE *file){
    // MAGIC
    uint8_t magic[MGC];
    read_bytes(file, magic, MGC);
    if (memcmp(magic_caff, magic, MGC) != 0){
        fprintf(stderr,
                "%sERROR%s: file has unknown magic in a block: %c %c %c %c\n",
                ERR_SET, RESET,
                (char)magic[0],
                (char)magic[1],
                (char)magic[2],
                (char)magic[3]);
        exit(-1);
    }

    // SIZE
    /* header length is also predefined
    so there is no need to use this
    information, just read it to confirm */
    const size_t size = _read_bytes(file, CAP);
    assert(size == 20);

    // ANIMATIONS
    /* not capacity but still a size value
    and still ocupies 8-bytes */
    const size_t anim = _read_bytes(file, ANM);
#if LOG
    printf("number of animations: %zu\n", n_anim);
#endif
    return anim;
}

#define DTE 6
void read_caff_crd(FILE *file){
    // DATE
    /* Y - year (2 bytes)
       M - month (1 byte)
       D - day (1 byte)
       h - hour (1 byte)
       m - minute (1 byte2) */
    uint8_t date[DTE];
    read_bytes(file, date, DTE);
#if LOG
    printf("date: ");
    print_date(date, DTE);
#endif
    // CREATOR
    const size_t s_creator = _read_bytes(file, CAP);
    if (s_creator == 0){
        printf("%sWARNING%s: file does not define the creator\n",
                WARN_SET, RESET);
    } else {
        uint8_t creator[s_creator];
        read_bytes(file, creator, s_creator);
#if LOG
        printf("creator: ");
        print_ascii(creator, s_creator);
#endif
    }
}

void create_jpg(const char *file_name, uint8_t *rgb_pixels, size_t s_pixels, size_t width, size_t height){
    int quality = 99;
    int stride_in_bytes = 3 * width;
    int write = stbi_write_jpg((char *)file_name, width, height, 3, rgb_pixels, quality);
    if (write == 0) {
        fprintf(stderr,
                "%sERROR%s: output file could not be written\n",
                ERR_SET, RESET);
        exit(-1);
    } else if (write == -1) {
        fprintf(stderr,
                "%sERROR%s: could not open output file for writing\n",
                ERR_SET, RESET);
        exit(-1);
    } else if (write == -2) {
        fprintf(stderr,
                "%sERROR%s: output file is not a JPEG file\n",
                ERR_SET, RESET);
        exit(-1);
    } else if (write == -3) {
        fprintf(stderr,
                "%sERROR%s: output file has invalid parameters\n",
                ERR_SET, RESET);
        exit(-1);
    }
    assert(write == 1);
#if LOG
    printf("successfully saved to \"%s\"\n", file_name);
#endif
}

#define WDT 8
#define HGT 8
#define ESC 10
const uint8_t magic_ciff[MGC] = {67, 73, 70, 70};
void read_ciff(FILE *file, const char *file_name, bool save){
    // MAGIC
    uint8_t magic[MGC];
    read_bytes(file, magic, MGC);
    if (memcmp(magic_ciff, magic, MGC) != 0){
        fprintf(stderr,
                "%sERROR%s: file has unknown magic in a block: %c %c %c %c\n",
                ERR_SET, RESET,
                (char)magic[0],
                (char)magic[1],
                (char)magic[2],
                (char)magic[3]);
        exit(-1);
    }

    // HEADER SIZE
    const size_t s_header = _read_bytes(file, CAP);

    /* 8-byte long integer,
    its value is the size of the image
	pixels located at the end of the file.
    Its value must be width*heigth*3 */
    // CONTENT SIZE
    size_t s_pixels = _read_bytes(file, CAP);
    // WIDTH
    size_t s_width = _read_bytes(file, WDT);
    // HEIGHT
    size_t s_height = _read_bytes(file, HGT);

    if (s_pixels != (s_width * s_height * 3)){
        fprintf(stderr,
                "%sERROR%s: pixel size is not equal to size defined in header\n",
                ERR_SET, RESET);
        exit(-1);
    }
    assert(s_pixels == (s_width * s_height * 3));

    // bytes for caption and tags
    const size_t s_caption_tags = s_header - MGC    // 4-byte magic
                                           - CAP      // 8-byte header size
                                           - CAP      // 8-byte content size
                                           - WDT    // 8-byte width of image
                                           - HGT;  // 8-byte height of image

    // CAPTION
    uint8_t t_caption[s_caption_tags];
    size_t iter = 0;
    uint8_t buff[1];
    read_bytes(file, buff, 1);
    while (buff[0] != ESCAPE) {
        t_caption[iter++] = buff[0];
        read_bytes(file, buff, 1);
        if (iter == s_caption_tags){
            fprintf(stderr,
                    "%sERROR%s: file caption larger than what header defines\n",
                    ERR_SET, RESET);
            exit(-1);
        }
    }
    // escaped caption
    size_t s_caption = iter + 1;
    if (iter == 0){
        printf("%sWARNING%s: file does not define the caption\n",
                WARN_SET, RESET);
    } else {
        uint8_t caption[s_caption];
        // copy data from temp caption
        for (int i = 0; i < s_caption; ++i){
            caption[i] = t_caption[i];
        }
#if LOG
        printf("caption: ");
        print_ascii(caption, s_caption);
#endif
    }

    // TAGS
    size_t s_tags = s_caption_tags - s_caption;
    if (s_tags == 0){
        printf("%sWARNING%s: file does not include any tags\n",
                WARN_SET, RESET);
    } else {
        uint8_t tags[s_tags];
        read_bytes(file, tags, s_tags);
        for (size_t i = 0; i < s_tags; ++i){
            if (tags[i] == ESCAPE){
                fprintf(stderr,
                        "%sERROR%s: file contains escape ASCII in tags\n",
                        ERR_SET, RESET);
                exit(-1);
            }
        }
#if LOG
        printf("tags: ");
        print_tags(tags, s_tags);
#endif
    }

    // PIXELS
    if (s_pixels == 0){
        printf("%sWARNING%s: file is missing the pixel data\n",
                WARN_SET, RESET);
    } else {
        uint8_t pixels[s_pixels];
        read_bytes(file, pixels, s_pixels);
        if (save){ create_jpg(file_name, pixels, s_pixels, s_width, s_height); }
    }
}

#define DUR 8
void read_caff_anm(FILE *file, const char *file_name, bool save){
    // DURATION
    size_t duration = _read_bytes(file, DUR);
# if LOG
    printf("duration: %s\n", duration);
#endif
    // CIFF
    read_ciff(file, file_name, save);
}

void read_caff(FILE *file, const char *file_name){
    // HEADER
    size_t h_id = _read_bytes(file, 1);
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
    const size_t h_length = _read_bytes(file, 8);
    assert(h_length == 20);

    /* the only valuable information
    in the header block is the
    number of animations in the CAFF */
    const size_t anim = read_caff_hdr(file);
#if LOG
    printf("\n");
#endif

    // ANIMATION + CREDITS blocks
    /* read all blocks from file
    + 1 for the credits block */
    bool save_first = true;
    for (size_t i = 0; i < anim + 1; ++i){
        uint8_t b_id = read_id(file);
        size_t b_size = _8bytes_size(file);
        if (b_id == 2){
            // CREDITS
            read_caff_crd(file);
#if LOG
            printf("\n");
#endif
        } else if (b_id == 3){
            // ANIMATION
            read_caff_anm(file, file_name, save_first);
            save_first = false;
#if LOG
            printf("\n");
#endif
        } else {
            fprintf(stderr,
                    "%sERROR%s: file has unknown id in a block: %uz\n",
                    ERR_SET, RESET, b_id);
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
    if (!(strcmp(flag, "-ciff") == 0 || strcmp(flag, "-caff") == 0)){
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
