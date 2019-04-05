#!/bin/sh

if [ "$#" -eq 1 -a -n $1 ]
then
BUILD_TARGET=$1
else
BUILD_TARGET=esxi-670
fi

# Use gcc 
# Below is the internal VMWare location.  Please change as required for your
# installed location.
CC=/build/toolchain/lin64/gcc-4.8.0/bin/gcc

# Use ld from binutils
# Below is the internal VMWare location.  Please change as required for your
# installed location.
LD=/build/toolchain/lin64/binutils-2.19.1/x86_64-linux5.0/bin/ld
# Use binary strip
STRIP=/build/toolchain/lin64/binutils-2.19.1/x86_64-linux5.0/bin/strip
# PR# 976913 requested that OSS binaries be stripped of debug
STRIP_OPTS=--strip-debug

SYS_ROOT=/nowhere

I_SYSTEM=/build/toolchain/lin64/gcc-4.8.0/lib/gcc/x86_64-linux5.0/4.8.0/include

# Use GNU grep 2.5.1
GREP=grep
# Use GNU sed 4.5.1
SED=sed
# Use GNU xargs 4.2.27
XARGS=xargs
# Use mkdir from GNU coreutils 5.97
MKDIR=mkdir

# Create output directories
#$GREP -v -e "SED" build-*.sh \
#| $GREP -o -e "-o [^ ]*\."            \
#| $SED -e 's?-o \(.*\)/[^/]*\.?\1?'   \
#| $GREP -v -e "\*"                    \
#| $XARGS $MKDIR -p

SRC_PATH=atlantic
OUT_PATH=.build
OBJ_PATH=$OUT_PATH/objs
BIN_PATH=$OUT_PATH/bin
mkdir $OUT_PATH
mkdir $OBJ_PATH
mkdir $OBJ_PATH/hw_atl
mkdir $BIN_PATH
# Compiler flags assume being compiled natively on a x86-64 machine

FLAGS="--sysroot=$SYS_ROOT -fwrapv -pipe -fno-strict-aliasing -Wno-unused-but-set-variable "
FLAGS=$FLAGS"-fno-working-directory -g -ggdb3 -O2 -mcmodel=smallhigh -Wall -Werror -Wstrict-prototypes "
FLAGS=$FLAGS"-fno-strict-aliasing -freg-struct-return -falign-jumps=1 -falign-functions=4 -falign-loops=1 "
FLAGS=$FLAGS"-m64 -mno-red-zone -mpreferred-stack-boundary=4 -minline-all-stringops -mno-mmx -mno-3dnow -mno-sse -mno-sse2 "
FLAGS=$FLAGS"-mcld -finline-limit=2000 -fno-common -ffreestanding -nostdinc -fomit-frame-pointer -nostdlib "
FLAGS=$FLAGS"-Wno-error -Wdeclaration-after-statement -Wno-pointer-sign -Wno-strict-prototypes "
FLAGS=$FLAGS"-Wno-enum-compare -Wno-switch "
FLAGS=$FLAGS"-DCONFIG_COMPAT -DCONFIG_DCB -DCONFIG_FCOE -DCONFIG_INET_LRO -DCONFIG_NETDEVICES_MULTIQUEUE -DCONFIG_PCI_IOV "
FLAGS=$FLAGS"-DCONFIG_PCI_MSI -DCONFIG_PROC_FS -DCPU=x86-64 -DDRIVER_ATLANTIC -DESX3_NETWORKING_NOT_DONE_YET -DGPLED_CODE "
FLAGS=$FLAGS"-DHAVE_DCBNL_OPS_GETAPP -DHAVE_IPLINK_VF_CONFIG -DATLANTIC_ESX_CNA -DATLANTIC_MQ -DATLANTIC_NO_LRO -DATLANTIC_VMDQ "
FLAGS=$FLAGS"-DCONFIG_PCI "
FLAGS=$FLAGS"-DKBUILD_MODNAME=\"atlantic\" -DLINUX_MODULE_AUX_HEAP_NAME=vmklnx_atlantic "
FLAGS=$FLAGS"-DLINUX_MODULE_HEAP_NAME=vmklnx_atlantic -DLINUX_MODULE_HEAP_INITIAL=1024*100 "
FLAGS=$FLAGS"-DLINUX_MODULE_HEAP_MAX=64*1024*1024 "
FLAGS=$FLAGS"-DLINUX_MODULE_SKB_HEAP "
FLAGS=$FLAGS"-DLINUX_MODULE_SKB_HEAP_INITIAL=512*1024 -DLINUX_MODULE_SKB_HEAP_MAX=128*1024*1024 -DLINUX_MODULE_VERSION=\"3.7.13.7.14iov\" "
FLAGS=$FLAGS"-DMODULE -DNET_DRIVER -DNO_FLOATING_POINT -DVMKERNEL -DVMKERNEL_MODULE -DVMKLINUX_MODULE_HEAP_ANY_MEM "
FLAGS=$FLAGS"-DVMK_DEVKIT_HAS_API_VMKAPI_BASE -DVMK_DEVKIT_HAS_API_VMKAPI_DEVICE -DVMK_DEVKIT_HAS_API_VMKAPI_ISCSI -DVMK_DEVKIT_HAS_API_VMKAPI_NET "
FLAGS=$FLAGS"-DVMK_DEVKIT_HAS_API_VMKAPI_RDMA -DVMK_DEVKIT_HAS_API_VMKAPI_SCSI -DVMK_DEVKIT_HAS_API_VMKAPI_SOCKETS -DVMK_DEVKIT_IS_DDK "
FLAGS=$FLAGS"-DVMK_DEVKIT_USES_BINARY_COMPATIBLE_APIS -DVMK_DEVKIT_USES_PUBLIC_APIS "
FLAGS=$FLAGS"-DVMNIX -DVMX86_RELEASE -DVMX86_SERVER -D_LINUX "
FLAGS=$FLAGS"-D__KERNEL__ -D__VMKERNEL_MODULE__ -D__VMKERNEL__ -D__VMKLNX__ -D__VMK_GCC_BUG_ALIGNMENT_PADDING__ "
FLAGS=$FLAGS"-D__VMWARE__ -isystem $I_SYSTEM "
FLAGS=$FLAGS"-Wno-unused-function "

INCLUDES="-I$BUILD_TARGET/BLD/build/version "
INCLUDES=$INCLUDES"-I$BUILD_TARGET/BLD/build/HEADERS/vmkdrivers-vmkernel/vmkernel64/release "
INCLUDES=$INCLUDES"-I$BUILD_TARGET/BLD/build/HEADERS/92-vmkdrivers-asm-x64/vmkernel64/release "
INCLUDES=$INCLUDES"-I$BUILD_TARGET/BLD/build/HEADERS/vmkapi-v2_3_0_0-all-public-bincomp/generic/release "
INCLUDES=$INCLUDES"-I$BUILD_TARGET/BLD/build/HEADERS/vmkapi-current-all-public-bincomp/generic/release "
INCLUDES=$INCLUDES"-I$SRC_PATH "
INCLUDES=$INCLUDES"-I$BUILD_TARGET/vmkdrivers/src_92/include "
INCLUDES=$INCLUDES"-I$BUILD_TARGET/vmkdrivers/src_92/include/vmklinux_92 "
INCLUDES=$INCLUDES"-I$BUILD_TARGET/vmkdrivers/src_92/drivers/net "
INCLUDES=$INCLUDES"-include $BUILD_TARGET/vmkdrivers/src_92/include/linux/autoconf.h "


$CC $FLAGS $INCLUDES -c -o $OBJ_PATH/aq_main.o $SRC_PATH/aq_main.c
$CC $FLAGS $INCLUDES -c -o $OBJ_PATH/aq_drvinfo.o $SRC_PATH/aq_drvinfo.c
$CC $FLAGS $INCLUDES -c -o $OBJ_PATH/aq_ethtool.o $SRC_PATH/aq_ethtool.c
$CC $FLAGS $INCLUDES -c -o $OBJ_PATH/aq_filters.o $SRC_PATH/aq_filters.c
$CC $FLAGS $INCLUDES -c -o $OBJ_PATH/aq_hw_utils.o $SRC_PATH/aq_hw_utils.c
$CC $FLAGS $INCLUDES -c -o $OBJ_PATH/aq_nic.o $SRC_PATH/aq_nic.c
$CC $FLAGS $INCLUDES -c -o $OBJ_PATH/aq_pci_func.o $SRC_PATH/aq_pci_func.c
$CC $FLAGS $INCLUDES -c -o $OBJ_PATH/aq_phy.o $SRC_PATH/aq_phy.c
$CC $FLAGS $INCLUDES -c -o $OBJ_PATH/aq_ring.o $SRC_PATH/aq_ring.c
$CC $FLAGS $INCLUDES -c -o $OBJ_PATH/aq_vec.o $SRC_PATH/aq_vec.c
$CC $FLAGS $INCLUDES -c -o $OBJ_PATH/hw_atl/hw_atl_b0.o $SRC_PATH/hw_atl/hw_atl_b0.c
$CC $FLAGS $INCLUDES -c -o $OBJ_PATH/hw_atl/hw_atl_llh.o $SRC_PATH/hw_atl/hw_atl_llh.c
$CC $FLAGS $INCLUDES -c -o $OBJ_PATH/hw_atl/hw_atl_utils.o $SRC_PATH/hw_atl/hw_atl_utils.c
$CC $FLAGS $INCLUDES -c -o $OBJ_PATH/hw_atl/hw_atl_utils_fw2x.o $SRC_PATH/hw_atl/hw_atl_utils_fw2x.c
$CC $FLAGS $INCLUDES -c -o $OBJ_PATH/vmklinux_module.o $BUILD_TARGET/vmkdrivers/src_92/common/vmklinux_module.c

$LD $LD_OPTS -r -o \
    $BIN_PATH/atlantic --whole-archive \
    $OBJ_PATH/aq_main.o \
    $OBJ_PATH/aq_drvinfo.o \
    $OBJ_PATH/aq_ethtool.o \
    $OBJ_PATH/aq_filters.o \
    $OBJ_PATH/aq_hw_utils.o \
    $OBJ_PATH/aq_nic.o \
    $OBJ_PATH/aq_pci_func.o \
    $OBJ_PATH/aq_phy.o \
    $OBJ_PATH/aq_ring.o \
    $OBJ_PATH/aq_vec.o \
    $OBJ_PATH/hw_atl/hw_atl_b0.o \
    $OBJ_PATH/hw_atl/hw_atl_llh.o \
    $OBJ_PATH/hw_atl/hw_atl_utils.o \
    $OBJ_PATH/hw_atl/hw_atl_utils_fw2x.o \
    $OBJ_PATH/vmklinux_module.o
$STRIP $STRIP_OPTS $BIN_PATH/atlantic
