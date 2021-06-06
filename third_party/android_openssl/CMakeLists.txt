if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set (SSL_ROOT_PATH ${CMAKE_CURRENT_LIST_DIR}/no-asm)
else()
    set (SSL_ROOT_PATH ${CMAKE_CURRENT_LIST_DIR})
endif()


if (QT_VERSION LESS 5.12.4)
    message(QT_VERSION)
    if (ANDROID_TARGET_ARCH STREQUAL "armeabi-v7a")
        set(ANDROID_EXTRA_LIBS
            ${SSL_ROOT_PATH}/Qt-5.12.3/arm/libcrypto.so
            ${SSL_ROOT_PATH}/Qt-5.12.3/arm/libssl.so
        CACHE INTERNAL "")
    endif()

    if (ANDROID_TARGET_ARCH STREQUAL "arm64-v8a")
        set(ANDROID_EXTRA_LIBS
            ${SSL_ROOT_PATH}/Qt-5.12.3/arm64/libcrypto.so
            ${SSL_ROOT_PATH}/Qt-5.12.3/arm64/libssl.so
        CACHE INTERNAL "")
    endif()

    if (ANDROID_TARGET_ARCH STREQUAL "x86")
        set(ANDROID_EXTRA_LIBS
            ${SSL_ROOT_PATH}/Qt-5.12.3/x86/libcrypto.so
            ${SSL_ROOT_PATH}/Qt-5.12.3/x86/libssl.so
        CACHE INTERNAL "")
    endif()
else()
    if (QT_VERSION EQUAL 5.12.4 OR QT_VERSION EQUAL 5.13.0)
        if (ANDROID_TARGET_ARCH STREQUAL "armeabi-v7a")
            set(ANDROID_EXTRA_LIBS
                ${SSL_ROOT_PATH}/Qt-5.12.4_5.13.0/arm/libcrypto.so
                ${SSL_ROOT_PATH}/Qt-5.12.4_5.13.0/arm/libssl.so
            CACHE INTERNAL "")
        endif()

        if (ANDROID_TARGET_ARCH STREQUAL "arm64-v8a")
            set(ANDROID_EXTRA_LIBS
                ${SSL_ROOT_PATH}/Qt-5.12.4_5.13.0/arm64/libcrypto.so
                ${SSL_ROOT_PATH}/Qt-5.12.4_5.13.0/arm64/libssl.so
            CACHE INTERNAL "")
        endif()

        if (ANDROID_TARGET_ARCH STREQUAL "x86")
            set(ANDROID_EXTRA_LIBS
                ${SSL_ROOT_PATH}/Qt-5.12.4_5.13.0/x86/libcrypto.so
                ${SSL_ROOT_PATH}/Qt-5.12.4_5.13.0/x86/libssl.so
            CACHE INTERNAL "")
        endif()

        if (ANDROID_TARGET_ARCH STREQUAL "x86_64")
            set(ANDROID_EXTRA_LIBS
                ${SSL_ROOT_PATH}/Qt-5.12.4_5.13.0/x86_64/libcrypto.so
                ${SSL_ROOT_PATH}/Qt-5.12.4_5.13.0/x86_64/libssl.so
            CACHE INTERNAL "")
        endif()
    else()
        if (NOT (QT_VERSION LESS 5.14.0))
            message(${SSL_ROOT_PATH})
            set(ANDROID_EXTRA_LIBS
                ${SSL_ROOT_PATH}/latest/arm/libcrypto_1_1.so
                ${SSL_ROOT_PATH}/latest/arm/libssl_1_1.so
                ${SSL_ROOT_PATH}/latest/arm64/libcrypto_1_1.so
                ${SSL_ROOT_PATH}/latest/arm64/libssl_1_1.so
                ${SSL_ROOT_PATH}/latest/x86/libcrypto_1_1.so
                ${SSL_ROOT_PATH}/latest/x86/libssl_1_1.so
                ${SSL_ROOT_PATH}/latest/x86_64/libcrypto_1_1.so
                ${SSL_ROOT_PATH}/latest/x86_64/libssl_1_1.so
            CACHE INTERNAL "")
        else()
            if (ANDROID_TARGET_ARCH STREQUAL "armeabi-v7a")
                set(ANDROID_EXTRA_LIBS
                    ${SSL_ROOT_PATH}/latest/arm/libcrypto_1_1.so
                    ${SSL_ROOT_PATH}/latest/arm/libssl_1_1.so
                CACHE INTERNAL "")
            endif()

            if (ANDROID_TARGET_ARCH STREQUAL "arm64-v8a")
                set(ANDROID_EXTRA_LIBS
                    ${SSL_ROOT_PATH}/latest/arm64/libcrypto_1_1.so
                    ${SSL_ROOT_PATH}/latest/arm64/libssl_1_1.so
                CACHE INTERNAL "")
            endif()

            if (ANDROID_TARGET_ARCH STREQUAL "x86")
                set(ANDROID_EXTRA_LIBS
                    ${SSL_ROOT_PATH}/latest/x86/libcrypto_1_1.so
                    ${SSL_ROOT_PATH}/latest/x86/libssl_1_1.so
                CACHE INTERNAL "")
            endif()

            if (ANDROID_TARGET_ARCH STREQUAL "x86_64")
                set(ANDROID_EXTRA_LIBS
                    ${SSL_ROOT_PATH}/latest/x86_64/libcrypto_1_1.so
                    ${SSL_ROOT_PATH}/latest/x86_64/libssl_1_1.so
                CACHE INTERNAL "")
            endif()
        endif()
    endif()
endif()
