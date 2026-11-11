#include "../imagelabelview/imagelabelview.h"

#include <QApplication>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QLabel>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QtMath>
#include <cmath>
#include <cstdlib>
#include <iostream>

namespace {
void check(bool condition, const char *message)
{
    if (!condition) {
        std::cerr << message << std::endl;
        std::exit(1);
    }
}

class CountingPoint : public ImagePointLabel
{
public:
    using ImagePointLabel::ImagePointLabel;
    ~CountingPoint() override { ++destroyed; }
    static int destroyed;
};
int CountingPoint::destroyed = 0;
}

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    ImageLabelView widget;
    widget.resize(600, 400);
    widget.show();
    app.processEvents();

    auto *view = widget.findChild<QGraphicsView *>("graphicsView");
    auto *size = widget.findChild<QLabel *>("imgSizeLabel");
    auto *position = widget.findChild<QLabel *>("positionLabel");
    auto *pixel = widget.findChild<QLabel *>("pixInfoLabel");
    auto *scale = widget.findChild<QLabel *>("scaleLabel");
    check(view && size && position && pixel && scale, "UI controls missing");

    QImage image(100, 80, QImage::Format_ARGB32);
    image.fill(QColor(12, 34, 56, 78));
    widget.setImage(image);
    check(size->text().contains("100x80"), "image dimensions not updated");
    check(view->transform().m11() > 0, "image fit scale is invalid");

    auto *point = new CountingPoint(QPointF(10.5, 20.5));
    auto *text = new ImageTextLabel(QPointF(20.5, 30.5), "test");
    auto *rect = new ImageRotatedRectLabel(21, 11, QPoint(40, 30), 30);
    auto *ellipse = new ImageEllipseLabel(QPoint(50, 40), 12, 8, 30);
    auto *arrow = new ImageArrowLabel(QPoint(10, 10), QPoint(21, 15));
    widget.addLabel(point);
    widget.addLabel(text);
    widget.addLabel(rect);
    widget.addLabel(ellipse);
    widget.addLabel(arrow);
    check(view->scene()->items().size() == 6, "labels were not added");
    check(point->pos() == QPointF(10.5, 20.5) && text->pos() == QPointF(20.5, 30.5),
          "fractional positions lost");
    check(rect->pos() == QPointF(40, 30) && ellipse->pos() == QPointF(50, 40)
          && arrow->pos() == QPointF(15.5, 12.5), "geometry centers incorrect");
    check(point->flags().testFlag(QGraphicsItem::ItemIgnoresTransformations)
          && text->flags().testFlag(QGraphicsItem::ItemIgnoresTransformations),
          "screen-sized labels follow zoom");
    check(!rect->flags().testFlag(QGraphicsItem::ItemIgnoresTransformations),
          "geometric labels ignore zoom");
    check(rect->boundingRect().contains(QPointF(0, 0)) && arrow->boundingRect().contains(QPointF(0, 0)),
          "invalid label bounds");
    QImage rendered(widget.size(), QImage::Format_ARGB32_Premultiplied);
    rendered.fill(Qt::transparent);
    widget.render(&rendered);
    check(!rendered.isNull(), "widget rendering failed");

    const QPoint mouse = view->mapFromScene(QPointF(50, 40));
    QMouseEvent move(QEvent::MouseMove, QPointF(mouse),
                     QPointF(view->viewport()->mapToGlobal(mouse)),
                     Qt::NoButton, Qt::NoButton, Qt::NoModifier);
    QApplication::sendEvent(view->viewport(), &move);
    check(position->text().contains("(50,40)"), "mouse coordinates not displayed");
    check(pixel->text().contains("(12,34,56,78)"), "pixel color not displayed");

    QImage gray(100, 80, QImage::Format_Grayscale8);
    gray.fill(123);
    widget.setImage(gray);
    check(pixel->text().contains("(123)"), "8-bit grayscale pixel not displayed");
#if QT_VERSION >= QT_VERSION_CHECK(5, 13, 0)
    QImage gray16(100, 80, QImage::Format_Grayscale16);
    gray16.fill(0);
    for (int y = 0; y < gray16.height(); ++y) {
        auto *line = reinterpret_cast<quint16 *>(gray16.scanLine(y));
        for (int x = 0; x < gray16.width(); ++x)
            line[x] = 12345;
    }
    widget.setImage(gray16);
    check(pixel->text().contains("(12345)"), "16-bit grayscale pixel not displayed");
#endif
    QImage indexed(100, 80, QImage::Format_Indexed8);
    indexed.setColorTable({qRgb(42, 42, 42), qRgb(200, 200, 200)});
    indexed.fill(0);
    widget.setImage(indexed);
    check(pixel->text().contains("(42)"), "indexed grayscale pixel not displayed");
    indexed.setColor(0, qRgb(12, 34, 56));
    widget.setImage(indexed);
    check(pixel->text().contains("(12,34,56)"), "indexed color pixel not displayed");
    QImage rgb(100, 80, QImage::Format_RGB888);
    rgb.fill(QColor(12, 34, 56));
    widget.setImage(rgb);
    check(pixel->text().contains("(12,34,56)"), "RGB pixel not displayed");
    rgb.fill(QColor(77, 77, 77));
    widget.setImage(rgb);
    check(pixel->text().contains("(77,77,77)"), "gray-colored RGB pixel lost channels");
    widget.setImage(image);
    check(pixel->text().contains("(12,34,56,78)"), "RGBA pixel not restored");

    const qreal before = view->transform().m11();
    const QRectF arrowBounds = arrow->boundingRect();
    QWheelEvent wheel(QPointF(mouse), QPointF(view->viewport()->mapToGlobal(mouse)),
                      QPoint(), QPoint(0, 120), Qt::NoButton, Qt::NoModifier,
                      Qt::NoScrollPhase, false);
    QApplication::sendEvent(view->viewport(), &wheel);
    check(std::abs(view->transform().m11() - before * 1.15) < 0.0001, "wheel zoom failed");
    check(arrow->boundingRect().width() < arrowBounds.width(), "arrow size did not adapt to zoom");
    check(!scale->text().isEmpty(), "zoom status missing");

    widget.setScaleFactorRange(0.5, 1.0);
    QWheelEvent limitedWheel(QPointF(mouse), QPointF(view->viewport()->mapToGlobal(mouse)),
                            QPoint(), QPoint(0, 120), Qt::NoButton, Qt::NoModifier,
                            Qt::NoScrollPhase, false);
    QApplication::sendEvent(view->viewport(), &limitedWheel);
    check(std::abs(view->transform().m11() - 1.0) < 0.0001, "maximum zoom not enforced");
    widget.setScaleFactorRange(-1, 0);
    QWheelEvent invalidRangeWheel(QPointF(mouse), QPointF(view->viewport()->mapToGlobal(mouse)),
                                 QPoint(), QPoint(0, 120), Qt::NoButton, Qt::NoModifier,
                                 Qt::NoScrollPhase, false);
    QApplication::sendEvent(view->viewport(), &invalidRangeWheel);
    check(std::abs(view->transform().m11() - 1.0) < 0.0001, "invalid zoom range was accepted");

    widget.addLabel(point);
    check(view->scene()->items().size() == 6, "duplicate label added");
    QGraphicsScene otherScene;
    auto *otherPoint = new CountingPoint(QPoint(2, 2));
    otherScene.addItem(otherPoint);
    widget.addLabel(otherPoint);
    check(otherPoint->scene() == &otherScene && view->scene()->items().size() == 6,
          "label was transferred from another scene");
    widget.clearLabels();
    check(CountingPoint::destroyed == 1 && view->scene()->items().size() == 1,
          "clearLabels did not delete labels");
    widget.setImage(QImage());
    check(size->text().contains("0x0") && pixel->text().contains("--")
          && std::isfinite(view->transform().m11()), "empty image not handled");

    ImageLabelView dragWidget;
    dragWidget.resize(600, 400);
    dragWidget.show();
    app.processEvents();
    auto *dragView = dragWidget.findChild<QGraphicsView *>("graphicsView");
    auto *dragPosition = dragWidget.findChild<QLabel *>("positionLabel");
    auto *dragPixel = dragWidget.findChild<QLabel *>("pixInfoLabel");
    QImage large(1000, 800, QImage::Format_RGB888);
    large.fill(QColor(10, 20, 30));
    dragWidget.setImage(large);
    dragWidget.setScaleFactorRange(0.1, 20);
    const QPoint dragStart = dragView->viewport()->rect().center();
    QWheelEvent zoom(QPointF(dragStart), QPointF(dragView->viewport()->mapToGlobal(dragStart)),
                     QPoint(), QPoint(0, 120), Qt::NoButton, Qt::NoModifier,
                     Qt::NoScrollPhase, false);
    QApplication::sendEvent(dragView->viewport(), &zoom);
    QApplication::sendEvent(dragView->viewport(), &zoom);
    QApplication::sendEvent(dragView->viewport(), &zoom);
    const qreal zoomedScale = dragView->transform().m11();
    const QPointF oldCenter = dragView->mapToScene(dragStart);
    QMouseEvent press(QEvent::MouseButtonPress, QPointF(dragStart),
                      QPointF(dragView->viewport()->mapToGlobal(dragStart)),
                      Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(dragView->viewport(), &press);
    const QPoint dragEnd = dragStart + QPoint(-40, -30);
    QMouseEvent dragMove(QEvent::MouseMove, QPointF(dragEnd),
                         QPointF(dragView->viewport()->mapToGlobal(dragEnd)),
                         Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(dragView->viewport(), &dragMove);
    const QPointF draggedCenter = dragView->mapToScene(dragStart);
    check(draggedCenter != oldCenter, "mouse drag did not pan image");
    const QPointF cursorScene = dragView->mapToScene(dragEnd);
    check(dragPosition->text().contains(QString("(%1,%2)").arg(qFloor(cursorScene.x())).arg(qFloor(cursorScene.y()))),
          "drag position is stale");
    check(dragPixel->text().contains("(10,20,30)"), "drag pixel information is stale");
    QMouseEvent release(QEvent::MouseButtonRelease, QPointF(dragEnd),
                        QPointF(dragView->viewport()->mapToGlobal(dragEnd)),
                        Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
    QApplication::sendEvent(dragView->viewport(), &release);

    large.fill(QColor(40, 50, 60));
    dragWidget.setImage(large);
    check(qFuzzyCompare(dragView->transform().m11(), zoomedScale), "same-size image reset zoom");
    check(dragView->mapToScene(dragStart) == draggedCenter, "same-size image reset pan");
    check(dragPixel->text().contains("(40,50,60)"), "replacement pixel information is stale");

    QImage larger(1200, 1000, QImage::Format_RGB888);
    larger.fill(QColor(70, 80, 90));
    dragWidget.setImage(larger);
    check(qFuzzyCompare(dragView->transform().m11(), zoomedScale), "different-size image reset zoom");
    const QPointF resizedCenter = dragView->mapToScene(dragStart);
    check(std::abs(resizedCenter.x() - draggedCenter.x()) <= 2.0 / zoomedScale
          && std::abs(resizedCenter.y() - draggedCenter.y()) <= 2.0 / zoomedScale,
          "different-size image reset pan");
    dragWidget.setImage(QImage());
    check(qFuzzyCompare(dragView->transform().m11(), zoomedScale)
          && dragPixel->text().contains("--"), "empty image reset zoom or pixel information");
    dragWidget.setImage(larger);
    check(qFuzzyCompare(dragView->transform().m11(), zoomedScale)
          && dragView->mapToScene(dragStart) == resizedCenter, "reload after empty image reset view");
    return 0;
}
