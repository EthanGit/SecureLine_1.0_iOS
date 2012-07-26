
EXTRA_DIST = amsip-internal.h test_wce.c dxifilter.h  am_video_module_mintpad.c am_filter_mintpad.cpp dxifilter.cpp

INSTALL_DIR  = $(prefix)
noinst_PROGRAMS = test test_register test_answercall test_notifyservice test_subscribeservice

test_SOURCES = test.c test_getopt.c test_getopt.h test_account.h
test_LDFLAGS = -L$(prefix)/lib @AMSIP_LIB@ @PTHREAD_LIBS@
test_LDADD = libamsip.la -leXosip2 -losipparser2 -losip2 -lortp -lmediastreamer -lspeex

test_register_SOURCES = test_register.c test_getopt.c test_getopt.h
test_register_LDFLAGS = -L$(prefix)/lib @AMSIP_LIB@ @PTHREAD_LIBS@
test_register_LDADD = ./libamsip.la -leXosip2 -losipparser2 -losip2 -lortp -lmediastreamer -lspeex

test_answercall_SOURCES = test_answercall.c test_getopt.c test_getopt.h
test_answercall_LDFLAGS = -L$(prefix)/lib @AMSIP_LIB@ @PTHREAD_LIBS@
test_answercall_LDADD = ./libamsip.la -leXosip2 -losipparser2 -losip2 -lortp -lmediastreamer -lspeex

test_notifyservice_SOURCES = test_notifyservice.c test_getopt.c test_getopt.h
test_notifyservice_LDFLAGS = -L$(prefix)/lib @AMSIP_LIB@ @PTHREAD_LIBS@
test_notifyservice_LDADD = ./libamsip.la -leXosip2 -losipparser2 -losip2 -lortp -lmediastreamer -lspeex

test_subscribeservice_SOURCES = test_subscribeservice.c test_getopt.c test_getopt.h
test_subscribeservice_LDFLAGS = -L$(prefix)/lib @AMSIP_LIB@ @PTHREAD_LIBS@
test_subscribeservice_LDADD = ./libamsip.la -leXosip2 -losipparser2 -losip2 -lortp -lmediastreamer -lspeex

lib_LTLIBRARIES = libamsip.la

libamsip_la_SOURCES = \
am_options.c   \
am_event.c      \
am_network.c  \
am_player.c      \
am_register.c   \
am_message.c  \
am_call.c          am_calls.h    \
am_publish.c    \
am_subscribe.c \
am_service.c    \
csharp.c           \
am_calls.c         \
sdptools.c        sdptools.h \
am_filter.c       \
am_ms2_video.c     am_video_codec.c  \
am_video_start.c    am_video_start.h   \
am_text_start.c      am_text_start.h     \
am_ms2_text.c       am_text_codec.c    \
am_ms2_win32.c    am_codec.c            \
am_audio_module.c am_sound_common.h \
am_video_module.c \
am_udpftp_start.c  am_udpftp_start.h     \
am_ms2_udpftp.c   am_udpftp_codec.c    \
am_udpftp.c  \
am_uuid.c               am_uuid.h         \
am_sysdep.c          am_sysdep.h


libamsip_la_LDFLAGS = -L$(prefix)/lib -version-info  $(LIBAMSIP_SO_VERSION)

libamsip_la_LIBADD = -leXosip2 -losipparser2 -losip2 @AMSIP_LIB@ @PTHREAD_LIBS@ \
-lortp -lmediastreamer -lresolv -lspeex

INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/include -I$(prefix)/include

AM_CFLAGS = $(AMSIP_FLAGS) $(PTHREAD_CFLAGS)


