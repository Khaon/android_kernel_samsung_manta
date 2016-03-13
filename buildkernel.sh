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

export KERNELDIR=`readlink -f .`;
export PARENT_DIR=`readlink -f ..`;
export ANY_KERNEL=/home/khaon/android/kernel/AnyKernel2;
export ARCH=arm;
export CCACHE_DIR=/home/khaon/caches/.ccache_kernels;
export PACKAGEDIR=/home/khaon/android/kernel/Packages;
export CROSS_COMPILE="ccache /home/khaon/android/rom/aosp_master/prebuilts/gcc/linux-x86/arm/arm-eabi-4.7/bin/arm-eabi-";
export MKBOOTIMG=/home/khaon/android/kernel/mkbootimg_tools/mkboot;
export MKBOOTIMG_TOOLTS_ZIMAGE_MANTA_FOLDER=/home/khaon/android/kernel/mkbootimg_tools/manta_temasek;
echo "${txtbld} Remove old zImage ${txtrst}";

make mrproper;
rm $PACKAGEDIR/zImage;
rm arch/arm/boot/zImage;

echo "${bldblu} Make the kernel ${txtrst}";
make khaon_manta_defconfig;

make -j8;

if [ -e $KERNELDIR/arch/arm/boot/zImage ]; then

	echo " ${bldgrn} Kernel built !! ${txtrst}";

	export curdate=`date "+%m-%d-%Y"`;

	cd $PACKAGEDIR;

	echo "${txtbld} Make AnyKernel flashable archive ${txtrst} "
	echo "";
	rm UPDATE-AnyKernel2-khaon-kernel-manta-marshmallow-v3.4.110-*.zip;
	cd $ANY_KERNEL;
	cp $KERNELDIR/arch/arm/boot/zImage zImage;
	mkdir -p $PACKAGEDIR;
	zip -r9 $PACKAGEDIR/UPDATE-AnyKernel2-khaon-kernel-manta-marshmallow-v3.4.110-"${curdate}".zip * -x README UPDATE-AnyKernel2.zip .git *~;
	cd $KERNELDIR;
else
	echo "KERNEL DID NOT BUILD! no zImage exist"
fi;