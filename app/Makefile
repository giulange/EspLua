#############################################################
# Required variables for each makefile
# Discard this section from all parent makefiles
# Expected variables (with automatic defaults):
#   CSRCS (all "C" files in the dir)
#   SUBDIRS (all subdirs with a Makefile)
#   GEN_LIBS - list of libs to be generated ()
#   GEN_IMAGES - list of object file images to be generated ()
#   GEN_BINS - list of binaries to be generated ()
#   COMPONENTS_xxx - a list of libs/objs in the form
#     subdir/lib to be extracted and rolled up into
#     a generated lib/image xxx.a ()
#
TARGET = eagle

ifndef PDIR # {
GEN_IMAGES= eagle.app.v6.out
GEN_BINS= eagle.app.v6.bin
SPECIAL_MKTARGETS=$(APP_MKTARGETS)
SUBDIRS=    \
	main	\
	driver	\
	user	\
	platform	\
	lua 	\
	coap	\
	mqtt	\
	u8glib  \
	smart 	\
	modules	\
	spiffs	\
	crypto	\
	dhtlib	\
	libc 	\
	cjson   

#	lwip	\
#	upgrade	\
#	json	\

endif # } PDIR

APPDIR = .
LDDIR = ../ld


TARGET_LDFLAGS =		\
	-nostdlib		\
	-Wl,-EL \
	--longcalls \
	--text-section-literals

LD_FILE = $(LDDIR)/eagle.app.v6.ld


COMPONENTS_eagle.app.v6 = \
    main/libaddmmain.a \
	user/libuser.a	\
	driver/libdriver.a	\
	platform/libplatform.a	\
	lua/liblua.a 	\
	coap/coap.a 	\
	mqtt/mqtt.a 	\
	u8glib/u8glib.a \
	smart/smart.a 	\
	spiffs/spiffs.a 	\
	cjson/libcjson.a 	\
	crypto/libcrypto.a	\
	dhtlib/libdhtlib.a	\
	libc/liblibc.a 	\
	modules/libmodules.a	

#	lwip/liblwip.a	\
#	json/libjson.a	\
#	upgrade/libupgrade.a	\

LINKFLAGS_eagle.app.v6 = \
	-L../lib        \
	-Wl,--gc-sections \
	-Wl,-Map -Wl,$(@:.out=.map) \
	-nostdlib	\
    -T$(LD_FILE)   \
	-Wl,--no-check-sections	\
    -u call_user_start	\
	-Wl,-static	\
	-Wl,--start-group	\
	-l0main	\
	-lmphy	\
	-lmlwip	\
	-lpp	\
	-lnet80211	\
	-lmwpa	\
	-lssl	\
	-lsmartconfig	\
	-lm	\
	-lmc	\
	-lmgcc	\
	$(DEP_LIBS_eagle.app.v6)	\
	-Wl,--end-group
	 
#	-ljson	\
#	-lhal					\
#	-lsmartconfig	\

DEPENDS_eagle.app.v6 = \
                $(LD_FILE) \
                $(LDDIR)/eagle.rom.addr.v6.ld

#############################################################
# Configuration i.e. compile options etc.
# Target specific stuff (defines etc.) goes in here!
# Generally values applying to a tree are captured in the
#   makefile at its root level - these are then overridden
#   for a subtree within the makefile rooted therein
#

#UNIVERSAL_TARGET_DEFINES =		\

# Other potential configuration flags include:
#	-DTXRX_TXBUF_DEBUG
#	-DTXRX_RXBUF_DEBUG
#	-DWLAN_CONFIG_CCX
CONFIGURATION_DEFINES =	-D__ets__ \
			-DICACHE_FLASH	

DEFINES +=				\
	$(UNIVERSAL_TARGET_DEFINES)	\
	$(CONFIGURATION_DEFINES)

DDEFINES +=				\
	$(UNIVERSAL_TARGET_DEFINES)	\
	$(CONFIGURATION_DEFINES)


#############################################################
# Recursion Magic - Don't touch this!!
#
# Each subtree potentially has an include directory
#   corresponding to the common APIs applicable to modules
#   rooted at that subtree. Accordingly, the INCLUDE PATH
#   of a module can only contain the include directories up
#   its parent path, and not its siblings
#
# Required for each makefile to inherit from the parent
#

INCLUDES := $(INCLUDES) -I $(PDIR)include
INCLUDES += -I ./
PDIR := ../$(PDIR)
sinclude $(PDIR)Makefile


.PHONY: FORCE
FORCE:
