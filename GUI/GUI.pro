#-------------------------------------------------
#
# Project created by QtCreator 2018-03-01T01:17:57
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = GUI
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


SOURCES += \
        main.cpp \
    ../M68k/sim68000/decode.cpp \
    ../M68k/sim68000/exec.cpp \
    ../M68k/sim68000/m68000.cpp \
    ../M68k/devices/DeviceRegistry.cpp \
    ../M68k/devices/Gdbsock.cpp \
    ../M68k/devices/M68681.cpp \
    ../M68k/devices/RAM.cpp \
    ../M68k/devices/Timer.cpp \
    ../M68k/loader/Loader.cpp \
    ../Framework/AddressSpace.cpp \
    ../Framework/BasicCPU.cpp \
    ../Framework/BasicDevice.cpp \
    ../Framework/BasicDeviceRegistry.cpp \
    ../Framework/Event.cpp \
    ../Framework/Interface.cpp \
    ../Framework/RegInfo.cpp \
    ../Framework/StatInfo.cpp \
    ../Framework/Tools.cpp \
    mainwindow.cpp \
    qtinterface.cpp \
    qtinterfacetypes.cpp \
    Widgets/memorydockwidget.cpp

HEADERS += \
        mainwindow.h \
    ../M68k/sim68000/m68000.hpp \
    ../M68k/devices/DeviceRegistry.hpp \
    ../M68k/devices/Gdbsock.hpp \
    ../M68k/devices/M68681.hpp \
    ../M68k/devices/RAM.hpp \
    ../M68k/devices/Timer.hpp \
    ../M68k/loader/Loader.hpp \
    ../Framework/AddressSpace.hpp \
    ../Framework/BasicCPU.hpp \
    ../Framework/BasicDevice.hpp \
    ../Framework/BasicDeviceRegistry.hpp \
    ../Framework/BasicLoader.hpp \
    ../Framework/BreakpointList.hpp \
    ../Framework/Event.hpp \
    ../Framework/Interface.hpp \
    ../Framework/RegInfo.hpp \
    ../Framework/StatInfo.hpp \
    ../Framework/Time.hpp \
    ../Framework/Tools.hpp \
    ../Framework/Types.hpp \
    mainwindow.h \
    qtinterface.h \
    qtinterfacetypes.h \
    Widgets/memorydockwidget.h

FORMS += \
    mainwindow.ui

INCLUDEPATH += $$PWD/..

QMAKE_EXTRA_TARGETS += buildtable cleantemp
buildtable.target = buildtable
CONFIG(release, debug|release):buildtable.commands = \"$$OUT_PWD/../instruction/instruction\" \"$$PWD/../M68k/sim68000/instruction.list\" \"$$PWD/../M68k/sim68000/DecodeTable.hpp\" m68000
CONFIG(debug, debug|release):buildtable.commands = \"$$OUT_PWD/../instruction/debug/instruction\" \"$$PWD/../M68k/sim68000/instruction.list\" \"$$PWD/../M68k/sim68000/DecodeTable.hpp\" m68000
CONFIG(release, debug|release):buildtable.commands += && \"$$OUT_PWD/../instruction/instruction\" \"$$PWD/../M68k/sim68360/instruction.list\" \"$$PWD/../M68k/sim68360/DecodeTable.hpp\" cpu32
CONFIG(debug, debug|release):buildtable.commands += && \"$$OUT_PWD/../instruction/debug/instruction\" \"$$PWD/../M68k/sim68360/instruction.list\" \"$$PWD/../M68k/sim68360/DecodeTable.hpp\" cpu32

all.depends = buildtable
cleantemp = del \"$$PWD/../M68k/sim68000/DecodeTable.hpp\" \"$$PWD/../M68k/sim68360/DecodeTable.hpp\"
clean.depends = cleantemp
PRE_TARGETDEPS += buildtable

QMAKE_CXXFLAGS += -std=c++11
