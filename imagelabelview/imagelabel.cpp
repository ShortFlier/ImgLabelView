#include "imagelabel.h"

#include <QDebug>
#include <QFontMetricsF>
#include <QPainter>
#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

namespace {
QRectF rotatedBounds(const QRectF &rect, qreal angle, qreal thickness)
{
    QTransform transform;
    transform.rotate(angle);
    const qreal margin = thickness * 0.7071067811865476 + 1.0;
    return transform.mapRect(rect).adjusted(-margin, -margin, margin, margin);
}

QRectF centeredRect(const QRectF &rect)
{
    return QRectF(-rect.width() / 2.0, -rect.height() / 2.0, rect.width(), rect.height());
}

qreal scaledThickness(qreal base, qreal scale)
{
    return base / (scale > 0.0 ? scale : 1.0);
}
}

ImagePointLabel::ImagePointLabel(QPointF point, int size, QColor color, int thickness, int angle,
                                 QGraphicsItem *parent)
    : ImageLabel(parent), _point(point), _size(size), _color(color),
      _thickness(thickness), _angle(angle)
{
    setFlag(ItemIgnoresTransformations);
}

QRectF ImagePointLabel::boundingRect() const
{
    const qreal margin = _thickness / 2.0 + 1.0;
    const qreal radius = _size / 2.0 + margin;
    return QRectF(-radius, -radius, 2 * radius, 2 * radius);
}

void ImagePointLabel::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    painter->setPen(QPen(_color, _thickness));
    painter->rotate(_angle);
    const qreal half = _size / 2.0;
    painter->drawLine(QPointF(-half, 0), QPointF(half, 0));
    painter->drawLine(QPointF(0, -half), QPointF(0, half));
}

ImageTextLabel::ImageTextLabel(QPointF point, QString text, QColor color, QFont font,
                               QGraphicsItem *parent)
    : ImageLabel(parent), _point(point), _text(std::move(text)), _color(color), _font(std::move(font))
{
    setFlag(ItemIgnoresTransformations);
}

QRectF ImageTextLabel::boundingRect() const
{
    return QFontMetricsF(_font).boundingRect(_text);
}

void ImageTextLabel::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    painter->setPen(_color);
    painter->setFont(_font);
    painter->drawText(QPointF(0, 0), _text);
}

ImageRotatedRectLabel::ImageRotatedRectLabel(QRect rect, qreal angle, QColor color, int thickness,
                                             QGraphicsItem *parent)
    : ImageLabel(parent), _rect(rect), _angle(angle), _color(color),
      _baseThickness(std::max(qreal(0.1), qreal(thickness))), _thickness(_baseThickness)
{
}

ImageRotatedRectLabel::ImageRotatedRectLabel(int w, int h, QPoint center, qreal angle, QColor color,
                                             int thickness, QGraphicsItem *parent)
    : ImageLabel(parent), _rect(center.x() - w / 2.0, center.y() - h / 2.0, w, h),
      _angle(angle), _color(color), _baseThickness(std::max(qreal(0.1), qreal(thickness))),
      _thickness(_baseThickness)
{
}

QRectF ImageRotatedRectLabel::boundingRect() const
{
    return rotatedBounds(centeredRect(_rect), _angle, _thickness);
}

void ImageRotatedRectLabel::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    QPen pen(_color);
    pen.setWidthF(_thickness);
    painter->setPen(pen);
    painter->rotate(_angle);
    painter->drawRect(centeredRect(_rect));
}

void ImageRotatedRectLabel::onViewScaleChanged(qreal viewScale)
{
    const qreal thickness = scaledThickness(_baseThickness, viewScale);
    if (!qFuzzyCompare(thickness, _thickness)) {
        prepareGeometryChange();
        _thickness = thickness;
        update();
    }
}

ImageEllipseLabel::ImageEllipseLabel(QPoint center, int radius, QColor color, int thickness,
                                     QGraphicsItem *parent)
    : ImageEllipseLabel(center, radius, radius, 0, color, thickness, parent)
{
}

ImageEllipseLabel::ImageEllipseLabel(QRect rect, qreal angle, QColor color, int thickness,
                                     QGraphicsItem *parent)
    : ImageLabel(parent), _rect(rect), _angle(angle), _color(color),
      _baseThickness(std::max(qreal(0.1), qreal(thickness))), _thickness(_baseThickness)
{
}

ImageEllipseLabel::ImageEllipseLabel(QPoint center, int rx, int ry, qreal angle, QColor color,
                                     int thickness, QGraphicsItem *parent)
    : ImageLabel(parent), _rect(center.x() - rx, center.y() - ry, 2.0 * rx, 2.0 * ry),
      _angle(angle), _color(color), _baseThickness(std::max(qreal(0.1), qreal(thickness))),
      _thickness(_baseThickness)
{
}

QRectF ImageEllipseLabel::boundingRect() const
{
    return rotatedBounds(centeredRect(_rect), _angle, _thickness);
}

void ImageEllipseLabel::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    QPen pen(_color);
    pen.setWidthF(_thickness);
    painter->setPen(pen);
    painter->rotate(_angle);
    painter->drawEllipse(centeredRect(_rect));
}

void ImageEllipseLabel::onViewScaleChanged(qreal viewScale)
{
    const qreal thickness = scaledThickness(_baseThickness, viewScale);
    if (!qFuzzyCompare(thickness, _thickness)) {
        prepareGeometryChange();
        _thickness = thickness;
        update();
    }
}

ImageArrowLabel::ImageArrowLabel(QPoint start, QPoint end, QColor color, int thickness,
                                 int arrowSize, QGraphicsItem *parent)
    : ImageLabel(parent), _start(start), _end(end), _color(color),
      _baseThickness(std::max(qreal(0.1), qreal(thickness))), _thickness(_baseThickness),
      _baseArrowSize(std::max(qreal(0.1), qreal(arrowSize))), _arrowSize(_baseArrowSize)
{
}

QPointF ImageArrowLabel::imagePos() const
{
    return (_start + _end) / 2.0;
}

QRectF ImageArrowLabel::boundingRect() const
{
    const QPointF center = imagePos();
    const qreal margin = _arrowSize + _thickness / 2.0 + 1.0;
    return QRectF(_start - center, _end - center).normalized()
        .adjusted(-margin, -margin, margin, margin);
}

void ImageArrowLabel::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    const QPointF start = _start - imagePos();
    const QPointF end = _end - imagePos();
    QPen pen(_color);
    pen.setWidthF(_thickness);
    painter->setPen(pen);
    painter->drawLine(start, end);
    const qreal angle = std::atan2(end.y() - start.y(), end.x() - start.x());
    const qreal offset = 3.14159265358979323846 / 6.0;
    QPolygonF head;
    head << end
         << end - QPointF(std::cos(angle + offset) * _arrowSize, std::sin(angle + offset) * _arrowSize)
         << end - QPointF(std::cos(angle - offset) * _arrowSize, std::sin(angle - offset) * _arrowSize);
    painter->setBrush(_color);
    painter->drawPolygon(head);
}

void ImageArrowLabel::onViewScaleChanged(qreal viewScale)
{
    const qreal thickness = scaledThickness(_baseThickness, viewScale);
    const qreal arrowSize = scaledThickness(_baseArrowSize, viewScale);
    if (!qFuzzyCompare(thickness, _thickness) || !qFuzzyCompare(arrowSize, _arrowSize)) {
        prepareGeometryChange();
        _thickness = thickness;
        _arrowSize = arrowSize;
        update();
    }
}

ImageMaskLabel::ImageMaskLabel(QPointF topLeft, QImage mask, QColor color, int threshold,
                               int diaphaneity, QGraphicsItem *parent)
    : ImageLabel(parent), _topLeft(topLeft)
{
    setMask(topLeft, mask, color, threshold, diaphaneity);
}

bool ImageMaskLabel::setMask(QPointF topLeft, const QImage &mask, QColor color, int threshold,
                             int diaphaneity)
{
    if (mask.isNull()) {
        prepareGeometryChange();
        _topLeft = topLeft;
        _overlay = QImage();
        update();
        return true;
    }
    const QImage grayscaleMask = mask.convertToFormat(QImage::Format_Grayscale8);
    QImage overlay(grayscaleMask.size(), QImage::Format_ARGB32);
    if (grayscaleMask.isNull() || overlay.isNull()) {
        qWarning() << "ImageMaskLabel: failed to allocate mask image";
        return false;
    }
    const int colorAlpha = (color.alpha() * qBound(0, diaphaneity, 255) + 127) / 255;

    for (int y = 0; y < grayscaleMask.height(); ++y) {
        const uchar *maskLine = grayscaleMask.constScanLine(y);
        QRgb *overlayLine = reinterpret_cast<QRgb *>(overlay.scanLine(y));
        for (int x = 0; x < grayscaleMask.width(); ++x) {
            int maskValue = maskLine[x];
            if (threshold >= 0)
                maskValue = maskValue > threshold ? 255 : 0;
            const int alpha = (maskValue * colorAlpha + 127) / 255;
            overlayLine[x] = qRgba(color.red(), color.green(), color.blue(), alpha);
        }
    }
    prepareGeometryChange();
    _topLeft = topLeft;
    _overlay = std::move(overlay);
    update();
    return true;
}

QRectF ImageMaskLabel::boundingRect() const
{
    return QRectF(-_overlay.width() / 2.0, -_overlay.height() / 2.0,
                  _overlay.width(), _overlay.height());
}

void ImageMaskLabel::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    painter->drawImage(boundingRect().topLeft(), _overlay);
}

QPointF ImageMaskLabel::imagePos() const
{
    return _topLeft + QPointF(_overlay.width() / 2.0, _overlay.height() / 2.0);
}

ImageContourLabel::ImageContourLabel(const QList<QPointF> &points, QColor color, int thickness,
                                     QGraphicsItem *parent)
    : ImageLabel(parent), _color(color),
      _baseThickness(std::max(qreal(0.1), qreal(thickness))), _thickness(_baseThickness)
{
    _points.reserve(points.size());
    for (const QPointF &point : points)
        _points << point;
    if (!_points.isEmpty()) {
        _pointBounds = _points.boundingRect();
        _center = _pointBounds.center();
        _points.translate(-_center);
        _pointBounds.translate(-_center);
    }

    this->setCacheMode(QGraphicsItem::DeviceCoordinateCache);
}

QRectF ImageContourLabel::boundingRect() const
{
    if (_points.isEmpty())
        return QRectF();
    const qreal margin = _thickness / 2.0 + 1.0;
    return _pointBounds.adjusted(-margin, -margin, margin, margin);
}

void ImageContourLabel::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    if (_points.isEmpty())
        return;
    QPen pen(_color);
    pen.setWidthF(_thickness);
    pen.setJoinStyle(Qt::RoundJoin);
    pen.setCapStyle(Qt::RoundCap);
    painter->setPen(pen);
    painter->setBrush(Qt::NoBrush);
    if (_points.size() == 1)
        painter->drawPoint(_points.first());
    else
        painter->drawPolygon(_points);
}

void ImageContourLabel::onViewScaleChanged(qreal viewScale)
{
    if (_points.isEmpty())
        return;
    const qreal thickness = scaledThickness(_baseThickness, viewScale);
    if (std::abs(thickness - _thickness) >= _thickness * 0.2) {
        prepareGeometryChange();
        _thickness = thickness;
        update();
    }
}

ImageContourFastLabel::ImageContourFastLabel(const QList<QPointF> &points, QColor color, int pixel,
                                             int diaphaneity, QGraphicsItem *parent)
    : ImageMaskLabel(QPointF(), QImage(), color, -1, diaphaneity, parent),
      _color(color), _basePixel(std::max(1, pixel)), _diaphaneity(diaphaneity)
{
    QPolygonF polygon;
    polygon.reserve(points.size());
    for (const QPointF &point : points) {
        if (!std::isfinite(point.x()) || !std::isfinite(point.y())) {
            qWarning() << "ImageContourFastLabel: non-finite contour point";
            return;
        }
        polygon << point;
    }
    if (polygon.isEmpty())
        return;
    const QRectF bounds = polygon.boundingRect();
    _center = bounds.center();
    const qreal left = std::floor(bounds.left()) - 2;
    const qreal top = std::floor(bounds.top()) - 2;
    const qreal width = std::ceil(bounds.right()) - left + 3;
    const qreal height = std::ceil(bounds.bottom()) - top + 3;
    if (!std::isfinite(_center.x()) || !std::isfinite(_center.y())
        || !std::isfinite(left) || !std::isfinite(top)
        || !std::isfinite(width) || !std::isfinite(height)
        || width < 1 || height < 1
        || width > std::numeric_limits<int>::max() || height > std::numeric_limits<int>::max()) {
        qWarning() << "ImageContourFastLabel: contour bounds are too large";
        _center = QPointF();
        return;
    }
    _skeleton = QImage(int(width), int(height), QImage::Format_RGB32);
    if (_skeleton.isNull()) {
        qWarning() << "ImageContourFastLabel: failed to allocate contour skeleton";
        return;
    }
    _skeletonTopLeft = QPointF(left, top);
    _skeleton.fill(Qt::black);
    {
        QPainter painter(&_skeleton);
        painter.translate(-_skeletonTopLeft);
        painter.setPen(QPen(Qt::white, 1));
        painter.setBrush(Qt::NoBrush);
        if (polygon.size() == 1)
            painter.drawPoint(polygon.first());
        else
            painter.drawPolygon(polygon);
    }
    onViewScaleChanged(1.0);
}

QRectF ImageContourFastLabel::boundingRect() const
{
    return QRectF(_topLeft - _center, _overlay.size());
}

void ImageContourFastLabel::onViewScaleChanged(qreal viewScale)
{
    if (_skeleton.isNull())
        return;
    if (!std::isfinite(viewScale) || viewScale <= 0) {
        qWarning() << "ImageContourFastLabel: invalid view scale" << viewScale;
        return;
    }
    const qreal scaledPixel = std::max(qreal(1), _basePixel / viewScale);
    if (!std::isfinite(scaledPixel) || scaledPixel > std::numeric_limits<int>::max()) {
        qWarning() << "ImageContourFastLabel: contour width is too large";
        return;
    }
    const int radius = int(std::floor(scaledPixel / 2));
    const int pixel = radius * 2 + 1;
    if (pixel == _maskPixel)
        return;
    if (!ensureCapacity(radius))
        return;

    const int alpha = (_color.alpha() * qBound(0, _diaphaneity, 255) + 127) / 255;
    const QRgb active = qRgba(_color.red(), _color.green(), _color.blue(), alpha);
    for (int y = 0; y < _overlay.height(); ++y) {
        const auto *distance = reinterpret_cast<const quint32 *>(_distances.constScanLine(y));
        auto *line = reinterpret_cast<QRgb *>(_overlay.scanLine(y));
        for (int x = 0; x < _overlay.width(); ++x)
            line[x] = distance[x] <= quint32(radius) ? active : qRgba(0, 0, 0, 0);
    }
    _maskPixel = pixel;
    update();
}

bool ImageContourFastLabel::ensureCapacity(int radius)
{
    if (radius <= _capacityRadius)
        return true;
    const qint64 capacity = std::max(qint64(radius), qint64(_capacityRadius) * 2);
    const qint64 width = _skeleton.width() + capacity * 2;
    const qint64 height = _skeleton.height() + capacity * 2;
    if (width > std::numeric_limits<int>::max() || height > std::numeric_limits<int>::max()) {
        qWarning() << "ImageContourFastLabel: contour mask is too large";
        return false;
    }
    QImage distances(int(width), int(height), QImage::Format_RGB32);
    QImage overlay(int(width), int(height), QImage::Format_ARGB32);
    if (distances.isNull() || overlay.isNull()) {
        qWarning() << "ImageContourFastLabel: failed to allocate contour mask";
        return false;
    }
    distances.fill(std::numeric_limits<quint32>::max());
    const int offset = int(capacity);
    for (int y = 0; y < _skeleton.height(); ++y) {
        const auto *source = reinterpret_cast<const QRgb *>(_skeleton.constScanLine(y));
        auto *line = reinterpret_cast<quint32 *>(distances.scanLine(y + offset));
        for (int x = 0; x < _skeleton.width(); ++x) {
            if (qRed(source[x]) != 0)
                line[x + offset] = 0;
        }
    }
    // Two 8-neighbour passes compute the square-dilation distance from the skeleton.
    const quint32 infinity = std::numeric_limits<quint32>::max();
    const int w = distances.width();
    const int h = distances.height();
    for (int y = 0; y < h; ++y) {
        auto *line = reinterpret_cast<quint32 *>(distances.scanLine(y));
        const auto *above = y > 0 ? reinterpret_cast<const quint32 *>(distances.constScanLine(y - 1)) : nullptr;
        for (int x = 0; x < w; ++x) {
            quint32 nearest = infinity;
            if (x > 0) nearest = std::min(nearest, line[x - 1]);
            if (above) {
                nearest = std::min(nearest, above[x]);
                if (x > 0) nearest = std::min(nearest, above[x - 1]);
                if (x + 1 < w) nearest = std::min(nearest, above[x + 1]);
            }
            if (nearest != infinity) line[x] = std::min(line[x], nearest + 1);
        }
    }
    for (int y = h - 1; y >= 0; --y) {
        auto *line = reinterpret_cast<quint32 *>(distances.scanLine(y));
        const auto *below = y + 1 < h ? reinterpret_cast<const quint32 *>(distances.constScanLine(y + 1)) : nullptr;
        for (int x = w - 1; x >= 0; --x) {
            quint32 nearest = infinity;
            if (x + 1 < w) nearest = std::min(nearest, line[x + 1]);
            if (below) {
                nearest = std::min(nearest, below[x]);
                if (x > 0) nearest = std::min(nearest, below[x - 1]);
                if (x + 1 < w) nearest = std::min(nearest, below[x + 1]);
            }
            if (nearest != infinity) line[x] = std::min(line[x], nearest + 1);
        }
    }
    prepareGeometryChange();
    _topLeft = _skeletonTopLeft - QPointF(offset, offset);
    _distances = std::move(distances);
    _overlay = std::move(overlay);
    _capacityRadius = offset;
    return true;
}
