#-------------------------------------------------
#
# Project created by QtCreator 2018-02-28T20:45:21
#
#-------------------------------------------------

TARGET = BSVC_Qt
TEMPLATE = subdirs
SUBDIRS = m68000 \
    instruction \
    m68360 \
    68kasm \
    GUI
m68000.depends = instruction
m68360.depends = instruction
GUI.depends = instruction

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

HEADERS +=

SOURCES +=

QMAKE_CXXFLAGS += -std=c++11

QT_VERSION = $$[QT_VERSION]
QT_VERSION = $$split(QT_VERSION, ".")
QT_VER_MAJ = $$member(QT_VERSION, 0)
QT_VER_MIN = $$member(QT_VERSION, 1)

lessThan(QT_VER_MAJ, 5) | lessThan(QT_VER_MIN, 6) {
   error($$[TARGET] requires Qt 5.6 or newer but Qt $$[QT_VERSION] was detected.)
}

