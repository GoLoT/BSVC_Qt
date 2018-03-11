TARGET = 68kasm
TEMPLATE = app

CONFIG += c++11 console
CONFIG -= app_bundle

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    ../Assemblers/68kasm/assemble.c \
    ../Assemblers/68kasm/build.c \
    ../Assemblers/68kasm/codegen.c \
    ../Assemblers/68kasm/directive.c \
    ../Assemblers/68kasm/error.c \
    ../Assemblers/68kasm/eval.c \
    ../Assemblers/68kasm/globals.c \
    ../Assemblers/68kasm/include.c \
    ../Assemblers/68kasm/instlookup.c \
    ../Assemblers/68kasm/insttable.c \
    ../Assemblers/68kasm/listing.c \
    ../Assemblers/68kasm/main.c \
    ../Assemblers/68kasm/movem.c \
    ../Assemblers/68kasm/object.c \
    ../Assemblers/68kasm/opparse.c \
    ../Assemblers/68kasm/symbol.c

HEADERS += \
    ../Assemblers/68kasm/asm.h
