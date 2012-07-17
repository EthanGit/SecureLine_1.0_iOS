#!/bin/sh

install_name_tool -id @rpath/libimf.dylib libimf.dylib
install_name_tool -change libintlc.dylib @rpath/libintlc.dylib libimf.dylib

install_name_tool -id @rpath/libirc.dylib libirc.dylib
install_name_tool -id @rpath/libintlc.dylib libintlc.dylib
install_name_tool -id @rpath/libiomp5.dylib libiomp5.dylib

