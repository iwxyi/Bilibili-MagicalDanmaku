message("BREAKPAD_crash_handler_attached")

INCLUDEPATH += $$PWD/handler/

HEADERS += \
    $$PWD/handler/QBreakpadHandler.h \
    $$PWD/handler/QBreakpadHttpUploader.h

LIBS += \
    -L$$PWD/handler -lqBreakpad
