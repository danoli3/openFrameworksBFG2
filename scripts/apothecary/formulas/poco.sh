#! /bin/bash
#
# Poco
# C++ with Batteries Included
# http://pocoproject.org/
#
# uses an autotools build system,
# specify specfic build configs in poco/config using ./configure --config=NAME

# define the version
VER=1.5.3-release

# tools for git use
GIT_URL=https://github.com/pocoproject/poco
GIT_TAG=poco-$VER

# For Poco Builds, we omit both Data/MySQL and Data/ODBC because they require
# 3rd Party libraries.  See https://github.com/pocoproject/poco/blob/develop/README
# for more information.


# @tgfrerer: we need more fine-grained control over poco source code versions, 
# which is why we specify the specific poco source code commit we want to use -
# When updating poco/this recipe, make sure to specify the proper hash here,
# and check if the header patches still apply cleanly.

# @TODO: New commit ref?
SHA=

# download the source code and unpack it into LIB_NAME
function download() {
	if [ "$SHA" == "" ] ; then
		curl -Lk $GIT_URL/archive/$GIT_TAG.tar.gz -o poco-$GIT_TAG.tar.gz
		tar -xf poco-$GIT_TAG.tar.gz
		mv poco-$GIT_TAG poco
		rm poco*.tar.gz
	else
		git clone $GIT_URL -b poco-$VER
	fi
}

# prepare the build environment, executed inside the lib src dir
function prepare() {
	if [ "$SHA" != "" ] ; then
		git reset --hard $SHA
	fi

	# make backups of the ios config files since we need to edit them
	if [ "$TYPE" == "ios" ] ; then
		cd build/config
		cp iPhone iPhone.orig
		cp iPhoneSimulator iPhoneSimulator.orig
	fi
}

# executed inside the lib src dir
function build() {

	if [ "$TYPE" == "osx" ] ; then
		local BUILD_OPTS="--no-tests --no-samples --static --omit=Data/MySQL,Data/ODBC"
		
		# 32 bit
		# For OS 10.9+ we must explicitly set libstdc++ for the 32-bit OSX build.
		./configure $BUILD_OPTS --cflags=-stdlib=libstdc++ --config=Darwin32
		make

		# 64 bit
		./configure $BUILD_OPTS --config=Darwin64-clang-libc++
		make

		cd lib/Darwin

		# delete debug builds
		rm i386/*d.a
		rm x86_64/*d.a

		# link into universal lib, strip "lib" from filename
		local lib
		for lib in $( ls -1 i386) ; do
			local renamedLib=$(echo $lib | sed 's|lib||')
			if [ ! -e $renamedLib ] ; then
				lipo -c i386/$lib x86_64/$lib -o $renamedLib
			fi
		done

	elif [ "$TYPE" == "vs" ] ; then
		#cmd.exe /c "buildwin.cmd "$VS_VER" build all both Win32 nosamples devenv"
		## OR
		cmake -G "Visual Studio $VS_VER" -DPOCO_STATIC="1" -DCMAKE_DEBUG_POSTFIX="mdd" -DCMAKE_RELEASE_POSTFIX="md"
		vs-build "Poco.sln" Build Debug
		vs-build "Poco.sln" Build Release
	
	elif [ "$TYPE" == "ios" ] ; then

		# maybe --poquito is a good idea?
		local BUILD_OPTS="--no-tests --no-samples --static --omit=Data/MySQL,Data/ODBC,NetSSL_OpenSSL,Crypto"

		cp build/config/iPhone.orig build/config/iPhone
		cp build/config/iPhoneSimulator.orig build/config/iPhoneSimulator

		# set SDK
		sed -i .tmp "s|#.*IPHONE_SDK_VERSION =.*|IPHONE_SDK_VERSION = $IOS_SDK_VER|" build/config/iPhone

		# fix any xcode path issues (currently fixed in newer poco, done here manually for now)
		sed -i .tmp 's|= /Developer|= $(shell xcode-select -print-path)|' build/config/iPhone
		sed -i .tmp 's|= /Developer|= $(shell xcode-select -print-path)|' build/config/iPhoneSimulator

		# armv7
		sed -i .tmp "s|POCO_TARGET_OSARCH.*?=.*|POCO_TARGET_OSARCH ?= armv7|" build/config/iPhone
		./configure $BUILD_OPTS --config=iPhone
		make

		# armv7s
		sed -i .tmp "s|POCO_TARGET_OSARCH.*?=.*|POCO_TARGET_OSARCH ?= armv7s|" build/config/iPhone
		./configure $BUILD_OPTS --config=iPhone
		make

		# simulator
		sed -i .tmp "s|POCO_TARGET_OSARCH.* = .*|POCO_TARGET_OSARCH = i386|" build/config/iPhoneSimulator
		./configure $BUILD_OPTS --config=iPhoneSimulator
		make

		cd lib/iPhoneOS

		# delete debug builds
		rm armv7/*d.a armv7s/*d.a ../iPhoneSimulator/i386/*d.a

		# link into universal lib, strip "lib" from filename
		local lib
		for lib in $( ls -1 ../iPhoneSimulator/i386) ; do
			local renamedLib=$(echo $lib | sed 's|lib||')
			if [ ! -e $renamedLib ] ; then
				lipo -c armv7/$lib armv7s/$lib ../iPhoneSimulator/i386/$lib -o $renamedLib
			fi
		done

	elif [ "$TYPE" == "android" ] ; then
		echoWarning "TODO: android build"

	elif [ "$TYPE" == "linux" ] ; then
		local BUILD_OPTS="--no-tests --no-samples --static --omit=Data/MySQL,Data/ODBC"
		./configure $BUILD_OPTS
		make
		# delete debug builds
		rm lib/Linux/$(uname -m)/*d.a
	elif [ "$TYPE" == "linux64" ] ; then
		local BUILD_OPTS="--no-tests --no-samples --static --omit=Data/MySQL,Data/ODBC"
		./configure $BUILD_OPTS
		make
		# delete debug builds
		rm lib/Linux/x86_64/*d.a
	elif [ "$TYPE" == "linuxarmv6l" ] ; then
		local BUILD_OPTS="--no-tests --no-samples --static --omit=Data/MySQL,Data/ODBC"
		./configure $BUILD_OPTS
		make
		# delete debug builds
		rm lib/Linux/armv6l/*d.a
	elif [ "$TYPE" == "linuxarmv7l" ] ; then
		local BUILD_OPTS="--no-tests --no-samples --static --omit=Data/MySQL,Data/ODBC"
		./configure $BUILD_OPTS
		make
		# delete debug builds
		rm lib/Linux/armv7l/*d.a
	else 
		echoWarning "TODO: build $TYPE lib"
	fi
}

# executed inside the lib src dir, first arg $1 is the dest libs dir root
function copy() {

	# headers
	mkdir -p $1/include/Poco
	cp -Rv Crypto/include/Poco/Crypto $1/include/Poco
	cp -Rv CppParser/include/Poco/CppParser $1/include/Poco
	cp -Rv Data/include/Poco/Data/ $1/include/Poco
	cp -Rv Data/SQLite/include/Poco/Data/SQLite $1/include/Poco/Data
	cp -Rv Foundation/include/Poco/* $1/include/Poco
	cp -Rv JSON/include/Poco/JSON $1/include/Poco
	cp -Rv MongoDB/include/Poco/MongoDB $1/include/Poco
	cp -Rv Net/include/Poco/Net $1/include/Poco
	cp -Rv NetSSL_OpenSSL/include/Poco/Net/* $1/include/Poco/Net
	cp -Rv PDF/include/Poco/PDF $1/include/Poco
	cp -Rv SevenZip/include/Poco/SevenZip $1/include/Poco
	cp -Rv Util/include/Poco/Util $1/include/Poco
	cp -Rv XML/include/Poco/* $1/include/Poco
	cp -Rv Zip/include/Poco/Zip $1/include/Poco

	# libs
	if [ "$TYPE" == "osx" ] ; then		
		mkdir -p $1/lib/$TYPE
		cp -v lib/Darwin/*.a $1/lib/$TYPE

	elif [ "$TYPE" == "ios" ] ; then
		mkdir -p $1/lib/$TYPE
		cp -v lib/iPhoneOS/*.a $1/lib/$TYPE

	elif [ "$TYPE" == "vs" ] ; then
		mkdir -p $1/lib/$TYPE
		cp -v lib/Release/*.lib $1/lib/$TYPE
		cp -v lib/Debug/*.lib $1/lib/$TYPE
	elif [ "$TYPE" == "linux" ] ; then
		mkdir -p $1/lib/$TYPE
		cp -v lib/Linux/$(uname -m)/*.a $1/lib/$TYPE
	elif [ "$TYPE" == "linux64" ] ; then
		mkdir -p $1/lib/$TYPE
		cp -v lib/Linux/x86_64/*.a $1/lib/$TYPE
	elif [ "$TYPE" == "linuxarmv6l" ] ; then
		mkdir -p $1/lib/$TYPE
		cp -v lib/Linux/armv6l/*.a $1/lib/$TYPE
	elif [ "$TYPE" == "linuxarmv7l" ] ; then
		mkdir -p $1/lib/$TYPE
		cp -v lib/Linux/armv7l/*.a $1/lib/$TYPE
	else
		echoWarning "TODO: copy $TYPE lib"
	fi
}

# executed inside the lib src dir
function clean() {

	if [ "$TYPE" == "vs" ] ; then
		echoWarning "TODO: clean vs"
	
	elif [ "$TYPE" == "android" ] ; then
		echoWarning "TODO: clean android"
	
	else
		make clean
	fi
}
