include("../consoleapp.pro")
include("../cppext/lib.pro")

lpcroot= ../../dro/cortexm/lpc

INCLUDEPATH += $$lpcroot
DEPENDPATH += $$lpcroot

TARGET = lpcbestbaud

SOURCES += \
    lpcbaud.cpp \
    $$lpcroot/baudsearch.cpp

HEADERS += \
    $$lpcroot/baudsearch.h
