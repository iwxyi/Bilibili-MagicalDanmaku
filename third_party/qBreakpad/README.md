#qBreakpad

[![Build status](https://travis-ci.org/buzzySmile/qBreakpad.svg?branch=master)](https://travis-ci.org/buzzySmile/qBreakpad)

qBreakpad is Qt library to use google-breakpad crash reporting facilities (and using it conviniently).
Supports
* Windows (but crash dump decoding will not work with MinGW compiler)
* Linux
* MacOS X

How to use
----------------
* Clone repository recursively
```bash
$ git clone --recursive https://github.com/buzzySmile/qBreakpad.git
```
* Build qBreakpad static library (qBreakpad/handler/)
* Include "qBreakpad.pri" to your target Qt project
```c++
include($$PWD/{PATH_TO_QBREAKPAD}/qBreakpad.pri)
```
* Setup linking with "qBreakpad" library
```c++
QMAKE_LIBDIR += $$PWD/{PATH_TO_QBREAKPAD}/handler
LIBS += -lqBreakpad
```
* Use ```QBreakpadHandler``` singleton class to enable automatic crash dumps generation on any failure; example:
```c++
#include <QBreakpadHandler.h>

int main(int argc, char* argv[])
{
    ...
    QBreakpadInstance.setDumpPath(QLatin1String("crashes"));
    ...
}
```
* Read Google Breakpad documentation to know further workflow

Getting started with Google Breakpad
----------------
https://chromium.googlesource.com/breakpad/breakpad/+/master/docs/getting_started_with_breakpad.md

Detail description about integration `qBreakpad` into your system and platform you could find in **[Wiki](https://github.com/buzzySmile/qBreakpad/wiki)**.
