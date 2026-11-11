#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    
    const QString imagePath = "testEllipes.bmp";
    w.imageLabelView->setImage(QImage(imagePath));

    w.imageLabelView->addLabel(new ImagePointLabel(QPoint(100, 100)));
    w.imageLabelView->addLabel(new ImagePointLabel(QPoint(150, 125)));
    w.imageLabelView->addLabel(new ImagePointLabel(QPoint(200, 200)));
    w.imageLabelView->addLabel(new ImagePointLabel(QPoint(210, 170)));

    return QApplication::exec();
}
