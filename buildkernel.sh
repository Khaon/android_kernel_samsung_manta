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
export ANY_KERNEL=/mnt/sdb3/Documents/kernels/AnyKernel2
export ARCH=arm
export CCACHE_DIR=/home/khaon/caches/.ccache_kernels
export PACKAGEDIR=/home/khaon/Documents/kernels/Packages/AOSP_Manta
export CROSS_COMPILE=/mnt/sdb3/android/optiPop/prebuilts/gcc/linux-x86/arm/arm-eabi-4.8/bin/arm-eabi-

echo "${txtbld} Remove old zImage ${txtrst}"
make mrproper
rm $PACKAGEDIR/zImage
rm arch/arm/boot/zImage

echo "${bldblu} Make the kernel ${txtrst}"
make khaon_manta_defconfig

make -j8

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
    mkdir -p $PACKAGEDIR
	zip -r9 $PACKAGEDIR/../UPDATE-AnyKernel2-khaon-kernel-manta-"${curdate}".zip * -x README UPDATE-AnyKernel2.zip .git *~
	cd $KERNELDIR
else
	echo "KERNEL DID NOT BUILD! no zImage exist"
fi;

