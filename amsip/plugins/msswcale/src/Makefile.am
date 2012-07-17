

plugindir=$(PACKAGE_PLUGINS_DIR)

plugin_LTLIBRARIES=libmsippyuv.la

libmsippyuv_la_SOURCES=yuv.c

IPPI_LIBS= 	-lippdc$(IPP_LIBEXT) \
		 -lippcc$(IPP_LIBEXT) \
		 -lippac$(IPP_LIBEXT) \
		 -lippsr$(IPP_LIBEXT)\
		 -lippvc$(IPP_LIBEXT) \
		 -lippcv$(IPP_LIBEXT)\
		 -lippj$(IPP_LIBEXT)\
		 -lippi$(IPP_LIBEXT)\
		 -lipps$(IPP_LIBEXT)\
		 -lippsc$(IPP_LIBEXT)\
		 -lippcore$(IPP_LIBEXT)

libmsippyuv_la_LIBADD=	$(IPPI_LIBS)

AM_CXXFLAGS=$(IPP_CXXFLAGS)

AM_LDFLAGS=$(IPP_LIBS)

