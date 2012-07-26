

plugindir=$(PACKAGE_PLUGINS_DIR)

plugin_LTLIBRARIES=libmsipph264.la

libmsipph264_la_SOURCES=h264.cc

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

libmsipph264_la_LIBADD=	-lh264_enc \
			-lh264_dec \
			-lcolor_space_converter \
			-lumc \
			-lvm \
			-lvm_plus \
			$(IPPI_LIBS)

AM_CXXFLAGS=$(IPP_CXXFLAGS) $(IPPSAMPLES_CXXFLAGS)

AM_LDFLAGS=$(IPP_LIBS) $(IPPSAMPLES_LIBS)

INCLUDES=	-I$(UMC_SRC_DIR)/core/umc/include \
		-I$(UMC_SRC_DIR)/core/vm/include \
		-I$(UMC_SRC_DIR)/codec/color_space_converter/include \
		-I$(UMC_SRC_DIR)/codec/h264_dec/include \
		-I$(UMC_SRC_DIR)/codec/h264_enc/include