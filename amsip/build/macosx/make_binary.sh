#!/bin/sh

mkdir -pv vbyantisip-macosx.app/Contents/include/amsip
cp -p ../../amsip/include/amsip/*.h vbyantisip-macosx.app/Contents/include/amsip/
mkdir -pv vbyantisip-macosx.app/Contents/include/amsiptools
cp -p ../../amsiptools/include/amsiptools/*.h vbyantisip-macosx.app/Contents/include/amsiptools/
mkdir -pv vbyantisip-macosx.app/Contents/include/eXosip2
cp -p ../../exosip/include/eXosip2/*.h vbyantisip-macosx.app/Contents/include/eXosip2/

mkdir -pv vbyantisip-macosx.app/Contents/include/osip2
cp -p ../../osip/include/osip2/*.h vbyantisip-macosx.app/Contents/include/osip2/

mkdir -pv vbyantisip-macosx.app/Contents/include/osipparser2/headers
cp -p ../../osip/include/osipparser2/*.h vbyantisip-macosx.app/Contents/include/osipparser2/
cp -p ../../osip/include/osipparser2/headers/*.h vbyantisip-macosx.app/Contents/include/osipparser2/headers

mkdir -pv vbyantisip-macosx.app/Contents/include/ortp
cp -p ../../oRTP/include/ortp/*.h vbyantisip-macosx.app/Contents/include/ortp/

mkdir -pv vbyantisip-macosx.app/Contents/include/mediastreamer2
cp -p ../../mediastreamer2/include/mediastreamer2/*.h vbyantisip-macosx.app/Contents/include/mediastreamer2/

mkdir -pv vbyantisip-macosx.app/Contents/include/srtp
cp -p ../../win32-bin/include/srtp/*.h vbyantisip-macosx.app/Contents/include/srtp/

mkdir -pv vbyantisip-macosx.app/Contents/include/speex
cp -p ../../codecs/speex/include/speex/*.h vbyantisip-macosx.app/Contents/include/speex
