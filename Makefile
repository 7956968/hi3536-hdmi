
CROSS_COMPILE = arm-hisiv400-linux-gnueabi-
AS		= $(CROSS_COMPILE)as
LD		= $(CROSS_COMPILE)ld
CC		= $(CROSS_COMPILE)gcc
CPP		= $(CC) -E
AR		= $(CROSS_COMPILE)ar
NM		= $(CROSS_COMPILE)nm

STRIP		= $(CROSS_COMPILE)strip
OBJCOPY		= $(CROSS_COMPILE)objcopy
OBJDUMP		= $(CROSS_COMPILE)objdump

export AS LD CC CPP AR NM
export STRIP OBJCOPY OBJDUMP

CFLAGS := -Wall -O2 -g
CFLAGS  += -mcpu=cortex-a7 -mfloat-abi=softfp -mfpu=neon-vfpv4 -mno-unaligned-access -fno-aggressive-loop-optimizations -I$(PWD)/include -I/home/book/work/host/arm-buildroot-linux-gnueabi/sysroot/usr/include/glib-2.0 -I/home/book/work/host/arm-buildroot-linux-gnueabi/sysroot/usr/lib/glib-2.0/include -I/home/book/work/host/arm-buildroot-linux-gnueabi/sysroot/usr/include -I/home/book/work/Hi3536_SDK_V2.0.6.0/mpp_single/include -I/home/book/work/Hi3536_SDK_V2.0.6.0/mpp_single/include/mkp -I/home/book/work/host/arm-buildroot-linux-gnueabi/sysroot/usr/include/gstreamer-1.0

LDFLAGS := -L/home/book/work/host/arm-buildroot-linux-gnueabi/sysroot/usr/lib -L/home/book/work/Hi3536_SDK_V2.0.6.0/mpp_single/lib -lglib-2.0 -lpthread -lm -lmpi -lhdmi -ljpeg6b -lupvqe -ldnvqe -lVoiceEngine -lhive_RNR -lhive_AEC -lhive_GAIN -lhive_RES -lhive_common -lhive_HPF -lhive_EQ -lhive_ANR -lhive_AGC -lgstapp-1.0 -lgstbase-1.0 -lgstreamer-1.0 -lgobject-2.0

export CFLAGS LDFLAGS

TOPDIR := $(shell pwd)
export TOPDIR

TARGET := sample_hdmi

obj-y += video_common.o
obj-y += video_hdmi.o
obj-y += sample_hdmi.o

all :
	make -C ./ -f $(TOPDIR)/Makefile.build
	$(CC) $(LDFLAGS) -o $(TARGET) built-in.o


clean:
	rm -f $(shell find -name "*.o")
	rm -f $(TARGET)

distclean:
	rm -f $(shell find -name "*.o")
	rm -f $(shell find -name "*.d")
	rm -f $(TARGET)
