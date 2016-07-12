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

#include <jni.h>
#include <stdint.h>
#include "onewebp.h"


#ifndef MAX_ON_STACK_OUTPUT
#define MAX_ON_STACK_OUTPUT 64000
#endif


static const char* exceptionMessage(int code) {
    switch (code) {
        case ERR_FORMAT:     return "ERR_FORMAT";
        case ERR_DECOMPRESS: return "ERR_DECOMPRESS";
        case ERR_TRANSFORM:  return "ERR_TRANSFORM";
        case ERR_COMPRESS:   return "ERR_COMPRESS";
        default:             return "ERR_UNKNOWN";
    }
}

static void throwException(JNIEnv* env, int code) {
    jclass cls = (*env)->FindClass(env, "one/webp/WebPException");
    if (cls != NULL) {
        (*env)->ThrowNew(env, cls, exceptionMessage(code));
    }
}

static Params paramsFromJava(jlong options) {
    union {
        unsigned int fields[2];
        Params params;
    } u;

    u.fields[0] = (unsigned int)options;
    u.fields[1] = (unsigned int)(options >> 32);
    return u.params;
}


JNIEXPORT jint JNICALL
Java_one_webp_WebP_convert0(JNIEnv* env, jobject cls, jbyteArray srcArray, jbyteArray dstArray, jlong options) {
    jint srcSize = (*env)->GetArrayLength(env, srcArray);
    jbyte* src = (*env)->GetPrimitiveArrayCritical(env, srcArray, NULL);
    jint dstSize = (*env)->GetArrayLength(env, dstArray);
    jbyte* dst = (*env)->GetPrimitiveArrayCritical(env, dstArray, NULL);

    Params params = paramsFromJava(options);
    int result = convert_to_webp(src, srcSize, dst, dstSize, params);

    (*env)->ReleasePrimitiveArrayCritical(env, srcArray, src, JNI_ABORT);
    (*env)->ReleasePrimitiveArrayCritical(env, dstArray, dst, JNI_COMMIT);

    if (result < 0) {
        throwException(env, result);
    }
    return result;
}

JNIEXPORT jbyteArray JNICALL
Java_one_webp_WebP_convert1(JNIEnv* env, jobject cls, jbyteArray srcArray, jlong options) {
    jint srcSize = (*env)->GetArrayLength(env, srcArray);
    jbyte* src = (*env)->GetPrimitiveArrayCritical(env, srcArray, NULL);
    jbyte dst[MAX_ON_STACK_OUTPUT];

    Params params = paramsFromJava(options);
    int result = convert_to_webp(src, srcSize, dst, sizeof(dst), params);

    (*env)->ReleasePrimitiveArrayCritical(env, srcArray, src, JNI_ABORT);

    if (result < 0) {
        throwException(env, result);
        return NULL;
    } else {
        jbyteArray array = (*env)->NewByteArray(env, result);
        if (array != NULL) {
            (*env)->SetByteArrayRegion(env, array, 0, result, dst);
        }
        return array;
    }
}

JNIEXPORT jlong JNICALL
Java_one_webp_WebP_phash0(JNIEnv* env, jobject cls, jbyteArray srcArray) {
    jint srcSize = (*env)->GetArrayLength(env, srcArray);
    jbyte* src = (*env)->GetPrimitiveArrayCritical(env, srcArray, NULL);

    jlong result = image_phash(src, srcSize);

    (*env)->ReleasePrimitiveArrayCritical(env, srcArray, src, JNI_ABORT);
    return result;
}
