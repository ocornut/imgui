TEMPLATE = lib
CONFIG += static
CONFIG -= app_bundle
CONFIG -= qt
CONFIG -= warn_on

CONFIG(release, debug|release) {
    TARGET = imgui
}

CONFIG(debug, debug|release) {
    TARGET = imgui-dbg
}

SOURCES += \
    imgui.cpp

HEADERS += \
    imgui.h \
    imconfig.h \
    immem.h
