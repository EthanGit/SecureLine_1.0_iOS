#!/bin/sh

mkdir -p armv6_build 
cd armv6_build/ 
../libvpx/configure --target=armv6-darwin-gcc 
make 
cd .. 
mkdir -p armv7_build 
cd armv7_build/ 
../libvpx/configure --target=armv7-darwin-gcc 
make 
cd .. 

lipo armv6_build/libvpx.a -arch armv7 armv7_build/libvpx.a -create -output libvpx.a 
mkdir -p ../mediastreamer2/vpx
cp libvpx/vpx/*.h ../mediastreamer2/vpx/
for f in ../mediastreamer2/vpx/*.h; do sed -i '' 's/\#include "vpx\//\#include "/' $f; done
