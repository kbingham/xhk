##
## Autotools build wrapper.
## Kieran Bingham
## August 2014
##
## Configures and builds under build-<arch>
##
## Cross compilation can be supported by defining
##   ARCH=<target arch>
##   CROSS=<cross prefix of toolchain on path>
##

##
## Some very useful advice used here from http://mad-scientist.net/make/multi-arch.html
##

# Disable all implicit build targets
.SUFFIXES:

#ARCH?=armv7
ARCH?=$(shell uname -m)

BUILD_DIR=build-$(ARCH)
SOURCE_DIR=$(CURDIR)

export CC=${CROSS}gcc
export LD=${CROSS}gcc
export CXX=${CROSS}c++
export ARCH
export PATH
export DESTDIR

# No make directory output from this wrapper
MAKE:=$(MAKE) --no-print-directory

.PHONY: all
all: $(BUILD_DIR)/Makefile
	+@echo "  [$(BUILD_DIR)] $(MAKECMDGOALS)"
	+@$(MAKE) --no-print-directory -C $(BUILD_DIR) $(MAKECMDGOALS)

$(BUILD_DIR):
	@echo " [$(BUILD_DIR)] MKDIR"
	@mkdir -p $@

$(SOURCE_DIR)/configure:
	@echo " [$(BUILD_DIR)] Autoconf"
	cd $(SOURCE_DIR) && autoreconf --verbose --force --install

$(BUILD_DIR)/Makefile: $(SOURCE_DIR)/configure | $(BUILD_DIR)
	@echo " [$(BUILD_DIR)] Configure"
	cd $(BUILD_DIR) && \
		$(SOURCE_DIR)/configure	\
			--prefix=/usr \
			--host=$(CROSS)linux \
			--build=$(shell uname -p)-linux \
			--with-sysroot \

# Prevent Make from sending these targets through the wrapper CMDGOALS
Makefile : ;
%.mk :: ;

# Send all other targets through the default target
% :: all ;

deb:
	@echo " [$(BUILD_DIR)] Packaging"
	debuild -us -uc

distclean:
	@echo " [$(BUILD_DIR)] $@"
	@rm -rf $(BUILD_DIR)
	@git clean -fxd
