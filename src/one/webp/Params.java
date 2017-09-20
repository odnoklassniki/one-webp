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

public class Params {
    private byte quality = 80;
    private byte compression = 5;
    private short maxWidth;
    private short maxHeight;
    private boolean useJpegScaling;
    private boolean lossless;
    private boolean multithreaded;
    private boolean png;
    private boolean jpeg;

    public Params quality(int quality) {
        check(quality >= 0 && quality <= 100);
        this.quality = (byte) quality;
        return this;
    }

    public Params compression(int compression) {
        check(compression >= 1 && compression <= 6);
        this.compression = (byte) compression;
        return this;
    }

    public Params maxWidth(int maxWidth) {
        check(maxWidth >= 1 && maxWidth <= 16383);
        this.maxWidth = (short) maxWidth;
        return this;
    }

    public Params maxHeight(int maxHeight) {
        check(maxHeight >= 1 && maxHeight <= 16383);
        this.maxHeight = (short) maxHeight;
        return this;
    }

    public Params useJpegScaling(boolean useJpegScaling) {
        this.useJpegScaling = useJpegScaling;
        return this;
    }

    public Params lossless(boolean lossless) {
        this.lossless = lossless;
        return this;
    }

    public Params multithreaded(boolean multithreaded) {
        this.multithreaded = multithreaded;
        return this;
    }

    public Params png(boolean png) {
        this.png = png;
        return this;
    }

    public Params jpeg(boolean jpeg) {
        this.jpeg = jpeg;
        return this;
    }

    public int quality() {
        return quality;
    }

    public int compression() {
        return compression;
    }

    public int maxWidth() {
        return maxWidth;
    }

    public int maxHeight() {
        return maxHeight;
    }

    public boolean useJpegScaling() {
        return useJpegScaling;
    }

    public boolean lossless() {
        return lossless;
    }

    public boolean multithreaded() {
        return multithreaded;
    }

    public boolean png() {
        return png;
    }

    public boolean jpeg() {
        return jpeg;
    }

    // See struct Params in onewebp.h
    long longValue() {
        return (long) maxWidth
                | (long) maxHeight << 16
                | (long) quality << 32
                | (long) compression << 40
                | (useJpegScaling ? 1L << 48 : 0)
                | (lossless ? 1L << 49 : 0)
                | (multithreaded ? 1L << 50 : 0)
                | (png ? 1L << 51 : 0)
                | (jpeg ? 1L << 52 : 0);
    }

    private void check(boolean condition) {
        if (!condition) {
            throw new IllegalArgumentException();
        }
    }
}
