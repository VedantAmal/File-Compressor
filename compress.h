#ifndef COMPRESS_H
#define COMPRESS_H

#include <stdio.h>

void compress_zlib(FILE *source, FILE *dest);
void decompress_zlib(FILE *source, FILE *dest);
void compress_bz2(FILE *source, FILE *dest);
void decompress_bz2(FILE *source, FILE *dest);
void compress_lzma(FILE *source, FILE *dest);
void decompress_lzma(FILE *source, FILE *dest);
void compress_file(const char *operation, const char *filename);

#endif // COMPRESS_H
