# Linux DVB driver for runqida (AVL6381+MXL603+IT9303) DVBC/DTMB/DVBT2 demodulator USB dongle

#### ubuntu上使用openwrt SDK交叉编译

```shell
#下载SDK并解压
wget https://downloads.immortalwrt.org/releases/23.05.2/targets/x86/64/immortalwrt-sdk-23.05.2-x86-64_gcc-12.3.0_musl.Linux-x86_64.tar.xz
tar xvf immortalwrt-sdk-23.05.2-x86-64_gcc-12.3.0_musl.Linux-x86_64.tar.xz

# 下载源码
git clone https://github.com/nxdong520/avl6381.git
cd avl6381

# 设置环境变量(X86_64架构)
export ARCH=x86
export STAGING_DIR=../immortalwrt-sdk-23.05.2-x86-64_gcc-12.3.0_musl.Linux-x86_64/staging_dir/toolchain-x86_64_gcc-12.3.0_musl/bin/
export KERNEL_DIR=../immortalwrt-sdk-23.05.2-x86-64_gcc-12.3.0_musl.Linux-x86_64/build_dir/target-x86_64_musl/linux-x86_64/linux-5.15.150
export TOOLCHAIN="../immortalwrt-sdk-23.05.2-x86-64_gcc-12.3.0_musl.Linux-x86_64/staging_dir/toolchain-x86_64_gcc-12.3.0_musl/bin/x86_64-openwrt-linux-"

# 编译准备
make prepare

#开始编译
make
```

#### 将生成的内核驱动程序ko文件及固件文件复制到目标机器
```shell
#复制固件文件到/lib/firmware/
cp dvb-usb-it9303-01.fw /lib/firmware/

#加载驱动程序
insmod dvb-core.ko
insmod dvb_usb_v2.ko
insmod mxl603.ko
insmod avl6381.ko
insmod it930x.ko

root@ImmortalWrt:~# dmesg 
[ 5309.959314] usb 1-1: new high-speed USB device number 3 using ehci-pci
[ 5310.403899] usb 1-1: prechip_version=83 chip_version=01 chip_type=9306
[ 5310.513552] usb 1-1: reply=00 00 00 00
[ 5310.513792] usb 1-1: dvb_usb_v2: found a 'ITE IT930x Generic' in cold state
[ 5310.514240] usb 1-1: dvb_usb_v2: downloading firmware from file 'dvb-usb-it9303-01.fw'
[ 5310.655780] usb 1-1: firmware version=1.4.0.0
[ 5310.655973] usb 1-1: dvb_usb_v2: found a 'ITE IT930x Generic' in warm state
[ 5310.656337] usb 1-1: dvb_usb_v2: will use the device's hardware PID filter (table count: 64)
[ 5310.656812] dvbdev: DVB: registering new adapter (ITE IT930x Generic)
[ 5310.879067] usb 1-1: Checking for Availink AVL6381 DVB-S2/T2/C demod ...
[ 5310.896016] i2c i2c-0: avl6381: found AVL6381 family_id=0x63814e24
[ 5310.900679] i2c i2c-0: avl6381: ChipFamillyID:0x63814e24
[ 5310.906434] i2c i2c-0: avl6381: GetChipID:0x4d36e8
[ 5315.472672] usb 1-1: DVB: registering adapter 0 frontend 0 (Availink AVL6381 DVBC/DTMB/DVBT2 demodulator)...
[ 5315.517301] i2c i2c-0: MXL603 detected id(01) ver(02)
[ 5315.517525] i2c i2c-0: Attaching MXL603
[ 5315.782119] usb 1-1: dvb_usb_v2: 'ITE IT930x Generic' successfully initialized and connected

root@ImmortalWrt:~# ls /dev/dvb/adapter0/ -al
drwxr-xr-x    2 root     root           120 Nov 26 14:51 .
drwxr-xr-x    3 root     root            60 Nov 24 13:11 ..
crw-------    1 root     root      212,   4 Nov 26 14:51 demux0
crw-------    1 root     root      212,   5 Nov 26 14:51 dvr0
crw-------    1 root     root      212,   3 Nov 26 14:51 frontend0
crw-------    1 root     root      212,   7 Nov 26 14:51 net0

```

#### 说明

驱动成功加载后出现/dev/dvb/adapterx/目录   
提供给所有支持标准linux dvb api的软件使用   
本源码比原产品驱动程序增加休眠功能，大大减低空闲时的发热量，使用时需要上层原件支持，如tvheadend中需要在适配器设置页面中Power save项打钩   
本源码信号显示由原产品的百分比显示改为分贝显示   
本源码比原产品增加硬件pid filter，减少usb通道带宽占用   
本源码通讯协议通过usb抓包取得，并参考逆向分析原产品驱动程序部分流程   

#### 参考资料
- https://elixir.bootlin.com/linux/v5.15.150/source/drivers/media/usb/dvb-usb-v2/af9035.c
- https://github.com/tbsdtv/linux_media/blob/latest/drivers/media/dvb-frontends/mn88436.c
