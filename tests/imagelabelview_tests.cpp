#include "../imagelabelview/imagelabelview.h"

#include <QApplication>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QScrollBar>
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

class InspectableFastContour : public ImageContourFastLabel
{
public:
    using ImageContourFastLabel::ImageContourFastLabel;
    const uchar *overlayData() const { return _overlay.constBits(); }
};
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

    ImagePointLabel fractionalPoint(QPointF(10.5, 20.5), 20.5, Qt::red, 1.5, 45.5);
    check(fractionalPoint.boundingRect() == QRectF(-12, -12, 24, 24),
          "fractional point size or thickness was not preserved");
    QImage pointRender(50, 50, QImage::Format_ARGB32_Premultiplied);
    pointRender.fill(Qt::transparent);
    {
        QPainter painter(&pointRender);
        fractionalPoint.paint(&painter, nullptr, nullptr);
        check(qFuzzyCompare(painter.pen().widthF(), qreal(1.5))
              && qFuzzyCompare(painter.transform().m11(), std::cos(qDegreesToRadians(qreal(45.5))))
              && qFuzzyCompare(painter.transform().m12(), std::sin(qDegreesToRadians(qreal(45.5)))),
              "fractional point thickness or angle was not preserved");
    }

    QImage mask(4, 2, QImage::Format_Grayscale8);
    for (int y = 0; y < mask.height(); ++y) {
        uchar *line = mask.scanLine(y);
        line[0] = 0;
        line[1] = 127;
        line[2] = 128;
        line[3] = 255;
    }
    const QColor maskColor(200, 100, 50);
    ImageMaskLabel softMask(QPointF(5.5, 6), mask, maskColor, -1, 128);
    check(softMask.imagePos() == QPointF(7.5, 7)
          && softMask.boundingRect() == QRectF(-2, -1, 4, 2),
          "mask geometry is incorrect");
    QImage maskRender(4, 2, QImage::Format_ARGB32);
    maskRender.fill(Qt::transparent);
    {
        QPainter painter(&maskRender);
        painter.translate(2, 1);
        softMask.paint(&painter, nullptr, nullptr);
    }
    check(maskRender.pixelColor(0, 0).alpha() == 0
          && maskRender.pixelColor(1, 0).alpha() == 64
          && maskRender.pixelColor(2, 0).alpha() == 64
          && maskRender.pixelColor(3, 0).alpha() == 128
          && maskRender.pixelColor(3, 0).red() == maskColor.red(),
          "mask intensity or opacity was not rendered correctly");
    ImageMaskLabel binaryMask(QPointF(5.5, 6), mask, maskColor, 127, 128);
    maskRender.fill(Qt::transparent);
    {
        QPainter painter(&maskRender);
        painter.translate(2, 1);
        binaryMask.paint(&painter, nullptr, nullptr);
    }
    check(maskRender.pixelColor(1, 0).alpha() == 0
          && maskRender.pixelColor(2, 0).alpha() == 128,
          "mask threshold was not applied correctly");

    const QList<QPointF> contourPoints = {QPointF(10, 10), QPointF(30, 10), QPointF(30, 30)};
    ImageContourLabel contour(contourPoints, Qt::red, 4);
    check(contour.imagePos() == QPointF(20, 20)
          && contour.boundingRect() == QRectF(-13, -13, 26, 26),
          "contour geometry is incorrect");
    QImage contourRender(80, 80, QImage::Format_ARGB32_Premultiplied);
    contourRender.fill(Qt::transparent);
    {
        QPainter painter(&contourRender);
        painter.translate(contour.imagePos());
        painter.setBrush(Qt::green);
        contour.paint(&painter, nullptr, nullptr);
    }
    check(contourRender.pixelColor(20, 20) == QColor(Qt::red)
          && contourRender.pixelColor(25, 15).alpha() == 0,
          "contour is not closed or was filled");
    ImageContourLabel thresholdContour(contourPoints, Qt::red, 4);
    const auto contourPenWidth = [&contourRender](ImageContourLabel &label) {
        QPainter painter(&contourRender);
        label.paint(&painter, nullptr, nullptr);
        return painter.pen().widthF();
    };
    ImageContourLabel fractionalContour(contourPoints, Qt::red, 1.5);
    check(qFuzzyCompare(contourPenWidth(fractionalContour), qreal(1.5))
          && fractionalContour.boundingRect() == QRectF(-11.75, -11.75, 23.5, 23.5),
          "fractional contour thickness was not preserved");
    fractionalContour.onViewScaleChanged(2.0);
    check(qFuzzyCompare(contourPenWidth(fractionalContour), qreal(0.75))
          && fractionalContour.boundingRect() == QRectF(-11.375, -11.375, 22.75, 22.75),
          "fractional contour thickness did not scale correctly");
    const QRectF initialContourBounds = thresholdContour.boundingRect();
    thresholdContour.onViewScaleChanged(4.0 / 4.5);
    check(thresholdContour.boundingRect() == initialContourBounds
          && qFuzzyCompare(contourPenWidth(thresholdContour), qreal(4)),
          "small contour thickness change caused an update");
    thresholdContour.onViewScaleChanged(4.0 / 5.0);
    check(qFuzzyCompare(contourPenWidth(thresholdContour), qreal(5))
          && thresholdContour.boundingRect().width() > initialContourBounds.width(),
          "large contour thickness change was not applied");
    thresholdContour.onViewScaleChanged(10.0);
    const QRectF thinBounds = thresholdContour.boundingRect();
    thresholdContour.onViewScaleChanged(4.0 / 0.38);
    check(thresholdContour.boundingRect() == thinBounds
          && qFuzzyCompare(contourPenWidth(thresholdContour), qreal(0.4)),
          "small thin-contour change caused an update");
    thresholdContour.onViewScaleChanged(4.0 / 0.3);
    check(qFuzzyCompare(contourPenWidth(thresholdContour), qreal(0.3))
          && thresholdContour.boundingRect().width() < thinBounds.width(),
          "large thin-contour change was not applied");
    contour.onViewScaleChanged(2.0);
    check(contour.boundingRect() == QRectF(-12, -12, 24, 24),
          "contour bounds did not adapt to zoom");
    contourRender.fill(Qt::transparent);
    {
        QPainter painter(&contourRender);
        painter.scale(2, 2);
        painter.translate(contour.imagePos());
        contour.paint(&painter, nullptr, nullptr);
    }
    int strokePixels = 0;
    for (int y = 16; y <= 24; ++y)
        strokePixels += contourRender.pixelColor(50, y).alpha() != 0;
    check(strokePixels == 4, "contour stroke width changed on screen after zoom");
    ImageContourLabel emptyContour({});
    check(emptyContour.boundingRect().isEmpty(), "empty contour has nonempty bounds");
    ImageContourLabel pointContour({QPointF(7, 9)}, Qt::red, 4);
    check(pointContour.imagePos() == QPointF(7, 9)
          && pointContour.boundingRect().contains(QPointF(0, 0)),
          "single-point contour geometry is incorrect");
    contourRender.fill(Qt::transparent);
    {
        QPainter painter(&contourRender);
        painter.translate(pointContour.imagePos());
        pointContour.paint(&painter, nullptr, nullptr);
    }
    check(contourRender.pixelColor(7, 9).alpha() != 0, "single-point contour was not drawn");

    InspectableFastContour flashContour(contourPoints, QColor(40, 80, 120), 1, 200);
    const auto renderFlash = [&contourRender](ImageContourFastLabel &label) {
        contourRender.fill(Qt::transparent);
        QPainter painter(&contourRender);
        painter.translate(label.imagePos());
        label.paint(&painter, nullptr, nullptr);
    };
    const auto flashStrokeWidth = [&contourRender]() {
        int width = 0;
        for (int y = 4; y <= 16; ++y)
            width += contourRender.pixelColor(22, y).alpha() != 0;
        return width;
    };
    renderFlash(flashContour);
    check(flashStrokeWidth() == 1 && contourRender.pixelColor(20, 20).alpha() == 200
          && contourRender.pixelColor(20, 20).blue() == 120
          && contourRender.pixelColor(25, 15).alpha() == 0,
          "flash contour mask has incorrect closure, color, opacity or width");
    const QImage originalFlash = contourRender.copy();
    const QPointF flashPosition = flashContour.imagePos();
    const QRectF flashBounds = flashContour.boundingRect();
    flashContour.onViewScaleChanged(4.0);
    renderFlash(flashContour);
    check(flashStrokeWidth() == 1 && flashContour.boundingRect() == flashBounds,
          "flash contour width dropped below one image pixel");
    flashContour.onViewScaleChanged(0.5);
    renderFlash(flashContour);
    check(flashStrokeWidth() == 3, "flash contour did not round two pixels up to three");
    flashContour.onViewScaleChanged(0.25);
    renderFlash(flashContour);
    check(flashStrokeWidth() == 5 && flashContour.imagePos() == flashPosition
          && flashContour.boundingRect().width() > flashBounds.width(),
          "flash contour did not widen or moved when zooming out");
    const QRectF wideFlashBounds = flashContour.boundingRect();
    const uchar *wideOverlay = flashContour.overlayData();
    flashContour.onViewScaleChanged(0.24);
    check(flashContour.boundingRect() == wideFlashBounds,
          "small scale changes unexpectedly changed flash contour width");
    flashContour.onViewScaleChanged(0.5);
    renderFlash(flashContour);
    check(flashStrokeWidth() == 3 && flashContour.overlayData() == wideOverlay,
          "flash contour did not reuse the image when shrinking to three pixels");
    flashContour.onViewScaleChanged(1.0);
    renderFlash(flashContour);
    check(flashStrokeWidth() == 1 && contourRender == originalFlash
          && flashContour.overlayData() == wideOverlay,
          "flash contour did not restore its original mask when shrinking");
    flashContour.onViewScaleChanged(0.25);
    renderFlash(flashContour);
    check(flashStrokeWidth() == 5 && flashContour.overlayData() == wideOverlay,
          "flash contour did not reuse the image when widening again");
    ImageContourFastLabel evenPixelContour(contourPoints, Qt::red, 2, 255);
    renderFlash(evenPixelContour);
    check(flashStrokeWidth() == 3, "even initial flash contour width was not rounded up");
    ImageContourFastLabel fractionalPixelContour(contourPoints, Qt::red, 1.75, 255);
    renderFlash(fractionalPixelContour);
    check(flashStrokeWidth() == 1, "fractional fast contour width was not rounded correctly");
    fractionalPixelContour.onViewScaleChanged(0.6);
    renderFlash(fractionalPixelContour);
    check(flashStrokeWidth() == 3, "fractional fast contour width was lost before scaling");
    ImageContourFastLabel emptyFlash({});
    check(emptyFlash.boundingRect().isEmpty(), "empty flash contour has nonempty bounds");
    ImageContourFastLabel pointFlash({QPointF(7, 9)}, Qt::red, 1, 255);
    renderFlash(pointFlash);
    check(contourRender.pixelColor(7, 9).alpha() == 255, "single-point flash contour was not drawn");
    const QImage originalPoint = contourRender.copy();
    pointFlash.onViewScaleChanged(0.2);
    pointFlash.onViewScaleChanged(1.0);
    renderFlash(pointFlash);
    check(contourRender == originalPoint, "isolated contour point was lost after resizing");

    const QPointF fractionalCenter(40.25, 30.5);
    const QRectF fractionalRect(29.5, 24.875, 21.5, 11.25);
    ImageRotatedRectLabel rectFromBounds(fractionalRect, 0, Qt::red, 1.5);
    ImageRotatedRectLabel rectFromCenter(21.5, 11.25, fractionalCenter, 0, Qt::red, 1.5);
    check(rectFromBounds.imagePos() == fractionalCenter
          && rectFromCenter.imagePos() == fractionalCenter
          && rectFromBounds.boundingRect() == rectFromCenter.boundingRect()
          && qFuzzyCompare(rectFromCenter.boundingRect().width(),
                           qreal(21.5 + 2 * (1.5 * 0.7071067811865476 + 1.0)))
          && qFuzzyCompare(rectFromCenter.boundingRect().height(),
                           qreal(11.25 + 2 * (1.5 * 0.7071067811865476 + 1.0))),
          "fractional rectangle geometry was not preserved");

    const QPen customPen(Qt::blue, 3.5, Qt::DashLine, Qt::SquareCap);
    ImageRotatedRectLabel styledRect(fractionalRect, 0, Qt::red, 1.5);
    styledRect.setPen(customPen);
    check(styledRect.getPen() == customPen,
          "rectangle pen settings were not retained");
    styledRect.onViewScaleChanged(2);
    {
        QPainter painter(&contourRender);
        styledRect.paint(&painter, nullptr, nullptr);
        check(painter.pen().color() == Qt::blue
              && painter.pen().style() == Qt::DashLine
              && painter.pen().capStyle() == Qt::SquareCap
              && qFuzzyCompare(painter.pen().widthF(), qreal(1.75)),
              "rectangle pen settings or scaled width were not applied");
    }
    const auto setAndCheckPen = [&customPen](ImageLabel &label) {
        label.setPen(customPen);
        return label.getPen() == customPen;
    };
    ImagePointLabel penPoint(QPointF(5, 5));
    ImageEllipseLabel penEllipse(QPointF(5, 5), 3);
    ImageArrowLabel penArrow(QPointF(0, 0), QPointF(10, 10));
    ImageContourLabel penContour(contourPoints);
    check(setAndCheckPen(penPoint) && setAndCheckPen(penEllipse)
          && setAndCheckPen(penArrow) && setAndCheckPen(penContour),
          "a pen-capable label did not retain its pen");
    softMask.setPen(customPen);
    check(softMask.getPen() == QPen(), "mask label unexpectedly accepted a pen");

    ImageEllipseLabel ellipseFromBounds(fractionalRect, 0, Qt::red, 1.5);
    ImageEllipseLabel ellipseFromRadii(fractionalCenter, 10.75, 5.625, 0, Qt::red, 1.5);
    ImageEllipseLabel circle(fractionalCenter, 5.625, Qt::red, 1.5);
    ImageEllipseLabel circleWithColor(fractionalCenter, 5.625, QColor(Qt::red), 1.5);
    check(ellipseFromBounds.imagePos() == fractionalCenter
          && ellipseFromBounds.boundingRect() == rectFromBounds.boundingRect(),
          "fractional ellipse rectangle was not preserved");
    check(ellipseFromRadii.imagePos() == fractionalCenter
          && ellipseFromRadii.boundingRect() == ellipseFromBounds.boundingRect(),
          "fractional ellipse geometry was not preserved");
    check(circle.imagePos() == fractionalCenter
          && circle.boundingRect() == circleWithColor.boundingRect()
          && qFuzzyCompare(circle.boundingRect().width(), rectFromCenter.boundingRect().height())
          && qFuzzyCompare(circle.boundingRect().height(), rectFromCenter.boundingRect().height()),
          "fractional circle geometry was not preserved");

    ImageArrowLabel fractionalArrow(QPointF(1.25, 2.5), QPointF(10.75, 8.5),
                                    Qt::red, 1.5, 3.25);
    check(fractionalArrow.imagePos() == QPointF(6, 5.5)
          && fractionalArrow.boundingRect() == QRectF(-9.75, -8, 19.5, 16),
          "fractional arrow geometry was not preserved");
    const auto penWidth = [&contourRender](ImageLabel &label) {
        QPainter painter(&contourRender);
        label.paint(&painter, nullptr, nullptr);
        return painter.pen().widthF();
    };
    check(qFuzzyCompare(penWidth(rectFromCenter), qreal(1.5))
          && qFuzzyCompare(penWidth(ellipseFromRadii), qreal(1.5))
          && qFuzzyCompare(penWidth(fractionalArrow), qreal(1.5)),
          "fractional line width was not preserved");

    ImageLineLabel segment(QPointF(10, 20), QPointF(30, 20), Qt::red, 2);
    check(segment.imagePos() == QPointF(20, 20)
          && segment.boundingRect().contains(QPointF(-10, 0))
          && segment.boundingRect().contains(QPointF(10, 0)),
          "line segment geometry is incorrect");
    segment.onViewScaleChanged(2);
    segment.setPen(customPen);
    check(segment.getPen() == customPen && qFuzzyCompare(penWidth(segment), qreal(1.75)),
          "line pen or scaled width is incorrect");

    ImageLabelView lineWidget;
    lineWidget.resize(650, 400);
    lineWidget.show();
    app.processEvents();
    auto *infiniteLine = new ImageLineLabel(QPointF(20, 30), QPointF(30, 30),
                                            Qt::red, 2, true);
    auto *angledLine = new ImageLineLabel(QPointF(25, 30), 90, Qt::blue, 2);
    lineWidget.addLabel(infiniteLine);
    lineWidget.addLabel(angledLine);
    QImage lineImage(100, 80, QImage::Format_RGB32);
    lineImage.fill(Qt::white);
    lineWidget.setImage(lineImage);
    auto *lineView = lineWidget.findChild<QGraphicsView *>("graphicsView");
    check(lineView, "line view is missing");
    const auto checkViewportLine = [&]() {
        app.processEvents();
        const int y = lineView->mapFromScene(QPointF(0, 30)).y();
        check(y >= 0 && y < lineView->viewport()->height(), "line left the test viewport");
        const int right = lineView->viewport()->width() - 3;
        const QPointF leftScene = lineView->mapToScene(QPoint(2, y));
        const QPointF rightScene = lineView->mapToScene(QPoint(right, y));
        check(infiniteLine->boundingRect().translated(infiniteLine->pos()).contains(leftScene)
              && infiniteLine->boundingRect().translated(infiniteLine->pos()).contains(rightScene),
              "infinite line bounds do not cover the viewport");
        QImage render(lineView->viewport()->size(), QImage::Format_ARGB32_Premultiplied);
        render.fill(Qt::transparent);
        lineView->viewport()->render(&render);
        check(render.pixelColor(2, y).red() > 200 && render.pixelColor(2, y).green() < 80
              && render.pixelColor(right, y).red() > 200 && render.pixelColor(right, y).green() < 80,
              "infinite line was not rendered at both viewport edges");
    };
    check(lineView->mapToScene(QPoint(2, lineView->mapFromScene(QPointF(0, 30)).y())).x() < 0,
          "test viewport does not extend beyond the image");
    checkViewportLine();
    {
        const int x = lineView->mapFromScene(QPointF(25, 30)).x();
        QImage render(lineView->viewport()->size(), QImage::Format_ARGB32_Premultiplied);
        render.fill(Qt::transparent);
        lineView->viewport()->render(&render);
        const int bottom = render.height() - 3;
        check(render.pixelColor(x, 2).blue() > 200 && render.pixelColor(x, 2).red() < 80
              && render.pixelColor(x, bottom).blue() > 200 && render.pixelColor(x, bottom).red() < 80,
              "vertical line was not rendered at both viewport edges");
    }
    lineWidget.resize(800, 400);
    checkViewportLine();
    const QPoint zoomPos = lineView->mapFromScene(QPointF(25, 30));
    QWheelEvent lineWheel(QPointF(zoomPos), QPointF(lineView->viewport()->mapToGlobal(zoomPos)),
                          QPoint(), QPoint(0, 120), Qt::NoButton,
                          Qt::NoModifier, Qt::NoScrollPhase, false);
    for (int i = 0; i < 10; ++i)
        QApplication::sendEvent(lineView->viewport(), &lineWheel);
    lineView->centerOn(QPointF(25, 30));
    checkViewportLine();
    QScrollBar *horizontal = lineView->horizontalScrollBar();
    check(horizontal->maximum() > horizontal->minimum(), "test view cannot pan horizontally");
    const int oldScroll = horizontal->value();
    horizontal->setValue(oldScroll == horizontal->minimum() ? horizontal->maximum()
                                                            : horizontal->minimum());
    check(horizontal->value() != oldScroll, "test view did not pan");
    checkViewportLine();
    lineWidget.setImage(QImage());
    checkViewportLine();
    ImageLineLabel outsideLine(QPointF(0, -10), QPointF(10, -10), Qt::red, 2, true);
    outsideLine.onViewRectChanged(QRectF(0, 0, 100, 80));
    check(outsideLine.boundingRect().isEmpty(), "outside infinite line was not clipped");
    ImageLineLabel invalidLine(QPointF(5, 5), QPointF(5, 5), Qt::red, 2, true);
    invalidLine.onViewRectChanged(QRectF(0, 0, 100, 80));
    check(invalidLine.boundingRect().isEmpty(), "infinite line with identical points was drawn");

    fractionalArrow.onViewScaleChanged(2);
    check(fractionalArrow.boundingRect() == QRectF(-7.75, -6, 15.5, 12)
          && qFuzzyCompare(penWidth(fractionalArrow), qreal(0.75)),
          "fractional arrow dimensions did not scale correctly");

    QImage image(100, 80, QImage::Format_ARGB32);
    image.fill(QColor(12, 34, 56, 78));
    widget.setImage(image);
    check(size->text().contains("100x80"), "image dimensions not updated");
    check(view->transform().m11() > 0, "image fit scale is invalid");

    auto *point = new CountingPoint(QPointF(10.5, 20.5));
    auto *text = new ImageTextLabel(QPointF(20.5, 30.5), "test");
    auto *rect = new ImageRotatedRectLabel(21, 11, QPoint(40, 30), 30.5);
    auto *ellipse = new ImageEllipseLabel(QPoint(50, 40), 12, 8, 30.5);
    const ImageRotatedRectLabel integerAngleRect(21, 11, QPoint(40, 30), 30);
    const ImageEllipseLabel integerAngleEllipse(QPoint(50, 40), 12, 8, 30.0);
    check(!qFuzzyCompare(rect->boundingRect().width(), integerAngleRect.boundingRect().width())
          && !qFuzzyCompare(ellipse->boundingRect().width(), integerAngleEllipse.boundingRect().width()),
          "fractional angles were truncated");
    auto *arrow = new ImageArrowLabel(QPoint(10, 10), QPoint(21, 15));
    auto *viewContour = new ImageContourLabel(contourPoints);
    widget.addLabel(point);
    widget.addLabel(text);
    widget.addLabel(rect);
    widget.addLabel(ellipse);
    widget.addLabel(arrow);
    widget.addLabel(viewContour);
    check(view->scene()->items().size() == 7, "labels were not added");
    check(point->pos() == QPointF(10.5, 20.5) && text->pos() == QPointF(20.5, 30.5),
          "fractional positions lost");
    check(rect->pos() == QPointF(40, 30) && ellipse->pos() == QPointF(50, 40)
          && arrow->pos() == QPointF(15.5, 12.5), "geometry centers incorrect");
    check(viewContour->pos() == QPointF(20, 20), "contour position is incorrect");
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
    const QRectF contourBounds = viewContour->boundingRect();
    QWheelEvent wheel(QPointF(mouse), QPointF(view->viewport()->mapToGlobal(mouse)),
                      QPoint(), QPoint(0, 120), Qt::NoButton, Qt::NoModifier,
                      Qt::NoScrollPhase, false);
    QApplication::sendEvent(view->viewport(), &wheel);
    check(std::abs(view->transform().m11() - before * 1.15) < 0.0001, "wheel zoom failed");
    check(arrow->boundingRect().width() < arrowBounds.width(), "arrow size did not adapt to zoom");
    check(viewContour->boundingRect() == contourBounds,
          "small view zoom unnecessarily changed contour bounds");
    QApplication::sendEvent(view->viewport(), &wheel);
    check(viewContour->boundingRect().width() < contourBounds.width(),
          "contour thickness did not adapt to cumulative view zoom");
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
    check(view->scene()->items().size() == 7, "duplicate label added");
    QGraphicsScene otherScene;
    auto *otherPoint = new CountingPoint(QPoint(2, 2));
    otherScene.addItem(otherPoint);
    widget.addLabel(otherPoint);
    check(otherPoint->scene() == &otherScene && view->scene()->items().size() == 7,
          "label was transferred from another scene");
    auto *viewFlash = new ImageContourFastLabel(contourPoints, Qt::red, 1, 255);
    widget.addLabel(viewFlash);
    const QPointF flashScenePosition = viewFlash->pos();
    const QRectF sceneBounds = viewFlash->boundingRect();
    viewFlash->onViewScaleChanged(0.25);
    check(viewFlash->pos() == flashScenePosition
          && viewFlash->boundingRect().width() > sceneBounds.width(),
          "flash contour scene position or bounds changed incorrectly on zoom");
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
