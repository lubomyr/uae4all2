#!/bin/sh


LOCAL_PATH=`dirname $0`
LOCAL_PATH=`cd $LOCAL_PATH && pwd`

JOBS=1

#ln -sf libsdl-1.2.so $LOCAL_PATH/../../../obj/local/armeabi/libSDL.so
#ln -sf libsdl-1.2.so $LOCAL_PATH/../../../obj/local/armeabi-v7a/libSDL.so
#ln -sf libsdl-1.2.so $LOCAL_PATH/../../../obj/local/mips/libSDL.so
#ln -sf libsdl-1.2.so $LOCAL_PATH/../../../obj/local/x86/libSDL.so

[ -e src-v7a-neon ] || ./copy_src2v7a-neon.sh
[ -e src-x86 ] || ./copy_src2x86.sh
#[ -e src-v7a ] || ./copy_src2v7a.sh
#[ -e src-v7a-hard ] || ./copy_src2v7a-hard.sh
#[ -e src-v7a-neon-hard ] || ./copy_src2v7a-neon-hard.sh
#[ -e src-mips ] || ./copy_src2xmips.sh

#../setEnvironment.sh sh -c "make -j$JOBS " && mv -f uae4all libapplication.so

#if [ "$1" = armeabi ]; then
#../setEnvironment.sh sh -c "make --makefile=Makefile.arm -j$JOBS" && mv -f uae4all libapplication.so
#fi
#if [ "$1" = armeabi-v7a ]; then
#../setEnvironment-armeabi-v7a.sh sh -c "make --makefile=Makefile.arm-v7a -j$JOBS" && mv -f uae4all-v7a libapplication-armeabi-v7a.so
#fi
#if [ "$1" = armeabi-v7a-hard ]; then
#../setEnvironment-armeabi-v7a-hard.sh sh -c "make --makefile=Makefile.arm-v7a-hard -j$JOBS" && mv -f uae4all-v7a-hard libapplication-armeabi-v7a-hard.so
#fi
if [ "$1" = armeabi-v7a ]; then
env CFLAGS="-mfpu=neon -funsafe-math-optimizations -ffast-math" \
../setEnvironment-armeabi-v7a.sh sh -c "make --makefile=Makefile.arm-v7a-neon -j$JOBS" && mv -f uae4all-v7a-neon libapplication-armeabi-v7a.so || exit 1
fi
#if [ "$1" = armeabi-v7a-hard ]; then
#../setEnvironment-armeabi-v7a-neon-hard.sh sh -c "make --makefile=Makefile.arm-v7a-neon-hard -j$JOBS" && mv -f uae4all-v7a-neon-hard libapplication-armeabi-v7a-hard.so
#fi
if [ "$1" = x86 ]; then
env CFLAGS="-ffast-math" \
../setEnvironment-x86.sh sh -c "make --makefile=Makefile.x86 -j$JOBS" && mv -f uae4all-x86 libapplication-x86.so || exit 1
fi
#if [ "$1" = mips ]; then
#../setEnvironment-mips.sh sh -c "make --makefile=Makefile.mips -j$JOBS" && mv -f uae4all-mips libapplication-mips.so
#fi
