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
    w.imageLabelView->addLabel(new ImageTextLabel(QPointF(70, 70), "Point", Qt::red));
    w.imageLabelView->addLabel(new ImageRotatedRectLabel(QRectF(200, 65, 120, 65), 20, Qt::green));
    w.imageLabelView->addLabel(new ImageEllipseLabel(QPointF(100, 250), 45, Qt::blue));
    w.imageLabelView->addLabel(new ImageEllipseLabel(QPointF(270, 250), 65, 35, -25, Qt::magenta));
    w.imageLabelView->addLabel(new ImageArrowLabel(QPointF(140, 150), QPointF(220, 180), Qt::cyan));
    w.imageLabelView->addLabel(new ImageLineLabel(QPointF(160, 160), QPointF(380, 380), Qt::blue));
    w.imageLabelView->addLabel(new ImageLineLabel(QPointF(200, 200), 30));

    return QApplication::exec();
}
