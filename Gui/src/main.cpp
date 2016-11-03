#include <QtGui>
#include <QApplication>
#include <ppl7.h>
#include <ppl7-inet.h>
#include "MainWindow/MainWindow.h"

int main(int argc, char *argv[])
{
	ppl7::SSL_Init();
#if QT_VERSION < 0x050000
	// Deprecated in Qt5, Qt5 geht davon aus, dass der Sourcecode UTF-8 kodiert ist
    QTextCodec::setCodecForTr(QTextCodec::codecForName("UTF-8"));
    QTextCodec::setCodecForCStrings(QTextCodec::codecForName("UTF-8"));
#endif
    QApplication a(argc, argv);
    MainWindow gui;

    a.connect(&a, SIGNAL(lastWindowClosed()), &a, SLOT(quit()));
    int ret=a.exec();
    ppl7::SSL_Exit();
#ifdef WIN32
    exit(ret);
#else
    return ret;
#endif
}
