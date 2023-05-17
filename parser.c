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

#define ERR_SET "\033[0;31m"
#define WARN_SET "\033[0;33m"
#define RESET "\033[0m"

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

char *g_file_name;
void set_g_file_name(const char* file_path) {
    char *separator = strrchr(file_path, '/');
    if (separator == NULL) {
        separator = (char *)file_path;
    }
    char *file_name = strdup(++separator);
    if (file_name == NULL){
        fprintf(stderr, "%sERROR%s: filename was not provided in the path\n",
                ERR_SET, RESET);
        exit(-1);
    }
    char *ext = strrchr(file_name, '.');
    if (ext != NULL) {
        *ext = '\0';
    }
    strcat((char *)file_name, ".jpg");
    g_file_name = file_name;
}

void read_bytes_to_buffer(FILE *file, void *buffer, const size_t buffer_capacity){
    assert(buffer_capacity != 0);
    size_t bytes_read = fread(buffer, buffer_capacity, 1, file);
    if (bytes_read != 1) {
        if (ferror(file)) {
            fprintf(stderr, "%sERROR%s: could not read %zu bytes from file: %s\n",
                    ERR_SET, RESET, buffer_capacity, strerror(errno));
            exit(-1);
        } else if (feof(file)) {
            fprintf(stderr, "%sERROR%s: could not read %zu bytes from file: end of file\n",
                    ERR_SET, RESET, buffer_capacity);
            exit(-1);
        } else {
            assert(0 && "unreachable");
        }
    }
}

size_t translate_bytes(uint8_t *buffer, const size_t buffer_capacity){
    uint64_t value = 0;
    for (unsigned int i = 0; i < buffer_capacity; ++i) {
        value |= (uint64_t)buffer[i] << (i * 8);
    }
    return (size_t)value;
}

size_t read_bytes_to_value(FILE *file, const size_t quantity){
    uint8_t bytes[quantity];
    read_bytes_to_buffer(file, bytes, quantity);
    size_t value = translate_bytes(bytes, quantity);
    return value;
}

void print_bytes(uint8_t *buffer, const size_t buffer_capacity){
    for (size_t i = 0; i < buffer_capacity; ++i){
        printf("%u ", buffer[i]);
    }
    printf("\n");
}

void print_date(uint8_t *buffer, const size_t buffer_capacity){
    for (size_t i = 0; i < buffer_capacity; ++i){
        if (buffer[i] < 10 && i != 0 && i != 1){
            printf("0%u", buffer[i]);
        } else {
            printf("%u", buffer[i]);
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

void print_ascii(uint8_t *buffer, const size_t buffer_capacity){
    for (size_t i = 0; i < buffer_capacity; ++i){
        printf("%c", (char)buffer[i]);
    }
    printf("\n");
}

void print_tags(uint8_t *buffer, const size_t buffer_capacity){
    printf("#");
    for (size_t i = 0; i < buffer_capacity; ++i){
        if (buffer[i] == 0 && i != buffer_capacity - 1){
            printf(" #");
        } else { printf("%c", (char)buffer[i]); }
    }
    printf("\n");
}

#define MGC 4
#define CAP 8
#define ANM 8
const uint8_t magic_caff[MGC] = {67, 65, 70, 70};
size_t read_caff_header(FILE *file){
    // MAGIC
    uint8_t magic[MGC];
    read_bytes_to_buffer(file, magic, MGC);
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
    const size_t header_size = read_bytes_to_value(file, CAP);
    assert(header_size == 20);

    // ANIMATIONS
    /* not capacity but still a size value
    and still ocupies 8-bytes */
    const size_t number_of_animations = read_bytes_to_value(file, ANM);
#if LOG
    printf("number of animations: %zu\n", number_of_animations);
#endif
    return number_of_animations;
}

#define DTE 6
void read_caff_credits(FILE *file){
    // DATE
    /* Y - year (2 bytes)
       M - month (1 byte)
       D - day (1 byte)
       h - hour (1 byte)
       m - minute (1 byte2) */
    uint8_t date[DTE];
    read_bytes_to_buffer(file, date, DTE);
#if LOG
    printf("date: ");
    print_date(date, DTE);
#endif
    // CREATOR
    const size_t size_of_creator = read_bytes_to_value(file, CAP);
    if (size_of_creator == 0){
        printf("%sWARNING%s: file does not define the creator\n",
                WARN_SET, RESET);
    } else {
        uint8_t creator[size_of_creator];
        read_bytes_to_buffer(file, creator, size_of_creator);
#if LOG
        printf("creator: ");
        print_ascii(creator, size_of_creator);
#endif
    }
}

void create_jpg(uint8_t *rgb_pixels, size_t width, size_t height){
    int quality = 99;
    int stride_in_bytes = 3 * width;
    int write = stbi_write_jpg(g_file_name, width, height, 3, rgb_pixels, quality);
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
    printf("successfully saved to \"%s\"\n", g_file_name);
#endif
}

#define WDT 8
#define HGT 8
#define ESC 10
const uint8_t magic_ciff[MGC] = {67, 73, 70, 70};
void read_ciff(FILE *file, bool save){
    // MAGIC
    uint8_t magic[MGC];
    read_bytes_to_buffer(file, magic, MGC);
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
    const size_t header_size = read_bytes_to_value(file, CAP);

    /* 8-byte long integer,
    its value is the size of the image
	pixels located at the end of the file.
    Its value must be width*heigth*3 */
    // CONTENT SIZE
    size_t pixel_size = read_bytes_to_value(file, CAP);
    // WIDTH
    size_t width_size = read_bytes_to_value(file, WDT);
    // HEIGHT
    size_t height_size = read_bytes_to_value(file, HGT);

    if (pixel_size != (width_size * height_size * 3)){
        fprintf(stderr,
                "%sERROR%s: pixel size is not equal to size defined in header\n",
                ERR_SET, RESET);
        exit(-1);
    }
    assert(pixel_size == (width_size * height_size * 3));

    // bytes for caption and tags
    const size_t caption_tags_size = header_size - MGC    // 4-byte magic
                                                 - CAP      // 8-byte header size
                                                 - CAP      // 8-byte content size
                                                 - WDT    // 8-byte width of image
                                                 - HGT;  // 8-byte height of image

    // CAPTION
    uint8_t caption_temp[caption_tags_size];
    size_t iter = 0;
    uint8_t buffer[1];
    read_bytes_to_buffer(file, buffer, 1);
    while (buffer[0] != ESC) {
        caption_temp[iter++] = buffer[0];
        read_bytes_to_buffer(file, buffer, 1);
        if (iter == caption_tags_size){
            fprintf(stderr,
                    "%sERROR%s: file caption larger than what header defines\n",
                    ERR_SET, RESET);
            exit(-1);
        }
    }
    // escaped caption
    size_t caption_size = iter + 1;
    if (iter == 0){
        printf("%sWARNING%s: file does not define the caption\n",
                WARN_SET, RESET);
    } else {
        uint8_t caption[caption_size];
        // copy data from temp caption
        for (unsigned int i = 0; i < caption_size; ++i){
            caption[i] = caption_temp[i];
        }
#if LOG
        printf("caption: ");
        print_ascii(caption, caption_size);
#endif
    }

    // TAGS
    size_t tags_size = caption_tags_size - caption_size;
    if (tags_size == 0){
        printf("%sWARNING%s: file does not include any tags\n",
                WARN_SET, RESET);
    } else {
        uint8_t tags[tags_size];
        read_bytes_to_buffer(file, tags, tags_size);
        for (size_t i = 0; i < tags_size; ++i){
            if (tags[i] == ESC){
                fprintf(stderr,
                        "%sERROR%s: file contains escape ASCII in tags\n",
                        ERR_SET, RESET);
                exit(-1);
            }
        }
#if LOG
        printf("tags: ");
        print_tags(tags, tags_size);
#endif
    }

    // PIXELS
    if (pixel_size == 0){
        printf("%sWARNING%s: file is missing the pixel data\n",
                WARN_SET, RESET);
    } else {
        uint8_t pixels[pixel_size];
        read_bytes_to_buffer(file, pixels, pixel_size);
        if (save){ create_jpg(pixels, width_size, height_size); }
    }
}

#define DUR 8
void read_caff_animation(FILE *file, bool save){
    // DURATION
    size_t duration = read_bytes_to_value(file, DUR);
# if LOG
    printf("duration: %zu\n", duration);
#endif
    // CIFF
    read_ciff(file, save);
}

#define ID 1
#define SZ 8
void read_caff(FILE *file){
    // HEADER
    size_t header_id = read_bytes_to_value(file, ID);
    if (header_id != 1){
        fprintf(stderr,
                "%sERROR%s: file does not start with a header\n",
                ERR_SET, RESET);
        exit(-1);
    }
    assert(header_id == 1);

    /* cap is no needed for hdr
    because all chunks lengths
    are predefined in the format */
    const size_t header_size = read_bytes_to_value(file, SZ);
    assert(header_size == 20);

    /* the only valuable information
    in the header block is the
    number of animations in the CAFF */
    const size_t number_of_animations = read_caff_header(file);
#if LOG
    printf("\n");
#endif

    // ANIMATION + CREDITS blocks
    /* read all blocks from file
    + 1 for the credits block */
    bool save_first = true;
    for (size_t i = 0; i < number_of_animations + 1; ++i){
        size_t block_id = read_bytes_to_value(file, ID);
        size_t block_size = read_bytes_to_value(file, SZ);
        if (block_id == 2){
            // CREDITS
            read_caff_credits(file);
#if LOG
            printf("\n");
#endif
        } else if (block_id == 3){
            // ANIMATION
            read_caff_animation(file, save_first);
            save_first = false;
#if LOG
            printf("\n");
#endif
        } else {
            fprintf(stderr,
                    "%sERROR%s: file has unknown id in a block: %zu\n",
                    ERR_SET, RESET, block_id);
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

    set_g_file_name(file_path);
    if (g_file_name == NULL){
        fprintf(stderr,
            "%sERROR%s: filename was not provided\n", ERR_SET, RESET);
        usage(stderr, program);
        exit(-1);
    }
    assert(g_file_name != NULL);

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
        read_caff(file);
    } else if (strcmp(flag, "-ciff") == 0){
        read_ciff(file, true);
    }

#endif

    return 0;
}

// TODO: check for file size, so no extra information in file
