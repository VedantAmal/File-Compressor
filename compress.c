#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>
#include <bzlib.h>
#include <lzma.h>
#include "compress.h"

#define CHUNK 16384

// Function to compress using zlib
void compress_zlib(FILE *source, FILE *dest) {
    int ret;
    z_stream strm;
    unsigned char in[CHUNK];
    unsigned char out[CHUNK];

    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    ret = deflateInit(&strm, Z_BEST_COMPRESSION);
    if (ret != Z_OK) return;

    do {
        strm.avail_in = fread(in, 1, CHUNK, source);
        if (ferror(source)) break;
        strm.next_in = in;

        do {
            strm.avail_out = CHUNK;
            strm.next_out = out;
            ret = deflate(&strm, Z_FINISH);
            fwrite(out, 1, CHUNK - strm.avail_out, dest);
        } while (strm.avail_out == 0);
    } while (ret != Z_STREAM_END);

    deflateEnd(&strm);
}

// Function to decompress using zlib
void decompress_zlib(FILE *source, FILE *dest) {
    int ret;
    z_stream strm;
    unsigned char in[CHUNK];
    unsigned char out[CHUNK];

    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    ret = inflateInit(&strm);
    if (ret != Z_OK) return;

    do {
        strm.avail_in = fread(in, 1, CHUNK, source);
        if (ferror(source)) break;
        strm.next_in = in;

        do {
            strm.avail_out = CHUNK;
            strm.next_out = out;
            ret = inflate(&strm, Z_NO_FLUSH);
            fwrite(out, 1, CHUNK - strm.avail_out, dest);
        } while (strm.avail_out == 0);
    } while (ret != Z_STREAM_END);

    inflateEnd(&strm);
}

// Function to compress using BZ2
void compress_bz2(FILE *source, FILE *dest) {
    int bzerror;
    BZFILE *bzfile;
    char buffer[CHUNK];
    int n;

    bzfile = BZ2_bzWriteOpen(&bzerror, dest, 9, 0, 0);
    if (bzerror != BZ_OK) return;

    while ((n = fread(buffer, 1, CHUNK, source)) > 0) {
        BZ2_bzWrite(&bzerror, bzfile, buffer, n);
    }

    BZ2_bzWriteClose(&bzerror, bzfile, 0, NULL, NULL);
}

// Function to decompress using BZ2
void decompress_bz2(FILE *source, FILE *dest) {
    int bzerror;
    BZFILE *bzfile;
    char buffer[CHUNK];
    int n;

    bzfile = BZ2_bzReadOpen(&bzerror, source, 0, 0, NULL, 0);
    while (bzerror == BZ_OK) {
        n = BZ2_bzRead(&bzerror, bzfile, buffer, CHUNK);
        fwrite(buffer, 1, n, dest);
    }
    BZ2_bzReadClose(&bzerror, bzfile);
}

// Function to compress using LZMA
void compress_lzma(FILE *source, FILE *dest) {
    unsigned char inbuf[CHUNK];
    unsigned char outbuf[CHUNK];
    size_t in_len, out_len;
    lzma_stream strm = LZMA_STREAM_INIT;
    lzma_ret ret;

    ret = lzma_easy_encoder(&strm, 9, LZMA_CHECK_CRC64);
    if (ret != LZMA_OK) return;

    while ((in_len = fread(inbuf, 1, CHUNK, source)) > 0) {
        strm.next_in = inbuf;
        strm.avail_in = in_len;

        do {
            strm.next_out = outbuf;
            strm.avail_out = CHUNK;
            ret = lzma_code(&strm, LZMA_RUN);
            out_len = CHUNK - strm.avail_out;
            fwrite(outbuf, 1, out_len, dest);
        } while (strm.avail_out == 0);
    }

    do {
        strm.next_out = outbuf;
        strm.avail_out = CHUNK;
        ret = lzma_code(&strm, LZMA_FINISH);
        out_len = CHUNK - strm.avail_out;
        fwrite(outbuf, 1, out_len, dest);
    } while (ret == LZMA_OK);

    lzma_end(&strm);
}

// Function to decompress using LZMA
void decompress_lzma(FILE *source, FILE *dest) {
    unsigned char inbuf[CHUNK];
    unsigned char outbuf[CHUNK];
    size_t in_len, out_len;
    lzma_stream strm = LZMA_STREAM_INIT;
    lzma_ret ret;

    ret = lzma_stream_decoder(&strm, UINT64_MAX, 0);
    if (ret != LZMA_OK) return;

    while ((in_len = fread(inbuf, 1, CHUNK, source)) > 0) {
        strm.next_in = inbuf;
        strm.avail_in = in_len;

        do {
            strm.next_out = outbuf;
            strm.avail_out = CHUNK;
            ret = lzma_code(&strm, LZMA_RUN);
            out_len = CHUNK - strm.avail_out;
            fwrite(outbuf, 1, out_len, dest);
        } while (strm.avail_out == 0);
    }

    lzma_end(&strm);
}

// New function to compress files
void compress_file(const char *operation, const char *filename) {
    FILE *source = fopen(filename, "rb");
    if (!source) {
        printf("Error: Cannot open file %s\n", filename);
        return;
    }

    char temp_filename[] = "temp_XXXXXX";
    char final_filename[256];
    FILE *temp1, *temp2, *dest;

    // Compressed file after zlib
    mkstemp(temp_filename);
    temp1 = fopen(temp_filename, "wb");

    // Combine compressing with zlib, bzip2, and LZMA
    if (strcmp(operation, "compress") == 0) {
        // Step 1: Compress with zlib
        compress_zlib(source, temp1);
        fclose(temp1);

        // Step 2: Compress with BZ2
        sprintf(final_filename, "%s.bz2", filename);
        temp1 = fopen(temp_filename, "rb");
        dest = fopen(final_filename, "wb");
        compress_bz2(temp1, dest);
        fclose(temp1);
        fclose(dest);

        // Step 3: Compress with LZMA
        sprintf(final_filename, "%s.lzma", filename);
        temp1 = fopen(temp_filename, "rb");
        dest = fopen(final_filename, "wb");
        compress_lzma(temp1, dest);
        fclose(temp1);
        fclose(dest);

        printf("File compressed successfully to: %s.lzma\n", filename);
    }
    // Decompressing
    else if (strcmp(operation, "decompress") == 0) {
        // Step 1: Decompress with LZMA
        sprintf(final_filename, "%s.lzma", filename);
        dest = fopen("decompressed_temp.bz2", "wb");
        FILE *lzma_source = fopen(final_filename, "rb");
        decompress_lzma(lzma_source, dest);
        fclose(lzma_source);
        fclose(dest);

        // Step 2: Decompress with BZ2
        temp1 = fopen("decompressed_temp.bz2", "rb");
        sprintf(final_filename, "decompressed_%s", filename);
        dest = fopen(final_filename, "wb");
        decompress_bz2(temp1, dest);
        fclose(temp1);
        fclose(dest);

        // Step 3: Decompress with zlib
        temp1 = fopen(final_filename, "rb");
        dest = fopen("decompressed_final.txt", "wb");
        decompress_zlib(temp1, dest);
        fclose(temp1);
        fclose(dest);

        printf("File decompressed successfully to: decompressed_final.txt\n");
    } else {
        printf("Invalid operation: %s\n", operation);
    }

    // Clean up temporary files
    remove(temp_filename);
}
