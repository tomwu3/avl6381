ccflags-y += -D CONFIG_DVB_NET

KERNEL_VERSION=$(notdir $(KERNEL_DIR))
ver_main := $(word 1,$(subst ., ,$(subst linux-,,$(KERNEL_VERSION))))
URL=https://www.kernel.org/pub/linux/kernel/v$(ver_main).x
DVB_CORE_DIR=drivers/media/dvb-core
DVB_USB_DIR=drivers/media/usb/dvb-usb
DVB_USB_V2_DIR=drivers/media/usb/dvb-usb-v2
DVB_FRONTENDS_DIR=drivers/media/dvb-frontends
DVB_TUNERS_DIR=drivers/media/tuners

dvb-core-objs += linuxdvb/dvbdev.o linuxdvb/dmxdev.o linuxdvb/dvb_demux.o 
dvb-core-objs += linuxdvb/dvb_ca_en50221.o linuxdvb/dvb_frontend.o linuxdvb/dvb_net.o 
dvb-core-objs += linuxdvb/dvb_ringbuffer.o linuxdvb/dvb_math.o

#linux kernel version <= 4.10.0
#dvb-core-objs += linuxdvb/dvb_filter.o

obj-m += dvb-core.o

dvb-usb-objs += linuxdvb/dvb-usb-firmware.o linuxdvb/dvb-usb-init.o linuxdvb/dvb-usb-urb.o 
dvb-usb-objs += linuxdvb/dvb-usb-i2c.o linuxdvb/dvb-usb-dvb.o linuxdvb/dvb-usb-remote.o linuxdvb/usb-urb.o
obj-m += dvb-usb.o

dvb_usb_v2-objs := linuxdvb/dvb_usb_core.o linuxdvb/dvb_usb_urb.o linuxdvb/usb_urb.o
obj-m += dvb_usb_v2.o

mxl603-objs += mxl603_tuner.o mxl603_api.o
obj-m += mxl603.o
obj-m += it930x.o
obj-m += avl6381.o


PWD=$(shell pwd)

ccflags-y += -I$(KERNEL_DIR)/$(DVB_CORE_DIR)
ccflags-y += -I$(KERNEL_DIR)/$(DVB_USB_DIR)
ccflags-y += -I$(KERNEL_DIR)/$(DVB_USB_V2_DIR)
ccflags-y += -I$(KERNEL_DIR)/$(DVB_FRONTENDS_DIR)
ccflags-y += -I$(KERNEL_DIR)/$(DVB_TUNERS_DIR)

all:
	make -C $(KERNEL_DIR) \
	ARCH=$(ARCH) \
	CROSS_COMPILE=$(TOOLCHAIN) \
	M=$(PWD) \
	modules

prepare:
	if [ ! -d "dl" ];then \
		mkdir dl; \
	fi
	if [ ! -d "linuxdvb" ];then \
		mkdir linuxdvb; \
	fi
	wget -c $(URL)/$(KERNEL_VERSION).tar.xz -O $(PWD)/dl/$(KERNEL_VERSION).tar.xz
	tar xvf $(PWD)/dl/$(KERNEL_VERSION).tar.xz -C $(KERNEL_DIR)/ --wildcards --strip-components 1 \
		$(KERNEL_VERSION)/$(DVB_CORE_DIR)/* \
		$(KERNEL_VERSION)/$(DVB_USB_DIR)/dvb*.h \
		$(KERNEL_VERSION)/$(DVB_USB_V2_DIR)/dvb*.h \
		$(KERNEL_VERSION)/$(DVB_FRONTENDS_DIR)/dvb* \
		$(KERNEL_VERSION)/$(DVB_TUNERS_DIR)/tuner-i2c.h
	tar xvf $(PWD)/dl/$(KERNEL_VERSION).tar.xz -C $(PWD)/linuxdvb/ --wildcards --strip-components 4 \
		$(KERNEL_VERSION)/$(DVB_CORE_DIR)/*.c \
		$(KERNEL_VERSION)/$(DVB_FRONTENDS_DIR)/dvb*.*
	tar xvf $(PWD)/dl/$(KERNEL_VERSION).tar.xz -C $(PWD)/linuxdvb/ --wildcards --strip-components 5 \
		$(KERNEL_VERSION)/$(DVB_USB_DIR)/usb-urb.c \
		$(KERNEL_VERSION)/$(DVB_USB_DIR)/dvb-usb*.c \
		$(KERNEL_VERSION)/$(DVB_USB_DIR)/dvb-usb*.h \
		$(KERNEL_VERSION)/$(DVB_USB_V2_DIR)/usb_urb.c \
		$(KERNEL_VERSION)/$(DVB_USB_V2_DIR)/dvb_usb*

clean:
	rm -f *.ko
	rm -f *.o
	rm -f *.mod
	rm -f *.mod.c
	rm -f *.mod.o
	rm -f *.order
	rm -f *.sysvers
	rm -f .*.cmd
	rm -f .*.swp
	rm -f Module.symvers
	rm -rf .tmp_versions/
	rm linuxdvb/*.o linuxdvb/.*.cmd
							  

