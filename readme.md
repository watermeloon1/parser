# Custom file format parser in C

Parser is an open-source command-line application that can generate a preview in JPG format from CIFF and CAFF format images. The purpose of this project is to apply the learned methods in practice, developing a security-critical software.

## File formats

### CIFF
The CrySyS Image File Format is an uncompressed image format that contains direct pixel information in addition to the metadata related to the image.

### CAFF
The CrySyS Animation File Format is an uncompressed animation format that is suitable for storing CIFF images, in addition to the metadata related to the animated images.

## Requirements

### Compilation
- To compile the source code, a makefile is required that can compile the application without warnings.
- The compiled binary should be named "parser."
- The application must be able to compile on Ubuntu 22.04 LTS.

### Running
- The application should be able to process a file received as a command-line parameter.
- For CAFF images, only the first embedded CIFF image should be converted.
- The output can be in either JPG or WebP format. (The use of external libraries to produce these formats is permitted.)
- The output file name should match the input file name.
- A successful execution should return a value of 0. In the case of failure, the return value should be -1.

## Usage

To build the application I included a [makefile](makefile) so running `make` should do the job. One other command is `make clean` that cleans the working directory from binaries and object files.

To run the application, navigate to the directory containing the application's binary file, and run the following command:

`./parser [option] [path-to-file]`

- `option`: must be either "--caff" or "--ciff".
- `path-to-file`: the path to the image file that needs to be converted.

Example usage:

`./parser --caff /path/to/image.caff`

## Security Testing

It is highly recommended to thoroughly test the application's security as poorly formatted files may pose a security risk.

## Contribution

Contributions to the project are welcome. Please submit your contributions via pull request to the project's GitHub repository.

## License

This software is licensed under the [MIT License](license.md). I used Sean T. Barrett's JPG formatter that was readily available on [github](https://github.com/nothings/stb/tree/master) under the MIT license, check it out if you are interested.
