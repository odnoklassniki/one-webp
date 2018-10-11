# one-webp
JNI wrapper to convert JPEG and PNG to WebP

### Mac OS X
Before build native library it's needed to install libpng, libwebp and jpeg-turbo:

Using brew:
```
   brew install libpng
   
   brew install webp
   
   brew install jpeg-turbo
 ```
For build libonewebp.dylib:
```
ant clean webp-dll -Djpeg-turbo.version=2.0.0 -Dlibpng.version=1.6.35 -Dlibwebp.version=1.0.0
```