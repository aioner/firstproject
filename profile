#TYPE=HI3516a
#TYPE=HI3536
#TYPE=TI368
#TYPE=linux_32
TYPE=linux_64
#TYPE=arm_A8

ifeq ($(TYPE),HI3536)
ARCH:=arm
CROSS_COMPILE_PREFIX=$(ARCH)-hisiv300-linux-
TARGET_DIR:=HI3536
COMPILE_OPTIONS:=-D_ARM_PLATFORM -D_ARM_CODER -mcpu=cortex-a7 -mfloat-abi=softfp -mfpu=neon-vfpv4 -mno-unaligned-access -fno-aggressive-loop-optimizations 
BOOST_LIB:=../../third_party/boost_1_55_0/lib/HI3536
BOOST_MT:=-mt
RESIP_LIB:=
AV_CHECK_SN_INC:=../../third_party/av_check_sn
AV_CHECK_SN_LIB:=
STRIP:=$(CROSS_COMPILE_PREFIX)strip
endif

ifeq ($(TYPE),HI3516a)
ARCH:=arm
CROSS_COMPILE_PREFIX=$(ARCH)-hisiv300-linux-uclibcgnueabi-
TARGET_DIR:=arm
COMPILE_OPTIONS:=-D_ARM_PLATFORM -D_ARM_CODER -D_USE_RTP_SEND_CONTROLLER 
BOOST_LIB:=../../third_party/boost_1_55_0/lib/arm
BOOST_MT:=-mt
RESIP_LIB:=-L../../../third_party/resiprocate-1.9.8/lib/Hi3516a/
AV_CHECK_SN_INC:=../../third_party/av_check_sn
AV_CHECK_SN_LIB:=
STRIP:=$(CROSS_COMPILE_PREFIX)strip
endif

ifeq ($(TYPE),linux_64)
#ARCH:=arm
CROSS_COMPILE_PREFIX=
TARGET_DIR:=linux_64
COMPILE_OPTIONS:= -D_USE_RTP_SEND_CONTROLLER
BOOST_LIB:=../../third_party/boost_1_55_0/lib/gcc_64
BOOST_MT:=
RESIP_LIB:=-L../../../third_party/resiprocate-1.9.8/lib/linux_64/
AV_CHECK_SN_INC:=../../third_party/av_check_sn
AV_CHECK_SN_LIB:=-lav_check_sn
STRIP:=
endif

ifeq ($(TYPE),linux_32)
#ARCH:=arm
CROSS_COMPILE_PREFIX=
TARGET_DIR:=linux_32
COMPILE_OPTIONS:= -D_USE_RTP_SEND_CONTROLLER 
BOOST_LIB:=../../third_party/boost_1_55_0/lib/gcc_32
BOOST_MT:=
RESIP_LIB:=-L../../../third_party/resiprocate-1.9.8/lib/linux_32/
AV_CHECK_SN_INC:=../../third_party/av_check_sn
AV_CHECK_SN_LIB:=-lav_check_sn
STRIP:=
endif

ifeq ($(TYPE),TI368)
ARCH:=arm_v5t_le
CROSS_COMPILE_PREFIX=$(ARCH)-
TARGET_DIR:=TI_DM368
COMPILE_OPTIONS:=-D_ARM_PLATFORM -D_ARM_CODER -DTI_368 -DCLOSE_SESSION 
BOOST_LIB:=../../third_party/boost_1_55_0/lib/arm_v5t
BOOST_MT:=-mt
RESIP_LIB:=../../../third_party/resiprocate-1.9.8/lib/TI_DM368/
AV_CHECK_SN_INC:=../../third_party/av_check_sn
AV_CHECK_SN_LIB:=
STRIP:=$(CROSS_COMPILE_PREFIX)strip
endif

ifeq ($(TYPE),arm_A8)
#ARCH:=arm
CROSS_COMPILE_PREFIX=
TARGET_DIR:=arm_A8
COMPILE_OPTIONS:=-D_ARM_PLATFORM -D_ARM_A8 -D_USE_RTP_SEND_CONTROLLER 
BOOST_LIB:=../../third_party/boost_1_55_0/lib/arm_A8
BOOST_MT:=
RESIP_LIB:=../../../resiprocate-1.9.8/lib/arm_A8/
AV_CHECK_SN_INC:=../../third_party/av_check_sn
AV_CHECK_SN_LIB:=
STRIP:=
endif

THIRD_PATH:=../../third_party
BOOST_INC:=../../third_party/boost_1_55_0

CC=$(CROSS_COMPILE_PREFIX)gcc
CXX=$(CROSS_COMPILE_PREFIX)g++

RELEASE_DIR=RELEASE-$(TYPE)
