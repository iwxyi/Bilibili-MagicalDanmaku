CONFIG += static

macx {
    CONFIG += c++11
    LIBS += -lcrypto
}

# test config
# TODO actually, I shoud check it better
LIST = thread exceptions rtti stl
for(f, LIST) {
    !CONFIG($$f) {
        warning("Add '$$f' to CONFIG, or you will find yourself in 'funny' problems.")
    }
}

# define breakpad server    SOCORRO / CALIPER
DEFINES += CALIPER
