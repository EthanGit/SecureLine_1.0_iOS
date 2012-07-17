#!/bin/bash
trap 0

source amsip_config.sh

pkgconfig=`which pkg-config`

check_amsip_check ()
{
    echo ""
    echo "Compilation on $OS"
    echo -n "amsip: checking for pkg-config: "
    if test "x$pkgconfig" = x; then
	echo "missing"
	exit
    else
	echo "$pkgconfig"
    fi

    echo -n "checking for SDL development package: "
    if PKG_CONFIG_PATH="$AMSIP_PREFIX/lib/pkgconfig" pkg-config --exists sdl; then
    #if PKG_CONFIG_PATH=/usr/local/antisip/lib/pkgconfig pkg-config --exists sdl; then
	echo yes
    else
	echo missing
    fi

    echo -n "checking for OGG development package: "
    if PKG_CONFIG_PATH="$AMSIP_PREFIX/lib/pkgconfig" pkg-config --exists ogg; then
    #if PKG_CONFIG_PATH=/usr/local/antisip/lib/pkgconfig pkg-config --exists ogg; then
	echo yes
    else
	echo missing
    fi
}

check_amsip_config ()
{
    echo ""
    echo -n "amsip: video "
    if test "x$ENABLE_VIDEO" = xyes; then
	echo "enabled"
    else
	echo "disabled"
    fi
    
    echo -n "amsip: SPEEX "
    if test "x$ENABLE_SPEEX" = xyes; then
	echo "enabled"
    else
	echo "disabled"
    fi
    
    echo -n "amsip: gsm "
    if test "x$ENABLE_GSM" = xyes; then
	echo "enabled"
    else
	echo "disabled"
    fi
    
    echo -n "amsip: ilbc "
    if test "x$ENABLE_ILBC" = xyes; then
	echo "enabled"
    else
	echo "disabled"
    fi

}


compile_amsip_all ()
{
   compile_amsip_srtp
   ldconfig
   compile_amsip_ortp
   ldconfig
   if test "x$ENABLE_SPEEX" = xyes; then
       compile_amsip_libspeex
       ldconfig
   fi
   if test "x$ENABLE_GSM" = xyes; then
       compile_amsip_libgsm
       ldconfig
   fi
   if test "x$ENABLE_VIDEO" = xyes; then
       compile_amsip_libtheora
       ldconfig
   fi
   if test "x$ENABLE_VIDEO" = xyes; then
       compile_amsip_libffmpeg
       ldconfig
   fi
   compile_amsip_mediastreamer2
   ldconfig
   #if test "x$ENABLE_ILBC" = xyes; then
   #    compile_amsip_msilbc
   #    ldconfig
   #fi
   if test "x$ENABLE_VIDEO" = xyes; then
       compile_amsip_msvideostitcher
       ldconfig
   fi
   compile_amsip_libosip2
   ldconfig
   compile_amsip_libeXosip2
   ldconfig
   compile_amsip_libamsip
   ldconfig
}

compile_amsip_ortp ()
{
    echo -n "amsip: compiling ortp "
    pushd oRTP > /dev/null 2>&1

    if test ! -f config.log; then
	set -x
        ./configure CPPFLAGS="$ORTP_CPPFLAGS" CFLAGS="$ORTP_CFLAGS" LIBS="$ORTP_LIBS" $ORTP_OPTIONS --prefix=$AMSIP_PREFIX > /dev/null 2>&1
	set +x
    fi
    if test ! -f config.log; then
	echo "FAILED"
	exit
    fi

    make > /dev/null 2>&1
    if test ! -f src/.libs/libortp.a; then
	echo "FAILED"
	exit
    fi
    make install > /dev/null 2>&1 || exit
    popd > /dev/null 2>&1
    echo "DONE"
}

compile_amsip_srtp ()
{
    echo -n "amsip: compiling srtp "
    pushd srtp > /dev/null 2>&1

    if test ! -f config.log; then
	set -x
        ./configure CPPFLAGS="$SRTP_CPPFLAGS"  CFLAGS="$SRTP_CFLAGS" LIBS="$SRTP_LIBS" $SRTP_OPTIONS --prefix=$AMSIP_PREFIX > /dev/null 2>&1
	set +x
    fi
    if test ! -f config.log; then
	echo "FAILED"
	exit
    fi

    make > /dev/null 2>&1
    if test ! -f libsrtp.a; then
	echo "FAILED"
	exit
    fi
    make uninstall > /dev/null 2>&1
    make install > /dev/null 2>&1 || exit
    popd > /dev/null 2>&1
    echo "DONE"
}

compile_amsip_libspeex ()
{
    echo -n "amsip: compiling SPEEX "
    pushd codecs/speex > /dev/null 2>&1

    if test ! -f config.log; then
	set -x
        ./configure CPPFLAGS="$LIBSPEEX_CPPFLAGS"  CFLAGS="$LIBSPEEX_CFLAGS" LIBS="$LIBSPEEX_LIBS" $LIBSPEEX_OPTIONS --prefix=$AMSIP_PREFIX > /dev/null 2>&1
	set +x
    fi
    if test ! -f config.log; then
	echo "FAILED"
	exit
    fi

    make > /dev/null 2>&1
    if test ! -f libspeex/.libs/libspeex.a; then
        echo "FAILED"
        exit
    fi
    make install > /dev/null 2>&1
    popd > /dev/null 2>&1
    echo "DONE"
}

compile_amsip_libgsm ()
{
    echo -n "amsip: compiling GSM "
    pushd codecs/gsm > /dev/null 2>&1

    if test ! -f config.log; then
	set -x
        ./configure $LIBGSM_OPTIONS --prefix=$AMSIP_PREFIX > /dev/null 2>&1
	set +x
	touch config.log
    fi
    if test ! -f config.log; then
	echo "FAILED"
	exit
    fi

    make > /dev/null 2>&1
    if test ! -f lib/libgsm.a; then
	echo "FAILED"
	exit
    fi
    make install > /dev/null 2>&1
    popd > /dev/null 2>&1
    echo "DONE"
}

compile_amsip_msilbc ()
{
    echo -n "amsip: compiling ILBC "
    pushd plugins/msilbc/ilbc-rfc3951 > /dev/null 2>&1

    if test ! -f config.log; then
	set -x
        ./configure CPPFLAGS="$LIBILBC_CPPFLAGS"  CFLAGS="$LIBILBC_CFLAGS" LIBS="$LIBILBC_LIBS" $LIBILBC_OPTIONS --prefix=$AMSIP_PREFIX > /dev/null 2>&1
	set +x
    fi
    if test ! -f config.log; then
	echo "FAILED"
	exit
    fi
    
    make > /dev/null 2>&1
    if test ! -f ilbc/.libs/libilbc.a; then
	echo "FAILED"
	exit
    fi
    make install > /dev/null 2>&1
    popd > /dev/null 2>&1
    echo "DONE"
    echo -n "amsip: compiling ILBC plugin "
    pushd plugins/msilbc > /dev/null 2>&1
    make > /dev/null 2>&1
    if test ! -f libmsilbc.so; then
	echo "FAILED"
    exit
    fi
    make install > /dev/null 2>&1
    popd > /dev/null 2>&1
    echo "DONE"
}

compile_amsip_msvideostitcher ()
{
    echo -n "amsip: compiling msvideostitcher plugin "
    pushd plugins/msvideostitcher > /dev/null 2>&1

    if test ! -f configure; then
	./autogen.sh
    fi

    if test ! -f config.log; then
	set -x
        ./configure CPPFLAGS="$MSVIDEOSTITCHERPLUGIN_CPPFLAGS"  CFLAGS="$MSVIDEOSTITCHERPLUGIN_CFLAGS" LIBS="$MSVIDEOSTITCHERPLUGIN_LIBS" $MSVIDEOSTITCHERPLUGIN_OPTIONS $MSVIDEOSTITCHERPLUGIN_OPTIONS --prefix=$AMSIP_PREFIX > /dev/null 2>&1
	set +x
    fi
    if test ! -f config.log; then
	echo "FAILED"
	exit
    fi
    
    make > /dev/null 2>&1
    if test ! -f src/.libs/libmsvideostitcher.a; then
	echo "FAILED"
	exit
    fi
    make install > /dev/null 2>&1
    popd > /dev/null 2>&1
    echo "DONE"
}

compile_amsip_libtheora ()
{
    echo -n "amsip: compiling THEORA "
    pushd codecs/libtheora > /dev/null 2>&1

    if test ! -f config.log; then
	set -x
        ./configure CPPFLAGS="$LIBTHEORA_CPPFLAGS" CFLAGS="$LIBTHEORA_CFLAGS" LIBS="$LIBTHEORA_LIBS" $LIBTHEORA_OPTIONS --prefix=$AMSIP_PREFIX > /dev/null 2>&1
	set +x
    fi
    if test ! -f config.log; then
	echo "FAILED"
	exit
    fi
        
    make > /dev/null 2>&1
    if test ! -f lib/.libs/libtheora.a; then
	echo "FAILED"
	exit
    fi
    make install > /dev/null 2>&1
    popd > /dev/null 2>&1
    echo "DONE"
}


compile_amsip_libffmpeg ()
{
    echo -n "amsip: compiling FFMPEG "
    pushd codecs/ffmpeg > /dev/null 2>&1

    set -x
    ./configure --extra-cflags="$LIBFFMPEG_CFLAGS" --extra-libs="$LIBFFMPEG_LIBS" $LIBFFMPEG_OPTIONS --prefix=$AMSIP_PREFIX > /dev/null 2>&1
    set +x

    make > /dev/null 2>&1
    if test ! -f libavformat/libavformat.a; then
	echo "FAILED"
	exit
    fi
    make install > /dev/null 2>&1
    popd > /dev/null 2>&1
    echo "DONE"
}

compile_amsip_mediastreamer2 ()
{
    echo -n "amsip: compiling mediastreamer2 "
    pushd mediastreamer2 > /dev/null 2>&1

    if test "x$OS" = xDarwin; then
	LIBMEDIASTREAMER2_OPTIONS="$LIBMEDIASTREAMER2_OPTIONS --enable-macaqsnd --disable-x11"
	LIBMEDIASTREAMER2_CFLAGS="-L$AMSIP_PREFIX/lib $LIBMEDIASTREAMER2_CFLAGS -ObjC -framework Cocoa"
    fi

    if test ! -f config.log; then
	set -x
	./configure CPPFLAGS="$LIBMEDIASTREAMER2_CPPFLAGS" LDFLAGS="-L$AMSIP_PREFIX/lib -lm" CFLAGS="$LIBMEDIASTREAMER2_CFLAGS" LIBS="$LIBMEDIASTREAMER2_LIBS" $LIBMEDIASTREAMER2_OPTIONS --prefix=$AMSIP_PREFIX > /dev/null 2>&1
	set +x
    fi

    if test ! -f config.log; then
	echo "FAILED"
	exit
    fi
    
    make > /dev/null 2>&1
    if test ! -f src/.libs/libmediastreamer.a; then
	echo "FAILED"
	exit
    fi
    make install > /dev/null 2>&1
    mkdir -p $AMSIP_PREFIX/lib/pkgconfig/
    cp mediastreamer.pc $AMSIP_PREFIX/lib/pkgconfig/mediastreamer.pc
    popd > /dev/null 2>&1
    echo "DONE"
}

compile_amsip_libosip2 ()
{
    echo -n "amsip: compiling osip2 "
    pushd osip > /dev/null 2>&1
    

    if test ! -f config.log; then
	set -x
        ./configure CPPFLAGS="$LIBOSIP_CPPFLAGS" CFLAGS="$LIBOSIP_CFLAGS" LIBS="$LIBOSIP_LIBS" $LIBOSIP_OPTIONS --prefix=$AMSIP_PREFIX > /dev/null 2>&1
	set +x
    fi
    if test ! -f config.log; then
	echo "FAILED"
	exit
    fi
    
    make > /dev/null 2>&1
    if test ! -f src/osip2/.libs/libosip2.a; then
	echo "FAILED"
	exit
    fi
    make install > /dev/null 2>&1
    popd > /dev/null 2>&1
    echo "DONE"
}

compile_amsip_libeXosip2 ()
{
    echo -n "amsip: compiling eXosip2 "
    pushd exosip > /dev/null 2>&1
    
    if test ! -f config.log; then
	set -x
        ./configure CPPFLAGS="$LIBEXOSIP_CPPFLAGS" CFLAGS="$LIBEXOSIP_CFLAGS" LIBS="$LIBEXOSIP_LIBS" $LIBEXOSIP_OPTIONS --prefix=$AMSIP_PREFIX > /dev/null 2>&1
	set +x
    fi
    if test ! -f config.log; then
	echo "FAILED"
	exit
    fi
    
    make > /dev/null 2>&1
    if test ! -f src/.libs/libeXosip2.a; then
	echo "FAILED"
	exit
    fi
    make install > /dev/null 2>&1
    popd > /dev/null 2>&1
    echo "DONE"
}

compile_amsip_libamsip ()
{
    echo -n "amsip: compiling libamsip "
    pushd amsip > /dev/null 2>&1
    
    if test ! -f config.log; then
	set -x
        ./configure CPPFLAGS="$LIBAMSIP_CPPFLAGS" CFLAGS="$LIBAMSIP_CFLAGS" LIBS="$LIBAMSIP_LIBS" $LIBAMSIP_OPTIONS --prefix=$AMSIP_PREFIX > /dev/null 2>&1
	set +x
    fi

    if test ! -f config.log; then
	echo "FAILED"
	exit
    fi
    
    make > /dev/null 2>&1
    if test ! -f src/.libs/libamsip.a; then
	echo "FAILED"
	exit
    fi
    make install > /dev/null 2>&1
    popd > /dev/null 2>&1
    echo "DONE"
}

reconfig_amsip_all ()
{
    pushd srtp > /dev/null 2>&1
    rm config.log
    popd
    pushd oRTP > /dev/null 2>&1
    rm config.log
    popd
    if test "x$ENABLE_SPEEX" = xyes; then
	pushd codecs/speex > /dev/null 2>&1
	rm config.log
	popd
    fi
    if test "x$ENABLE_GSM" = xyes; then
	pushd codecs/gsm > /dev/null 2>&1
	rm config.log
	popd
    fi
    #if test "x$ENABLE_ILBC" = xyes; then
	#pushd plugins/msilbc/ilbc-rfc3951 > /dev/null 2>&1
	#rm config.log
	#popd
    #fi
    if test "x$ENABLE_VIDEO" = xyes; then
	pushd codecs/libtheora > /dev/null 2>&1
	rm config.log
	popd
	pushd codecs/ffmpeg > /dev/null 2>&1
	rm config.log
	popd
    fi
    pushd mediastreamer2 > /dev/null 2>&1
    rm config.log
    popd
    pushd osip > /dev/null 2>&1
    rm config.log
    popd
    pushd exosip > /dev/null 2>&1
    rm config.log
    popd
    pushd amsip > /dev/null 2>&1
    rm config.log
    popd
}

case "$1" in

  reconfig)
     check_amsip_config
     echo ""
     if test "x$2" = xall; then
        reconfig_amsip_all
     elif test "x$2" = x; then
        reconfig_amsip_all
     #else
     #   reconfig_amsip_all
     fi
     ;;

  compile)
     check_amsip_check
     check_amsip_config
     echo ""
     if test "x$2" = xall; then
        compile_amsip_all
     elif test "x$2" = x; then
        compile_amsip_all
     #else
     #   compile_amsip_3pp
     fi
     ;;

  forcecompile)
     check_amsip_config
     echo ""
     if test "x$2" = xall; then
        compile_amsip_all
     elif test "x$2" = x; then
        compile_amsip_all
     #else
     #   compile_amsip_3pp
     fi
     ;;

  check)
     check_amsip_config
     check_amsip_check
     ;;
  *)
     echo "Usage: $N {check|reconfig|compile|forcecompile}" >&2
     exit 1
     ;;
esac

exit

