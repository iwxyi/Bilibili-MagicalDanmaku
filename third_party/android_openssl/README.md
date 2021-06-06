# Android OpenSSL support for Qt
OpenSSL scripts and binaries for Android (useful for Qt Android apps)

In this repo you can find the prebuilt OpenSSL libs for Android and a qmake include project `.pri` file that can be used integrated with Qt projects.

The following directories are available
* `Qt-5.12.3`: used for Qt 5.12.3 and below.
* `Qt-5.12.4_5.13.0`: this has OpenSSL 1.1.x which can be used **ONLY** with Qt 5.12.4 and 5.13.0. Be aware that on Android 5 (API 21) these libs names are clashing with the system SSL libs which are using OpenSSL 1.0, this means your Qt app will fail to use OpenSSL 1.1 as the system ones are already loaded by the OS.
* `latest`: used for Qt 5.12.5+ and 5.13.1+.


## How to use

To add OpenSSL in your QMake project, append the following to your `.pro` project file:

```
android: include(<path/to/android_openssl/openssl.pri)
```

To add OpenSSL in your CMake project, append the following to your project's `CMakeLists.txt` file, anywhere before the find_package() call for Qt5 modules:

```
if (ANDROID)
    include(<path/to/android_openssl/CMakeLists.txt)
endif()
```

There is also a `static` folder which contains the OpenSSL static libs prefixed with the abi. These libs are useful to build Qt **5.14.1+**.
