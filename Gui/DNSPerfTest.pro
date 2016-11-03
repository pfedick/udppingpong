TEMPLATE = app
TARGET = DNSPerfTest
QT += core \
    gui
    
equals(QT_MAJOR_VERSION, 5){
   QT += webkitwidgets
   QT += widgets
   QT += printsupport
}

CONFIG += debug_and_release
CONFIG(debug, debug|release) { 
    win32:TARGET = DNSPerfTest
    unix:TARGET = debug/DNSPerfTest
    DEFINES += DEBUGGUI
}
else { 
    win32:TARGET = DNSPerfTest
    unix:TARGET = release/DNSPerfTest
}
HEADERS += src/MainWindow/MainWindow.h
SOURCES += src/main.cpp \
	src/MainWindow/MainWindow.cpp
FORMS += src/MainWindow/MainWindow.ui
RESOURCES += resources.qrc
RC_FILE = resource.rc
INCLUDEPATH += include
INCLUDEPATH += src
unix:INCLUDEPATH += ${HOME}/include
win32:INCLUDEPATH += /usr/local/include \
    /usr/local/ssl/include
win32:LIBPATH += /usr/local/lib \
    /usr/local/ssl/lib
CONFIG(debug, debug|release) { 
    # Debug
    unix:LIBS += `ppl7-config \
        --libs \
        debug`
    win32:LIBS += `ppl7-config \
        --libs \
        debug`
    win32:LIBS += -lcrypt32 \
        -lws2_32 \
        -lgdi32
}
else { 
    # Release
    unix:LIBS += `ppl7-config \
        --libs \
        release`
    win32:LIBS += `ppl7-config \
        --libs \
        release`
    win32:LIBS += -lcrypt32 \
        -lws2_32 \
        -lgdi32
}
CODECFORSRC = UTF-8
CODECFORTR = UTF-8
