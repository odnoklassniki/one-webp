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

#include <stdlib.h>
#include <string.h>
#include "turbojpeg.h"
#include "png.h"
#include "webp/encode.h"
#include "onewebp.h"


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


int decompress_jpeg(unsigned char* src, unsigned long srcSize, RawImage* rawImage, Params params) {
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

int decompress_png(unsigned char* src, unsigned long srcSize, RawImage* rawImage, Params params) {
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

int compress_webp(unsigned char* dst, unsigned long dstSize, RawImage* rawImage, Params params) {
    WebPConfig config;
    WebPPicture picture;
    Buffer buffer = { dst, dst + dstSize };
    int maxWidth = params.maxWidth ? params.maxWidth : WEBP_MAX_DIMENSION;
    int maxHeight = params.maxHeight ? params.maxHeight : WEBP_MAX_DIMENSION;

    WebPConfigInit(&config);
    config.quality = (float)params.quality;
    config.method = params.compression;
    config.lossless = params.lossless;
    config.thread_level = params.multithreaded;

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
            return ERR_TRANSFORM;
        }
    }

    if (!WebPEncode(&config, &picture)) {
        return ERR_COMPRESS;
    }

    return (int)(buffer.ptr - dst);
}

int convert_to_webp(unsigned char* src, unsigned long srcSize,
                    unsigned char* dst, unsigned long dstSize,
                    Params params) {
    RawImage rawImage;
    int result;

    if (srcSize >= 4 && src[1] == 'P' && src[2] == 'N' && src[3] == 'G') {
        result = decompress_png(src, srcSize, &rawImage, params);
    } else {
        result = decompress_jpeg(src, srcSize, &rawImage, params);
    }

    if (result == 0) {
        result = compress_webp(dst, dstSize, &rawImage, params);
        free(rawImage.argb);
    }

    return result;
}
