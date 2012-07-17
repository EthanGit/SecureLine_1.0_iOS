#!/bin/sh

##   *** tested with ****
#
# git clone http://git.chromium.org/webm/libvpx.git
# git checkout 313bfbb6a29bf7c60d9a9e33397ed82a8d11fe2d


mkdir -p x86_build 
cd x86_build/ 
../libvpx/configure --target=x86-darwin10-gcc --enable-realtime-only --enable-pic
make 
cd .. 
mkdir -p x64_build 
cd x64_build/ 
../libvpx/configure --target=x86_64-darwin10-gcc --enable-realtime-only
make 
cd .. 

lipo x86_build/libvpx.a -arch x86_64 x64_build/libvpx.a -create -output libvpx.a 
mkdir -p ../mediastreamer2/vpx
cp libvpx/vpx/*.h ../mediastreamer2/vpx/
for f in ../mediastreamer2/vpx/*.h; do sed -i '' 's/\#include "vpx\//\#include "/' $f; done
