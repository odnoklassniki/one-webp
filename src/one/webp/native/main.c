/*
 * Copyright 2016 Odnoklassniki Ltd, Mail.Ru Group
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "onewebp.h"


#if _MSC_VER >= 1900
FILE* __cdecl __iob_func() {
    return NULL;
}
#endif

#ifndef MAX_WEBP_OUTPUT
#define MAX_WEBP_OUTPUT (16 * 1024 * 1024)
#endif

static unsigned char dst[MAX_WEBP_OUTPUT];


static int parse_params(int argc, char** argv, Params* params) {
    int i = 1;
    while (i < argc - 2) {
        char* arg = argv[i++];
        if (strcmp(arg, "-q") == 0) {
            params->quality = atoi(argv[i++]);
        } else if (strcmp(arg, "-c") == 0) {
            params->compression = atoi(argv[i++]);
        } else if (strcmp(arg, "-w") == 0) {
            params->maxWidth = atoi(argv[i++]);
        } else if (strcmp(arg, "-h") == 0) {
            params->maxHeight = atoi(argv[i++]);
        } else if (strcmp(arg, "-j") == 0) {
            params->useJpegScaling = 1;
        } else if (strcmp(arg, "-l") == 0) {
            params->lossless = 1;
        } else if (strcmp(arg, "-mt") == 0) {
            params->multithreaded = 1;
        } else if (strcmp(arg, "-png") == 0) {
            params->png = 1;
        } else if (strcmp(arg, "-jpeg") == 0) {
            params->jpeg = 1;
        }
    }
    return i;
}

static void show_usage() {
    printf("Usage: onewebp [options] <input.jpg|png|webp> <output.webp|png>\n"
           "  -q 0..100 : Output quality\n"
           "  -c 1..6   : Compression level\n"
           "  -w px     : Max width\n"
           "  -h px     : Max height\n"
           "  -j        : Use JPEG scaling\n"
           "  -l        : Lossless compression\n"
           "  -mt       : Multithreaded encoding\n"
           "  -png      : PNG output\n"
           "  -jpeg     : JPEG output\n"
           "\n");
}

static int read_file(const char* fileName, unsigned char** buf, unsigned long* size) {
    FILE* f = fopen(fileName, "rb");
    if (f == NULL) {
        return 1;
    }

    fseek(f, 0L, SEEK_END);
    *size = ftell(f);
    fseek(f, 0L, SEEK_SET);

    *buf = (unsigned char*)malloc(*size);
    if (fread(*buf, 1, *size, f) != *size) {
        free(*buf);
        fclose(f);
        return 1;
    }

    fclose(f);
    return 0;
}

static int write_file(const char* fileName, unsigned char* buf, unsigned long size) {
    FILE* f = fopen(fileName, "wb");
    if (f == NULL) {
        return 1;
    }

    if (fwrite(buf, 1, size, f) != size) {
        fclose(f);
        return 1;
    }

    fclose(f);
    return 0;
}


int main(int argc, char** argv) {
    Params params = { 0, 0, 80, 5, 0 };
    int paramCount = parse_params(argc, argv, &params);
    if (paramCount + 2 != argc) {
        show_usage();
        return 1;
    }

    const char* inFile = argv[paramCount];
    const char* outFile = argv[paramCount + 1];
    unsigned char* src;
    unsigned long srcSize;

    if (read_file(inFile, &src, &srcSize)) {
        printf("Error reading %s\n", inFile);
        return 1;
    }

    int result = convert_image(src, srcSize, dst, sizeof(dst), params);
    if (result < 0) {
        printf("Error (%d) converting %s\n", result, inFile);
        free(src);
        return 1;
    }

    if (write_file(outFile, dst, result)) {
        printf("Error writing %s\n", outFile);
        free(src);
        return 1;
    }

    printf("Wrote %d bytes to %s\n", result, outFile);
    free(src);
    return 0;
}
