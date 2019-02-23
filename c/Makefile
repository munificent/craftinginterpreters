
# This Makefile leverages CMake to produce the several different build types.
# This gives the ability to call 'make' in the root of the project and get
# sensible results, which allows nice integration into vim's ':make' command,
# as well as aliases like make='bear make' for integration with YouCompleteMe.

PROJECTROOT := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
BUILDROOT := $(abspath $(PROJECTROOT)/BUILD)

.DEFAULT_GOAL := all
.PHONY : all
all : debug release relwithdebinfo minsizerel coverage

.PHONY : echo
echo :
	@echo "PROJECTROOT=$(PROJECTROOT)"
	@echo "BUILDROOT=$(BUILDROOT)"

.PHONY : clean
clean :
	$(RM) -r "$(BUILDROOT)"

.PHONY : debug
debug :
	mkdir -p "$(BUILDROOT)/Debug"
	cd "$(BUILDROOT)/Debug" ; cmake "$(PROJECTROOT)" -DCMAKE_BUILD_TYPE=Debug
	$(MAKE) -C "$(BUILDROOT)/Debug"

.PHONY : release
release :
	mkdir -p "$(BUILDROOT)/Release"
	cd "$(BUILDROOT)/Release" ; cmake "$(PROJECTROOT)" -DCMAKE_BUILD_TYPE=Release
	$(MAKE) -C "$(BUILDROOT)/Release"

.PHONY : RelWithDebInfo
relwithdebinfo :
	mkdir -p "$(BUILDROOT)/RelWithDebInfo"
	cd "$(BUILDROOT)/RelWithDebInfo" ; cmake "$(PROJECTROOT)" -DCMAKE_BUILD_TYPE=RelWithDebInfo
	$(MAKE) -C "$(BUILDROOT)/RelWithDebInfo"

.PHONY : minsizerel
minsizerel :
	mkdir -p "$(BUILDROOT)/MinSizeRel"
	cd "$(BUILDROOT)/MinSizeRel" ; cmake "$(PROJECTROOT)" -DCMAKE_BUILD_TYPE=MinSizeRel
	$(MAKE) -C "$(BUILDROOT)/MinSizeRel"

.PHONY : coverage
coverage :
	mkdir -p "$(BUILDROOT)/Coverage"
	cd "$(BUILDROOT)/Coverage" ; cmake "$(PROJECTROOT)" -DCMAKE_BUILD_TYPE=Coverage
	$(MAKE) -C "$(BUILDROOT)/Coverage"

