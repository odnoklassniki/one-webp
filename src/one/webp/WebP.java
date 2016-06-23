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

package one.webp;

public class WebP {

    public static int convert(byte[] src, byte[] dst, Params params) throws WebPException {
        return convert0(src, dst, params.longValue());
    }

    public static byte[] convert(byte[] src, Params params) throws WebPException {
        return convert1(src, params.longValue());
    }

    private static native int convert0(byte[] src, byte[] dst, long options);
    private static native byte[] convert1(byte[] src, long options);

    static {
        System.loadLibrary("onewebp");
    }
}
