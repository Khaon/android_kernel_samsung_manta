#!/bin/sh

# Colorize and add text parameters
red=$(tput setaf 1) # red
grn=$(tput setaf 2) # green
cya=$(tput setaf 6) # cyan
txtbld=$(tput bold) # Bold
bldred=${txtbld}$(tput setaf 1) # red
bldgrn=${txtbld}$(tput setaf 2) # green
bldblu=${txtbld}$(tput setaf 4) # blue
bldcya=${txtbld}$(tput setaf 6) # cyan
txtrst=$(tput sgr0) # Reset

export KERNELDIR=`readlink -f .`
export PARENT_DIR=`readlink -f ..`
export INITRAMFS_F2FS=/home/khaon/Documents/kernels/Ramdisks/CM12_MANTA_F2FS
export INITRAMFS_EXT4=/home/khaon/Documents/kernels/Ramdisks/CM12_MANTA_EXT4
export PACKAGEDIR=/home/khaon/Documents/kernels/Packages/AOSP_Manta
export ZIP_TEMPLATE=/home/khaon/Documents/kernels/Packages/META-INF/Manta
export ANY_KERNEL=/home/khaon/kernels/AnyKernel2
#Enable FIPS mode
export USE_SEC_FIPS_MODE=true
export ARCH=arm
export CCACHE_DIR=/home/khaon/caches/.ccache_kernels
export CROSS_COMPILE=/home/khaon/Documents/Toolchains/arm-eabi-4.8/bin/arm-eabi-

echo "${txtbld} Remove old Package Files ${txtrst}"
rm -rf $PACKAGEDIR/*

echo "${txtbld} Setup Package Directory ${txtrst}"
mkdir -p $PACKAGEDIR/system/lib/modules
mkdir -p $PACKAGEDIR/system/etc/init.d

echo "${txtbld} Remove old zImage ${txtrst}"
make mrproper
rm $PACKAGEDIR/zImage
rm arch/arm/boot/zImage

echo "${bldblu} Make the kernel ${txtrst}"
make khaon_manta_defconfig

make -j12

echo "${txtbld} Copy modules to Package ${txtrst} "
cp -a $(find . -name *.ko -print |grep -v initramfs) $PACKAGEDIR/system/lib/modules

if [ -e $KERNELDIR/arch/arm/boot/zImage ]; then

	echo " ${bldgrn} Kernel built !! ${txtrst}"

	export curdate=`date "+%m-%d-%Y"`

	cd $PACKAGEDIR

	echo "${txtbld} Make AnyKernel flashable archive ${txtrst} "
	echo ""
	rm ../UPDATE-AnyKernel2-khaon-kernel-manta-*.zip
	cd $ANY_KERNEL
	git clean -fdx; git reset --hard; git checkout manta;
	cp $KERNELDIR/arch/arm/boot/zImage zImage
	zip -r9 $PACKAGEDIR/../UPDATE-AnyKernel2-khaon-kernel-manta-"${curdate}".zip * -x README UPDATE-AnyKernel2.zip .git *~
	cd $KERNELDIR
else
	echo "KERNEL DID NOT BUILD! no zImage exist"
fi;

