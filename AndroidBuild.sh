#!/bin/sh


LOCAL_PATH=`dirname $0`
LOCAL_PATH=`cd $LOCAL_PATH && pwd`

JOBS=1

#ln -sf libsdl-1.2.so $LOCAL_PATH/../../../obj/local/armeabi/libSDL.so
#ln -sf libsdl-1.2.so $LOCAL_PATH/../../../obj/local/armeabi-v7a/libSDL.so
#ln -sf libsdl-1.2.so $LOCAL_PATH/../../../obj/local/mips/libSDL.so
#ln -sf libsdl-1.2.so $LOCAL_PATH/../../../obj/local/x86/libSDL.so

#../setEnvironment.sh sh -c "make -j1 " && mv -f uae4all libapplication.so

#if [ "$1" = armeabi ]; then
#../setEnvironment.sh sh -c "make --makefile=Makefile.arm -j1" && mv -f uae4all libapplication.so
#fi
#if [ "$1" = armeabi-v7a ]; then
#env CFLAGS="-funsafe-math-optimizations -Ofast" \
#../setEnvironment-armeabi-v7a.sh sh -c "make --makefile=Makefile.arm-v7a -j$JOBS" && mv -f uae4all-v7a libapplication-armeabi-v7a.so
#fi
#if [ "$1" = armeabi-v7a-hard ]; then
#env CFLAGS="-funsafe-math-optimizations -O2" \
#../setEnvironment-armeabi-v7a-hard.sh sh -c "make --makefile=Makefile.arm-v7a-hard -j$JOBS" && mv -f uae4all-v7a-hard libapplication-armeabi-v7a-hard.so
#fi
if [ "$1" = armeabi-v7a ]; then
env CFLAGS="-mfpu=neon -funsafe-math-optimizations -Ofast" \
../setEnvironment-armeabi-v7a.sh sh -c "make --makefile=Makefile.arm-v7a-neon -j$JOBS" && mv -f uae4all-v7a-neon libapplication-armeabi-v7a.so
fi
#if [ "$1" = armeabi-v7a-hard ]; then
#env CFLAGS="-mfpu=neon -funsafe-math-optimizations" \
#../setEnvironment-armeabi-v7a-hard.sh sh -c "make --makefile=Makefile.arm-v7a-neon-hard -j$JOBS" && mv -f uae4all-v7a-neon-hard libapplication-armeabi-v7a-hard.so
#fi
if [ "$1" = x86 ]; then
env CFLAGS="" \
../setEnvironment-x86.sh sh -c "make --makefile=Makefile.x86 -j$JOBS" && mv -f uae4all-x86 libapplication-x86.so
fi
#if [ "$1" = mips ]; then
#../setEnvironment-mips.sh sh -c "make --makefile=Makefile.mips -j1" && mv -f uae4all-mips libapplication-mips.so
#fi