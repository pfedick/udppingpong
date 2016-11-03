#include "MainWindow.h"


#include <QMenuBar>
#include <QMdiSubWindow>
#include <QList>



MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
	ui.setupUi(this);
	this->show();
}

MainWindow::~MainWindow()
{
}


