ifndef LINPHONE_BUILD
 LINPHONE_BUILD="../../"
endif

PLUGINS_MAKEFILE=$(LINPHONE_BUILD)/mediastreamer2/plugins/plugins.mk

all:
	$(MAKE) -f $(PLUGINS_MAKEFILE) LINPHONE_BUILD=$(LINPHONE_BUILD)\
		PLUGIN_NAME=msilbc \
		PLUGIN_LIBS="-lilbc -lmediastreamer -lspeex -lortp" \
		PLUGIN_OBJS=ilbc.o

dist:
	$(MAKE) -f $(PLUGINS_MAKEFILE) LINPHONE_BUILD=$(LINPHONE_BUILD) \
					PLUGIN_NAME=msilbc PLUGIN_SOURCES="ilbc.c Makefile README ilbc-rfc3951.tar.gz" dist

install:
	$(MAKE) -f $(PLUGINS_MAKEFILE) PLUGIN_NAME=msilbc LINPHONE_BUILD=$(LINPHONE_BUILD) install

clean:
	$(MAKE) -f $(PLUGINS_MAKEFILE) LINPHONE_BUILD=$(LINPHONE_BUILD) clean
