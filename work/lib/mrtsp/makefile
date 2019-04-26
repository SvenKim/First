#----------------for platform---------------------------------------
ifeq ($(strip $(frameworks_root)),)
  frameworks_root:=$(shell \
          check_path="$(shell pwd)"; \
          while [ "$$check_path" != "" ]; do \
             if [ -d "$$check_path/frameworks" ] && [ -f "$$check_path/frameworks/frameworks.mk" ]; then \
               echo "$$check_path/frameworks"; \
               break;\
             fi;\
             check_path=`echo "$$check_path" | sed "s/\/[^\/]*$$//"`; \
          done;\
      )
  ifneq ($(frameworks_root),)
    include $(frameworks_root)/frameworks.mk
  endif
endif
#----------------for platform---------------------------------------


#----------------target-os target-arch------------------------------
ifeq ($(strip $(target_os)),)
  ifneq ($(shell uname -a | grep -i "linux"),)
    target_os=linux
  else
    ifneq ($(shell uname -a | grep -i "darwin"),)
      target_os=darwin
    endif
  endif
endif
ifeq ($(strip $(target_arch)),)
  ifneq ($(shell uname -a | grep -i "x86_64"),)
    target_arch=x64
  else
    ifneq ($(shell uname -a | grep -i "i386"),)
      target_arch=x86
    else
      ifneq ($(shell uname -a | grep -i "i686"),)
        target_arch=x86
      else
        ifneq ($(shell uname -a | grep -i "x86"),)
          target_arch=x86
        endif
      endif
    endif
  endif
endif
#----------------target-os target-arch------------------------------


#----------------compile util prepare(cross deal with here)---------
ifneq ($(strip $(target_os)),)
  ifeq ($(shell test -e makefile.$(strip $(target_os)) && echo $$?), 0)
    include makefile.$(strip $(target_os)) 
  endif
endif
ifeq ($(strip $(cc)),)
  cc:=gcc
endif
ifeq ($(strip $(ar)),)
  ar:=ar
endif
ifeq ($(strip $(ranlib)),)
  ranlib:=ranlib
endif
ifeq ($(strip $(cxx)),)
  cxx:=g++
endif
ifeq ($(strip $(as)),)
  as:=gcc
endif
ifeq ($(strip $(objc)),)
  objc:=clang
endif
ifeq ($(strip $(objcxx)),)
  objcxx:=clang
endif
#----------------compile util prepare(cross deal with here)---------


#makefile.native
ifeq ($(shell test -e makefile.native && echo $$?), 0)
  include makefile.native
endif


#target_name
tmp_target_name_direct=$(shell pwd | sed "s/$$//")
ifeq ($(strip $(target_name)),)
  target_name:=$(notdir $(shell pwd | sed "s/\/src$$//"))
endif


#target_type a|so|channels|comp|exe|empty==auto-detect
ifeq ($(strip $(target_type)),)
  ifneq ($(shell pwd | grep "/lib/"),)
    target_type:=so
  endif
  ifneq ($(shell pwd | grep "/component/"),)
    target_type:=comp
  endif
  ifneq ($(shell pwd | grep "/media/channels/"),)
    target_type:=channels
  endif
  ifeq ($(target_type),)
    target_type:=exe
  endif
endif
ifeq ($(filter a so comp channels exe, $(target_type)),)
  target_type=exe
endif

#target_file
ifeq ($(strip $(target_file)),)
  #target_suffix
  ifeq ($(target_suffix),)
    ifneq ($(target_type),exe)
      ifeq ($(target_type),so)
        ifneq ($(so_suffix),)
          target_suffix:=$(so_suffix)
        else
          target_suffix:=.$(target_type)
        endif
      else
        target_suffix:=.$(target_type)
      endif
    endif
  endif
  
  #target_prefix
  ifeq ($(target_prefix),)
    ifneq ($(filter a so, $(target_type)),)
      ifeq ($(shell echo $(target_name) | grep "^lib"),)
        target_prefix:=lib
      endif
    endif
  endif
  
  target_file:=$(target_prefix)$(target_name)$(target_debug)$(target_suffix)
endif

#platforms_root
ifeq ($(strip $(platforms_root)),)
  ifneq ($(strip $(frameworks_root)),)
    platforms_root:=$(frameworks_root)/../platforms
  else
    platforms_root:=platforms
  endif
endif


#arch_path
ifeq ($(strip $(arch_path)),)
  arch_path:=$(platforms_root)
  $(shell test -e "$(arch_path)" || mkdir -p "$(arch_path)")
  ifeq ($(strip $(target_os)),)
    ifeq ($(strip $(target_arch)),)
      arch_path:=$(arch_path)/unknown-unknown
    else
      arch_path:=$(arch_path)/unknown-$(target_arch)
    endif
  else
    ifeq ($(strip $(target_arch)),)
      arch_path:=$(arch_path)/$(target_os)-unknown
    else
      arch_path:=$(arch_path)/$(target_os)-$(target_arch)
    endif
  endif
endif
$(shell test -e "$(arch_path)" || mkdir -p "$(arch_path)")


#xxx_path
ifeq ($(strip $(bin_path)),)
  bin_path:=$(arch_path)/bin
endif
ifeq ($(strip $(lib_path)),)
  lib_path:=$(arch_path)/lib
endif
ifeq ($(strip $(obj_path)),)
  obj_path:=$(arch_path)/obj
  $(shell test -e "$(obj_path)" || mkdir -p "$(obj_path)")
  obj_path:=$(obj_path)/$(target_file)
endif
$(shell test -e "$(obj_path)" || mkdir -p "$(obj_path)")
ifeq ($(strip $(inc_path)),)
  inc_path:=$(arch_path)/include
endif


#cflags
cflags:= $(cflags) -I./ -I../
ifneq ($(inc_path),)
  cflags:= $(cflags) -I$(inc_path)
endif
ifneq ($(inc_path),$(arch_path)/include)
  cflags:= $(cflags) -I$(arch_path)/include)
endif
ifneq ($(frameworks_root),)
  cflags:= $(cflags) -I$(frameworks_root)/../platforms/universal/include
endif
ifneq ($(tmp_target_name_direct),$(target_name)) 
  cflags:= $(cflags) -I../../ -I../include -I../inc
endif
ifneq ($(strip $(target_debug)),)
  cflags:= $(cflags) -D_DEBUG -g
endif
ifneq ($(frameworks_root),)
  cflags:= $(cflags) -I $(inc_path)
endif
ifeq ($(target_type),a)
  cflags:= $(cflags) $(cflags_a) -fPIC -D_LIB
endif
ifeq ($(target_type),so)
  cflags:= $(cflags) $(cflags_so) -fPIC
endif
ifeq ($(target_type),channels)
  cflags:= $(cflags) $(cflags_channels) -fPIC
endif
ifeq ($(target_type),comp)
  cflags:= $(cflags) $(cflags_comp) -fPIC
endif
ifeq ($(target_type),exe)
  cflags:= $(cflags) $(cflags_exe) -D_CONSOLE
endif


#ldflags
ifneq ($(frameworks_root),)
  ldflags:= $(ldflags) -L$(lib_path) -L$(bin_path) -Wl,-rpath,$(lib_path):$(bin_path)
endif


#srcs
ifeq ($(strip $(srcs)),)
  srcs:=$(shell find * | grep "[.]c[p]*$$")
  ifneq ($(filter 1 Y y YES yes Yes enabled,$(objc_enabled)),)
    srcs:=$(srcs) $(shell find * | grep "[.][m]*$$")
  endif
  ifneq ($(filter 1 Y y YES yes Yes enabled,$(asm_enabled)),)
    srcs:=$(srcs) $(shell find * | grep "[.]s$$")
  endif
endif


#objs
ifeq ($(strip $(objs)),)
  objs_short:=$(patsubst %.c,%.o,$(filter %.c,$(srcs))) \
      $(patsubst %.cpp,%.opp,$(filter %.cpp,$(srcs))) \
      $(patsubst %.s,%.os,$(filter %.s,$(srcs))) \
      $(patsubst %.m,%.om,$(filter %.m,$(srcs))) \
      $(patsubst %.mm,%.omm,$(filter %.mm,$(srcs)))
  objs:=$(patsubst %.c,$(obj_path)/%.o,$(filter %.c,$(srcs))) \
      $(patsubst %.cpp,$(obj_path)/%.opp,$(filter %.cpp,$(srcs))) \
      $(patsubst %.s,$(obj_path)/%.os,$(filter %.s,$(srcs))) \
      $(patsubst %.m,$(obj_path)/%.om,$(filter %.m,$(srcs))) \
      $(patsubst %.mm,$(obj_path)/%.omm,$(filter %.omm,$(srcs)))
  $(foreach i, $(sort $(dir $(objs))), $(shell test -e $(i) || mkdir -p $(i) ))
  vpath %.o $(obj_path)
  vpath %.opp $(obj_path)
  vpath %.os $(obj_path)
  vpath %.om $(obj_path)
  vpath %.omm $(obj_path)
endif


#target
target:=$(obj_path)/$(target_file)

.PHONY: all clean install installi

all:$(target)


$(target):$(objs_short)
ifeq ($(strip $(target_build)),)
  ifneq ($(filter so channels comp, $(target_type)),)
	$(cc) -shared -o $@ $(objs) $(ldflags) $(libs)
  endif
  ifeq ($(target_type),exe)
	$(cc) -o $@ $(objs) $(ldflags) $(libs)
  endif
  ifeq ($(target_type),a)
	$(ar) rc $(target) $(objs) 
	$(ranlib) $(target) 
  endif
else
	$(target_build)
endif


install:
ifneq ($(filter a so, $(target_type)),)
	if [ ! -e "$(lib_path)" ]; then mkdir -p "$(lib_path)"; fi
	cp -f $(target) "$(lib_path)/$(target_file)"
else
	if [ ! -e "$(bin_path)" ]; then mkdir -p "$(bin_path)"; fi
	cp -f $(target) "$(bin_path)/$(target_file)"
endif


installi:
ifneq ($(filter a so, $(target_type)),)
	if [ ! -e "$(inc_path)" ]; then mkdir -p "$(inc_path)"; fi
	cp -f *.h "$(inc_path)/"
	rm -f "$(inc_path)/_*.h"
	rm -f "$(inc_path)/*__*.h"
else
	echo "install include header files nothing"
endif


%.o:%.c
	$(cc) $(cflags) -c -o $(obj_path)/$@ $<


%.opp:%.cpp
	$(cxx) $(cflags) -c -o $(obj_path)/$@ $<


%.os:%.s
	$(as) $(cflags) -c -o $(obj_path)/$@ $<


%.om:%.m
	$(objc) $(cflags) -c -o $(obj_path)/$@ $<


%.omm:%.mm
	$(objcxx) $(cflags) -c -o $(obj_path)/$@ $<


clean:  
ifneq ($(strip $(objs_keep)), true)
	-rm -f $(objs)
endif
	-rm -f $(target)


