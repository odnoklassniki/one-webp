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

import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.Arrays;

import static java.nio.file.StandardOpenOption.*;

public class Converter {

    private static int parseParams(String[] args, Params params) {
        int i = 0;
        while (i < args.length - 2) {
            switch (args[i++]) {
                case "-q":
                    params.quality(Integer.parseInt(args[i++]));
                    break;
                case "-c":
                    params.compression(Integer.parseInt(args[i++]));
                    break;
                case "-w":
                    params.maxWidth(Integer.parseInt(args[i++]));
                    break;
                case "-h":
                    params.maxHeight(Integer.parseInt(args[i++]));
                    break;
                case "-j":
                    params.useJpegScaling(true);
                    break;
                case "-l":
                    params.lossless(true);
                    break;
                case "-mt":
                    params.multithreaded(true);
                    break;
                case "-png":
                    params.png(true);
                    break;
                case "-jpeg":
                    params.jpeg(true);
                    break;
            }
        }
        return i;
    }

    public static void main(String[] args) throws Exception {
        Params params = new Params();
        int paramCount = parseParams(args, params);
        if (paramCount + 2 != args.length) {
            throw new IllegalArgumentException("[options] <input.jpg|png|webp> <output.webp|png>");
        }

        String inFile = args[paramCount];
        String outFile = args[paramCount + 1];

        byte[] src = Files.readAllBytes(Paths.get(inFile));
        byte[] dst = new byte[4 * 1024 * 1024];
        int dstSize = WebP.convert(src, dst, params);
        Files.write(Paths.get(outFile), Arrays.copyOf(dst, dstSize), CREATE, TRUNCATE_EXISTING);
    }
}
