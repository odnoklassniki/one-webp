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

#define _USE_MATH_DEFINES

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "turbojpeg.h"
#include "png.h"
#include "webp/encode.h"
#include "onewebp.h"


#define HASH_SIZE 64

typedef struct {
    unsigned char* ptr;
    const unsigned char* limit;
} Buffer;


static int jpeg_scaling_factor(int width, int height, int maxWidth, int maxHeight) {
    int scaleWidth = maxWidth ? width / maxWidth : 16;
    int scaleHeight = maxHeight ? height / maxHeight : 16;
    int scale = scaleWidth < scaleHeight ? scaleWidth : scaleHeight;

    if (scale >= 12) return 1;  // 1/8
    if (scale >= 6)  return 2;  // 1/4
    if (scale >= 4)  return 3;  // 3/8
    if (scale >= 3)  return 4;  // 1/2
    if (scale >= 2)  return 6;  // 3/4
    return 0;
}

static int webp_writer(const uint8_t* data, size_t dataSize, const WebPPicture* picture) {
    Buffer* buffer = (Buffer*)picture->custom_ptr;
    if (buffer->ptr + dataSize > buffer->limit) {
        return 0;
    }

    memcpy(buffer->ptr, data, dataSize);
    buffer->ptr += dataSize;
    return 1;
}


static int decompress_jpeg(unsigned char* src, unsigned long srcSize, RawImage* rawImage, Params params) {
    tjhandle handle = tjInitDecompress();

    int width, height, subsamp, colorspace;
    if (tjDecompressHeader3(handle, src, srcSize, &width, &height, &subsamp, &colorspace)) {
        tjDestroy(handle);
        return ERR_FORMAT;
    }

    if (params.useJpegScaling && (params.maxWidth || params.maxHeight)) {
        int scale = jpeg_scaling_factor(width, height, params.maxWidth, params.maxHeight);
        if (scale) {
            width = (width * scale + 7) / 8;
            height = (height * scale + 7) / 8;
        }
    }
    
    rawImage->width = width;
    rawImage->height = height;
    rawImage->argb = (unsigned char*)malloc(width * height * 4);

    if (tjDecompress2(handle, src, srcSize, rawImage->argb, width, 0, height, TJPF_BGRX, 0)) {
        free(rawImage->argb);
        tjDestroy(handle);
        return ERR_DECOMPRESS;
    }

    tjDestroy(handle);
    return 0;
}

static int decompress_png(unsigned char* src, unsigned long srcSize, RawImage* rawImage, Params params) {
    png_image png = { NULL };
    png.version = PNG_IMAGE_VERSION;

    if (!png_image_begin_read_from_memory(&png, src, srcSize)) {
        return ERR_FORMAT;
    }

    png.format = PNG_FORMAT_BGRA;
    rawImage->width = png.width;
    rawImage->height = png.height;
    rawImage->argb = (unsigned char*)malloc(PNG_IMAGE_SIZE(png));

    if (!png_image_finish_read(&png, NULL, rawImage->argb, 0, NULL)) {
        free(rawImage->argb);
        return ERR_DECOMPRESS;
    }

    return 0;
}

int decompress_image(unsigned char* src, unsigned long srcSize, RawImage* rawImage, Params params) {
    if (srcSize >= 4 && src[1] == 'P' && src[2] == 'N' && src[3] == 'G') {
        return decompress_png(src, srcSize, rawImage, params);
    } else {
        return decompress_jpeg(src, srcSize, rawImage, params);
    }
}

int compress_webp(unsigned char* dst, unsigned long dstSize, RawImage* rawImage, Params params) {
    Buffer buffer = { dst, dst + dstSize };
    int maxWidth = params.maxWidth ? params.maxWidth : WEBP_MAX_DIMENSION;
    int maxHeight = params.maxHeight ? params.maxHeight : WEBP_MAX_DIMENSION;

    WebPConfig config;
    WebPConfigInit(&config);
    config.quality = (float)params.quality;
    config.method = params.compression;
    config.lossless = params.lossless;
    config.thread_level = params.multithreaded;

    WebPPicture picture;
    WebPPictureInit(&picture);
    picture.use_argb = 1;
    picture.colorspace = WEBP_YUV420;
    picture.width = rawImage->width;
    picture.height = rawImage->height;
    picture.argb = (uint32_t*)rawImage->argb;
    picture.argb_stride = rawImage->width;
    picture.writer = webp_writer;
    picture.custom_ptr = &buffer;

    if (picture.width > maxWidth || picture.height > maxHeight) {
        if ((float)picture.width / maxWidth > (float)picture.height / maxHeight) {
            maxHeight = 0;
        } else {
            maxWidth = 0;
        }
        if (!WebPPictureRescale(&picture, maxWidth, maxHeight)) {
            WebPPictureFree(&picture);
            return ERR_TRANSFORM;
        }
    }

    if (!WebPEncode(&config, &picture)) {
        WebPPictureFree(&picture);
        return ERR_COMPRESS;
    }

    WebPPictureFree(&picture);
    return (int)(buffer.ptr - dst);
}

int convert_to_webp(unsigned char* src, unsigned long srcSize,
                    unsigned char* dst, unsigned long dstSize,
                    Params params) {
    RawImage rawImage;
    int result = decompress_image(src, srcSize, &rawImage, params);

    if (result == 0) {
        result = compress_webp(dst, dstSize, &rawImage, params);
        free(rawImage.argb);
    }

    return result;
}

static void argb_to_grayscale(unsigned int* src, int stride, float* grayscale) {
    int i, j;
    for (i = 0; i < HASH_SIZE; i++) {
        for (j = 0; j < HASH_SIZE; j++) {
            unsigned int argb = src[i * stride + j];
            unsigned int r = (argb >> 16) & 0xff;
            unsigned int g = (argb >> 8) & 0xff;
            unsigned int b = (argb) & 0xff;
            grayscale[i * HASH_SIZE + j] = r * 0.299f + g * 0.587f + b * 0.114f;
        }
    }
}

static void dct_vector(float* vector) {
    float transformed[HASH_SIZE];
    int i, j;
    for (i = 0; i < HASH_SIZE; i++) {
        float sum = 0;
        for (j = 0; j < HASH_SIZE; j++) {
            sum += vector[j] * cos(i * M_PI * (j + 0.5) / HASH_SIZE);
        }
        sum *= sqrt(2.0f / HASH_SIZE);
        transformed[i] = (i == 0) ? sum * M_SQRT1_2 : sum;
    }
    memcpy(vector, transformed, sizeof(transformed));
}

static void dct_8x8(float* pixels, float* transformed) {
    int i, j;
    for (i = 0; i < HASH_SIZE; i++) {
        float* row = &pixels[i * HASH_SIZE];
        dct_vector(row);
    }

    for (i = 0; i < 8; i++) {
        float col[HASH_SIZE];
        for (j = 0; j < HASH_SIZE; j++) {
            col[j] = pixels[j * HASH_SIZE + i];
        }
        dct_vector(col);
        for (j = 0; j < 8; j++) {
            transformed[i * 8 + j] = col[j];
        }
    }
}

static float median(float* pixels) {
    float sum = 0;
    int i;
    for (i = 0; i < 64; i++) {
        sum += pixels[i];
    }
    return sum / 64;
}

static unsigned long long bitmask(float* pixels, float median) {
    unsigned long long mask = 0;
    int i;
    for (i = 0; i < 64; i++) {
        if (pixels[i] > median) {
            mask |= 1ULL << i;
        }
    }
    return mask;
}

unsigned long long image_phash(unsigned char* src, unsigned long srcSize) {
    RawImage rawImage;
    Params params = {HASH_SIZE, HASH_SIZE, 0, 0, 1, 0, 0};
    if (decompress_image(src, srcSize, &rawImage, params)) {
        return 0;
    }

    WebPPicture picture;
    WebPPictureInit(&picture);
    picture.use_argb = 1;
    picture.width = rawImage.width;
    picture.height = rawImage.height;
    picture.argb = (uint32_t*)rawImage.argb;
    picture.argb_stride = rawImage.width;

    if (!WebPPictureRescale(&picture, HASH_SIZE, HASH_SIZE)) {
        WebPPictureFree(&picture);
        free(rawImage.argb);
        return 0;
    }

    float grayscale[HASH_SIZE * HASH_SIZE];
    argb_to_grayscale(picture.argb, picture.argb_stride, grayscale);
    WebPPictureFree(&picture);
    free(rawImage.argb);

    float transformed[64];
    dct_8x8(grayscale, transformed);

    float m = median(transformed);
    return bitmask(transformed, m);
}
