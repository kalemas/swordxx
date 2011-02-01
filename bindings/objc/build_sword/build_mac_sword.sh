#!/bin/sh -e
# script by Manfred Bergmann in 2006, updated in 2010
#

APP=sword
VERS=1.6.2
BDIR=`pwd`

DEBUG=0
FAT=0
PPC=0
INTEL=0

SWORDPATH="$BDIR/../../../"
ICUPATH="$BDIR/../dependencies/icu"
PPCCLUCENEPATH="$BDIR/../build_clucene/build/ppc_inst"
INTELCLUCENEPATH="$BDIR/../build_clucene/build/intel_inst"
INTEL64CLUCENEPATH="$BDIR/../build_clucene/build/intel64_inst"

# check commandline
for arg in "$@" 
do
	if [ "$arg" = "debug" ]; then
		DEBUG=1
		echo "building debug version"
	fi
	if [ "$arg" = "fat" ]; then
		FAT=1
		PPC=1
		INTEL=1
		echo "building fat version"
	fi
	if [ "$arg" = "ppc" ]; then
		PPC=1
		echo "building ppc version"
	else
		PPC=0
	fi
	if [ "$arg" = "intel" ]; then
		INTEL=1
		echo "building intel version"
	else
		INTEL=0
	fi
done

# using seperate build dirs and building in them doesn't work with sword
BUILD=$BDIR/build
PPCPREFIX=$BUILD/ppc_inst
INTELPREFIX=$BUILD/intel_inst
INTEL64PREFIX=$BUILD/intel64_inst
RESULTPREFIX=$BUILD/result_inst

# Create install dirs if they doesn't exist
if [ ! -d $BUILD ]; then
	mkdir -p $BUILD
fi
if [ ! -d $PPCPREFIX ]; then
	mkdir -p $PPCPREFIX
fi
if [ ! -d $INTELPREFIX ]; then
	mkdir -p $INTELPREFIX
fi
if [ ! -d $INTEL64PREFIX ]; then
	mkdir -p $INTEL64PREFIX
fi
if [ ! -d $RESULTPREFIX ]; then
	mkdir -p $RESULTPREFIX
	if [ ! -d $RESULTPREFIX/lib ]; then
		mkdir -p $RESULTPREFIX/lib
	fi
fi

# add icu tools to path
export PATH="$PATH:$ICUPATH/bin"
export DYLD_LIBRARY_PATH="$DYLD_LIBRARY_PATH:$ICUPATH/lib"

PPC_LIB_EXPORT=
INTEL_LIB_EXPORT=
INTEL64_LIB_EXPORT=

# build stuff
if [ $PPC -eq 1 ] || [ $FAT -eq 1 ]; then
    echo "building PPC version of library..."    
	cd $SWORDPATH
	echo 'autogen.sh ...'
	./autogen.sh
	echo 'autogen.sh ... done'
	export CC=gcc
	export CXX=g++
	export SDK=/Developer/SDKs/MacOSX10.5.sdk
	if [ $DEBUG -eq 1 ]; then
		export CFLAGS="-O0 -g -arch ppc -mmacosx-version-min=10.5 -isysroot $SDK -I$SDK/usr/include -I$ICUPATH/include"
	else
	    export CFLAGS="-O2 -g0 -arch ppc -mmacosx-version-min=10.5 -isysroot $SDK -I$SDK/usr/include -I$ICUPATH/include"
	fi
	export CXXFLAGS="$CFLAGS"
	export LDFLAGS="-isysroot $SDK -Wl,-syslibroot,$SDK"
	./configure --prefix=$PPCPREFIX --with-clucene=$PPCCLUCENEPATH --with-zlib --with-conf --with-icu --with-curl --disable-tests --disable-shared --enable-utilities
	make all install
	make clean
	cd $BDIR
	# copy to result dir
	PPC_LIB_EXPORT="$RESULTPREFIX/lib/lib$APP-ppc.a"
	cp $PPCPREFIX/lib/lib$APP.a $PPC_LIB_EXPORT
    echo "building PPC version of library...done"
fi

if [ $INTEL -eq 1 ] || [ $FAT -eq 1 ]; then
    echo "building INTEL version of library..."
	cd $SWORDPATH
	echo 'autogen.sh ...'
	./autogen.sh
	echo 'autogen.sh ... done'
	export CC=gcc
	export CXX=g++
	export SDK=/Developer/SDKs/MacOSX10.5.sdk
	if [ $DEBUG -eq 1 ]; then
		export CFLAGS="-O0 -g -arch i686 -mmacosx-version-min=10.5 -isysroot $SDK -I$SDK/usr/include -I$ICUPATH/include"
	else
	    export CFLAGS="-O2 -g0 -arch i686 -mmacosx-version-min=10.5 -isysroot $SDK -I$SDK/usr/include -I$ICUPATH/include"
	fi
	export CXXFLAGS="$CFLAGS"
	export LDFLAGS="-isysroot $SDK -Wl,-syslibroot,$SDK"
	./configure --prefix=$INTELPREFIX --with-clucene=$INTELCLUCENEPATH --with-zlib --with-conf --with-icu --with-curl --enable-tests --disable-shared --enable-utilities
	make all install
	make clean
	cd $BDIR
	# copy to result dir
	INTEL_LIB_EXPORT="$RESULTPREFIX/lib/lib$APP-intel.a"
	cp $INTELPREFIX/lib/lib$APP.a $INTEL_LIB_EXPORT
    echo "building INTEL version of library...done"

    echo "building INTEL64 version of library..."
	cd $SWORDPATH
	make clean
	echo 'autogen.sh ...'
	./autogen.sh
	echo 'autogen.sh ... done'
	export CC=gcc
	export CXX=g++
	export SDK=/Developer/SDKs/MacOSX10.5.sdk
	if [ $DEBUG -eq 1 ]; then
		export CFLAGS="-O0 -g -arch x86_64 -mmacosx-version-min=10.5 -isysroot $SDK -I$SDK/usr/include -I$ICUPATH/include"
	else
	    export CFLAGS="-O2 -g0 -arch x86_64 -mmacosx-version-min=10.5 -isysroot $SDK -I$SDK/usr/include -I$ICUPATH/include"
	fi
	export CXXFLAGS="$CFLAGS"
	export LDFLAGS="-isysroot $SDK -Wl,-syslibroot,$SDK"
	./configure --prefix=$INTEL64PREFIX --with-clucene=$INTEL64CLUCENEPATH --with-zlib --with-conf --with-icu --with-curl --enable-tests --disable-shared --enable-utilities
	make all install
	make clean
	cd $BDIR
	# copy to result dir
	INTEL64_LIB_EXPORT="$RESULTPREFIX/lib/lib$APP-intel64.a"
	cp $INTEL64PREFIX/lib/lib$APP.a $INTEL64_LIB_EXPORT
    echo "building INTEL64 version of library...done"
fi

# only for fat version
if [ $FAT -eq 1 ]; then
	# creating result
	# build fat binary with lipo
	lipo -create $PPC_LIB_EXPORT $INTEL_LIB_EXPORT $INTEL64_LIB_EXPORT -output $RESULTPREFIX/lib/lib$APP-ub.a
fi

# run runlib to update the library content
#ranlib $RESULTPREFIX/lib/*