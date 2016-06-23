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

enum ErrorCode {
    ERR_FORMAT     = -1,
    ERR_DECOMPRESS = -2,
    ERR_TRANSFORM  = -3,
    ERR_COMPRESS   = -4
};

typedef struct {
    unsigned int maxWidth       : 16;
    unsigned int maxHeight      : 16;
    unsigned int quality        : 8;
    unsigned int compression    : 8;
    unsigned int useJpegScaling : 1;
    unsigned int lossless       : 1;
    unsigned int multithreaded  : 1;
} Params;

typedef struct {
    int width;
    int height;
    unsigned char* argb;
} RawImage;


int decompress_jpeg(unsigned char* src, unsigned long srcSize, RawImage* rawImage, Params params);
int decompress_png(unsigned char* src, unsigned long srcSize, RawImage* rawImage, Params params);
int compress_webp(unsigned char* dst, unsigned long dstSize, RawImage* rawImage, Params params);

int convert_to_webp(unsigned char* src, unsigned long srcSize,
                    unsigned char* dst, unsigned long dstSize,
                    Params params);
