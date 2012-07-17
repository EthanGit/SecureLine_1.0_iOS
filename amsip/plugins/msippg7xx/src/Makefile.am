
SUBDIRS = sys

plugindir=$(PACKAGE_PLUGINS_DIR)

plugin_LTLIBRARIES=libmsippg7xx.la

libmsippg7xx_la_SOURCES= \
aux_tbls.c   decgsmamr.c  encg729.c    ownamrwb.c  tabgsmamr.c  uscgsmamr.c \
bitgsmamr.c  dtxamrwb.c   encgsmamr.c  owng722.c   uscamrwb.c   utilamrwb.c \
decamrwb.c   dtxgsmamr.c  formats.c    owng723.c   usccodec.c   util.c \
decg711.c    encamrwb.c   g722api.c    owng729.c   uscg711.c    utilgsmamr.c \
decg722.c    encg711.c    gain_util.c  player.c    uscg722.c    vadg723.c \
decg723.c    encg722.c    loadcodec.c  plcg711.c   uscg723.c    vadg729.c  \
decg729.c    encg723.c    msipp.c      tabamrwb.c  uscg729i.c   wavfile.c \
amrwbapi.h             owng711.h                       umc_usc.h \
amrwb_types.h          owng722.h                       usc_base.h \
amrwbusc.h             owng723.h                       usccodec.h \
aux_fnxs.h             owng729.h                       usc_ec.h \
base_rtp_cnvrt.h       owngsmamr.h                     uscfmt.h \
common_rtp_payloads.h  rtp_amr_payload.h               usc.h \
dummy_rtp_payload.h    rtp_amrwbe_payload.h            usc_objects_decl.h \
formats.h              rtppayload.h                    uscreg.h \
g711api.h              scratchmem.h                    util.h \
g722api.h              umc_audio_codec.h               vadg723.h \
g722.h                 umc_base_codec.h                vadg729.h \
g723api.h              umc_default_memory_allocator.h  vm_debug.h \
g723ipp.h              umc_dynamic_cast.h              vm_file.h \
g729api.h              umc_linked_list.h               vm_mutex.h \
g729usc.h              umc_media_data.h                vm_shared_object.h \
gsmamrapi.h            umc_memory_allocator.h          vm_strings.h \
gsmamr.h               umc_rtp_packet.h                vm_sys_info.h \
gsmamr_types.h         umc_speech_codec.h              vm_time.h \
gsmamrusc.h            umc_speech_data.h               vm_types.h \
loadcodec.h            umc_structures.h                wavfile.h \
ownamrwb.h             umc_usc_base.h

IPPI_LIBS= \
-lipps \
-lippsc \
-lippcore

libmsippg7xx_la_LIBADD=	$(IPPI_LIBS)

AM_CFLAGS=$(IPP_CFLAGS) $(IPPSAMPLES_CFLAGS)

AM_LDFLAGS=$(IPP_LIBS) $(IPPSAMPLES_LIBS)

INCLUDES=-I$(top_srcdir)/src -I$(top_srcdir)/src/sys
