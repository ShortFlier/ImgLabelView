#include "mainwindow.h"
#include "./ui_mainwindow.h"



MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    imageLabelView = new ImageLabelView(this);
    setCentralWidget(imageLabelView);

}

MainWindow::~MainWindow()
{
    delete ui;
}
