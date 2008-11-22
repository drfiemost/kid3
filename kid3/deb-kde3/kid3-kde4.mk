#!/usr/bin/make -f

# For Ubuntu <=8.04 use this:
#DEB_CMAKE_PREFIX = /usr/lib/kde4
#DEB_CONFIG_INSTALL_DIR = $(DEB_CMAKE_PREFIX)/etc/kde4
# For Ubuntu >=8.10, Debian use this:
DEB_CMAKE_PREFIX ?= /usr
DEB_CONFIG_INSTALL_DIR ?= /usr/share/kde4/config

DEB_CMAKE_EXTRA_FLAGS += \
			-DCMAKE_BUILD_TYPE=Debian \
			$(KDE4-ENABLE-FINAL) \
			-DKDE4_BUILD_TESTS=false \
			-DKDE_DISTRIBUTION_TEXT="Debian packages" \
			-DCMAKE_SKIP_RPATH=true \
			-DKDE4_USE_ALWAYS_FULL_RPATH=false \
			-DCONFIG_INSTALL_DIR=$(DEB_CONFIG_INSTALL_DIR) \
			-DDATA_INSTALL_DIR=$(DEB_CMAKE_PREFIX)/share/kde4/apps \
			-DHTML_INSTALL_DIR=$(DEB_CMAKE_PREFIX)/share/doc/kde4/HTML \
			-DKCFG_INSTALL_DIR=$(DEB_CMAKE_PREFIX)/share/kde4/config.kcfg \
			-DLIB_INSTALL_DIR=$(DEB_CMAKE_PREFIX)/lib \
			-DSYSCONF_INSTALL_DIR=/etc

# Set the one below to something else than 'yes' to disable linking 
# with --as-needed (on by default)
DEB_KDE_LINK_WITH_AS_NEEDED ?= yes
ifneq (,$(findstring yes, $(DEB_KDE_LINK_WITH_AS_NEEDED)))
	ifeq (,$(findstring no-as-needed, $(DEB_BUILD_OPTIONS)))
		DEB_KDE_LINK_WITH_AS_NEEDED := yes
		DEB_CMAKE_EXTRA_FLAGS += \
					-DCMAKE_SHARED_LINKER_FLAGS="-Wl,--no-undefined -Wl,--as-needed" \
					-DCMAKE_MODULE_LINKER_FLAGS="-Wl,--no-undefined -Wl,--as-needed" \
					-DCMAKE_EXE_LINKER_FLAGS="-Wl,--no-undefined -Wl,--as-needed"
	else
		DEB_KDE_LINK_WITH_AS_NEEDED := no
	endif
else
	DEB_KDE_LINK_WITH_AS_NEEDED := no
endif

DEB_KDE_ENABLE_FINAL ?=

ifeq (,$(findstring noopt,$(DEB_BUILD_OPTIONS)))
    cdbs_treat_me_gently_arches := arm m68k alpha ppc64 armel armeb
    ifeq (,$(filter $(DEB_HOST_ARCH_CPU),$(cdbs_treat_me_gently_arches)))
        KDE4-ENABLE-FINAL = $(if $(DEB_KDE_ENABLE_FINAL),-DKDE4_ENABLE_FINAL=true,)
    else
        KDE4-ENABLE-FINAL =
    endif
endif

CMAKE = cmake
DEB_CMAKE_INSTALL_PREFIX = $(DEB_CMAKE_PREFIX)
DEB_CMAKE_NORMAL_ARGS = -DCMAKE_INSTALL_PREFIX="$(DEB_CMAKE_INSTALL_PREFIX)" -DCMAKE_C_COMPILER:FILEPATH="$(CC)" -DCMAKE_CXX_COMPILER:FILEPATH="$(CXX)" -DCMAKE_C_FLAGS="$(CFLAGS)" -DCMAKE_CXX_FLAGS="$(CXXFLAGS)" -DCMAKE_SKIP_RPATH=ON -DCMAKE_VERBOSE_MAKEFILE=ON

build: kid3-kde4.build-stamp

kid3-kde4.build-stamp:
	mkdir kid3-kde4; \
	cd kid3-kde4; \
	$(CMAKE) .. $(DEB_CMAKE_NORMAL_ARGS) $(DEB_CMAKE_EXTRA_FLAGS)
	cd ..
	$(MAKE) -C kid3-kde4

	touch kid3-kde4.build-stamp

clean: 
	[ ! -f kid3-kde4/Makefile ] || $(MAKE) -C kid3-kde4 clean
	-rm -rf kid3-kde4.build-stamp kid3-kde4

install: build
	$(MAKE) -C kid3-kde4 install DESTDIR=$(CURDIR)/debian/kid3-kde4

.PHONY: build clean install
