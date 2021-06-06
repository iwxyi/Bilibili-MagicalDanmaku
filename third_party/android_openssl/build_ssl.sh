#!/bin/bash

BUILD_DIR=$(pwd)
DEFAULT_PATH=$PATH

declare -a params=( 'no-asm' '' )
declare -A ssl_versions=( ["1.1.1k"]=~/android/ndk/21.4.7075529 ["1.0.2u"]=~/android/android-ndk-r10e )

# Qt up to Qt-5.12.3 are using openssl 1.0.x
# Qt 5.12.4 and 5.13.0 are using 1.1.x without any suffix, this will make he andorid linker to load system libs on Andorid 5 which will make Qt SSL support unusable in this combination
# Qt 5.12.5+ and 5.13.1+ are using OpenSSL 1.1.x but the library is suffixed with _1_1.so
# Qt 5.14.0+ same as above, but the static ssl libs are stored in a multiabi folder

declare -A qt_versions=( ["latest"]="1.1*" ["Qt-5.12.3"]="1.0*" ["Qt-5.12.4_5.13.0"]="1.1*" )
declare -A qt_architectures=( ["x86_64"]="x86_64" ["x86"]="x86" ["arm64"]="arm64-v8a" ["arm"]="armeabi-v7a" )
declare hosts=( "linux-x86_64" "linux-x86" "darwin-x86_64" "darwin-x86" )

for param in ${!params[@]}
do
    if [ ${params[$param]} ]; then
        rm -fr ${params[$param]}
        mkdir ${params[$param]}
        pushd ${params[$param]}
    fi
    rm -fr static
    mkdir -p static/lib/multiabi
    mkdir -p static/include
    for ssl_version in ${!ssl_versions[@]}
    do
        echo "SSL version = $ssl_version"
        if [ ! -f "openssl-$ssl_version.tar.gz" ]; then
            wget https://www.openssl.org/source/openssl-$ssl_version.tar.gz
        fi
        export ANDROID_NDK_HOME="${ssl_versions[$ssl_version]}"

        for qt_version in ${!qt_versions[@]}
        do
            if [[ $ssl_version != ${qt_versions[$qt_version]} ]]; then
                continue;
            fi
            echo "Build $ssl_version for $qt_version"
            for arch in ${!qt_architectures[@]}
            do
                rm -fr $arch $qt_version/$arch
                mkdir -p $qt_version/$arch || exit 1
                if [ $qt_version != "multiabi" ]; then
                    mkdir -p static/lib/$arch
                fi
                rm -fr openssl-$ssl_version
                tar xfa openssl-$ssl_version.tar.gz || exit 1
                pushd openssl-$ssl_version || exit 1
                ANDROID_TOOLCHAIN=""

                if [[ $ssl_version == 1.1* ]]; then
                    case $arch in
                        arm)
                            ANDROID_API=16
                            ;;
                        x86)
                            ANDROID_API=16
                            ;;
                        arm64)
                            ANDROID_API=21
                            ;;
                        x86_64)
                            ANDROID_API=21
                            ;;
                    esac

                    for host in ${hosts[@]}
                    do
                        if [ -d "$ANDROID_NDK_HOME/toolchains/llvm/prebuilt/$host/bin" ]; then
                            ANDROID_TOOLCHAIN="$ANDROID_NDK_HOME/toolchains/llvm/prebuilt/$host/bin"
                            break
                        fi
                    done

                    export PATH="$ANDROID_TOOLCHAIN:$DEFAULT_PATH"

                    ./Configure ${params[$param]} shared android-${arch} -D__ANDROID_API__=${ANDROID_API} || exit 1
                    make depend

                    case $qt_version in
                        latest)
                            make -j$(nproc) SHLIB_VERSION_NUMBER= SHLIB_EXT=_1_1.so build_libs || exit 1
                            llvm-strip --strip-all libcrypto_1_1.so
                            llvm-strip --strip-all libssl_1_1.so
                            cp libcrypto_1_1.so libssl_1_1.so ../$qt_version/$arch || exit 1
                            cp libcrypto.a ../static/lib/multiabi/libcrypto_${qt_architectures[$arch]}.a  || exit 1
                            cp libssl.a ../static/lib/multiabi/libssl_${qt_architectures[$arch]}.a  || exit 1
                            mv libcrypto.a ../static/lib/$arch/libcrypto.a  || exit 1
                            mv libssl.a ../static/lib/$arch/libssl.a  || exit 1
                            ;;
                        Qt-5.12.4_5.13.0)
                            make -j$(nproc) SHLIB_VERSION_NUMBER= build_libs || exit 1
                            llvm-strip --strip-all libcrypto.so
                            llvm-strip --strip-all libssl.so
                            cp libcrypto.so libssl.so ../$qt_version/$arch || exit 1
                            ;;
                        *)
                            echo "Unhandled Qt version $qt_version"
                            exit 1
                            ;;
                    esac
                else
                    # For 1.0 use NDK r10e
                    case $arch in
                        arm)
                            export ANDROID_EABI="arm-linux-androideabi-4.9"
                            export CROSS_COMPILE="arm-linux-androideabi-"
                            export ANDROID_API=android-9
                            CONF_PARAM=android
                            ;;
                        arm64)
                            export ANDROID_EABI="aarch64-linux-android-4.9"
                            export CROSS_COMPILE="aarch64-linux-android-"
                            export ANDROID_API=android-21
                            CONF_PARAM=android
                            ;;
                        x86)
                            export ANDROID_EABI="x86-4.9"
                            export CROSS_COMPILE="i686-linux-android-"
                            export ANDROID_API=android-9
                            CONF_PARAM=android-x86
                            ;;
                        x86_64)
                            popd
                            continue
                            ;;
                        *)
                            echo "Unhandled arch $arch"
                            exit 1
                            ;;
                    esac

                    export ANDROID_PLATFORM=arch-$arch
                    export SYSTEM=android
                    export ANDROID_SYSROOT=$ANDROID_NDK_HOME/platforms/$ANDROID_API/$ANDROID_PLATFORM
                    export ANDROID_DEV=$ANDROID_SYSROOT/usr
                    export CFLAGS="--sysroot=$ANDROID_SYSROOT"
                    export CPPFLAGS="--sysroot=$ANDROID_SYSROOT"
                    export CXXFLAGS="--sysroot=$ANDROID_SYSROOT"

                    for host in ${hosts[@]}
                    do
                        if [ -d "$ANDROID_NDK_HOME/toolchains/$ANDROID_EABI/prebuilt/$host/bin" ]; then
                            ANDROID_TOOLCHAIN="$ANDROID_NDK_HOME/toolchains/$ANDROID_EABI/prebuilt/$host/bin"
                            break
                        fi
                    done

                    export PATH="$ANDROID_TOOLCHAIN:$DEFAULT_PATH"

                    ./Configure ${params[$param]} shared $CONF_PARAM || exit 1
                    make depend
                    make -j$(nproc) CALC_VERSIONS="SHLIB_COMPAT=; SHLIB_SOVER=" build_libs || exit 1
                    ${CROSS_COMPILE}strip --strip-all libcrypto.so
                    ${CROSS_COMPILE}strip --strip-all libssl.so

                    # Unset variables so they don't interfere with different ssl_versions
                    unset CXXFLAGS CPPFLAGS CFLAGS ANDROID_DEV ANDROID_SYSROOT SYSTEM ANDROID_PLATFORM CONF_PARAM CROSS_COMPILE ANDROID_EABI
                    cp libcrypto.so libssl.so ../$qt_version/$arch  || exit 1
                fi
                popd
                if [[ $ssl_version == 1.1* ]] && [ ! -d static/include/openssl ]; then
                    cp -a openssl-$ssl_version/include/openssl static/include || exit 1
                fi
            done
        done
        rm -fr openssl-$ssl_version
        rm openssl-$ssl_version.tar.gz
    done
    if [ ${params[$param]} ]; then
        popd
    fi
done
