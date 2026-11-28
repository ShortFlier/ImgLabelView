#pragma once

#include <QColor>
#include <QFont>
#include <QGraphicsItem>
#include <QImage>
#include <QPen>
#include <QPolygonF>


// 点和文字的大小以屏幕像素为单位。
// 几何标签的尺寸以图像像素为单位，线宽以屏幕像素为单位。
// 角度默认参考图片坐标系，顺时针为正
class ImageLabel : public QGraphicsItem
{
public:
    explicit ImageLabel(QGraphicsItem *parent = nullptr) : QGraphicsItem(parent) {}

    //缩放后重绘
    virtual void onViewScaleChanged(qreal viewScale) = 0;

    //返回在图片上的位置
    virtual QPointF imagePos() const = 0;

    virtual void onViewRectChanged(const QRectF &viewRect) { Q_UNUSED(viewRect); }

    // Labels without a pen keep the default no-op behavior.
    virtual QPen getPen() { return QPen(); }
    virtual void setPen(const QPen &pen) { Q_UNUSED(pen); }
};

//交叉点，形状为"+"
class ImagePointLabel : public ImageLabel
{
public:
    /*
    @point "+"的中心位置，图片坐标系
    @size "+"的正方形边长
    @thickness "+"线条宽度
    @angle "+"的旋转角度
    */
    ImagePointLabel(QPointF point, qreal size = 20, QColor color = Qt::red,
                    qreal thickness = 4, qreal angle = 45, QGraphicsItem *parent = nullptr);
    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
    QPointF imagePos() const override { return _point; }
    void onViewScaleChanged(qreal viewScale) override { Q_UNUSED(viewScale); }
    QPen getPen() override { return _pen; }
    void setPen(const QPen &pen) override;

private:
    QPointF _point;
    qreal _size;
    QPen _pen;
    qreal _thickness;
    qreal _angle;
};

//文字显示
class ImageTextLabel : public ImageLabel
{
public:
    /*
    @point 文字的左上角位置，图片坐标系
    */
    ImageTextLabel(QPointF point, QString text, QColor color = Qt::red,
                   QFont font = QFont("Arial", 12), QGraphicsItem *parent = nullptr);
    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
    QPointF imagePos() const override { return _point; }
    void onViewScaleChanged(qreal viewScale) override { Q_UNUSED(viewScale); }

private:
    QPointF _point;
    QString _text;
    QColor _color;
    QFont _font;
};

//矩形、旋转矩形
class ImageRotatedRectLabel : public ImageLabel
{
public:
    /*
    @rect 未旋转的矩形区域，图片坐标系；以矩形中心为旋转中心
    @angle 旋转角度，单位为度；顺时针为正
    */
    ImageRotatedRectLabel(QRectF rect, qreal angle = 0, QColor color = Qt::red,
                          qreal thickness = 4, QGraphicsItem *parent = nullptr);
    /*
    @w、@h 未旋转矩形的宽和高，图片像素
    @center 矩形中心位置，图片坐标系
    @angle 旋转角度，单位为度；顺时针为正
    */
    ImageRotatedRectLabel(qreal w, qreal h, QPointF center, qreal angle = 0, QColor color = Qt::red,
                          qreal thickness = 4, QGraphicsItem *parent = nullptr);
    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
    QPointF imagePos() const override { return _rect.center(); }
    void onViewScaleChanged(qreal viewScale) override;
    QPen getPen() override { return _pen; }
    void setPen(const QPen &pen) override;

private:
    QRectF _rect;
    qreal _angle;
    QPen _pen;
    qreal _baseThickness;
    qreal _thickness;
};

//圆、椭圆
class ImageEllipseLabel : public ImageLabel
{
public:
    /*
    此函数用于构造圆
    */
    ImageEllipseLabel(QPointF center, qreal radius, QColor color = Qt::red,
                      qreal thickness = 4, QGraphicsItem *parent = nullptr);
    ImageEllipseLabel(QPointF center, qreal radius, Qt::GlobalColor color,
                      qreal thickness = 4, QGraphicsItem *parent = nullptr);
    /*
    @rect 未旋转椭圆的外接矩形，图片坐标系；以矩形中心为旋转中心
    @angle 旋转角度，单位为度；顺时针为正
    */
    ImageEllipseLabel(QRectF rect, qreal angle, QColor color = Qt::red,
                      qreal thickness = 4, QGraphicsItem *parent = nullptr);
    /*
    @center 椭圆中心位置，图片坐标系
    @rx、@ry 椭圆的水平和垂直半径，图片像素
    @angle 旋转角度，单位为度；绕椭圆中心旋转，顺时针为正
    */
    ImageEllipseLabel(QPointF center, qreal rx, qreal ry, qreal angle, QColor color = Qt::red,
                      qreal thickness = 4, QGraphicsItem *parent = nullptr);
    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
    QPointF imagePos() const override { return _rect.center(); }
    void onViewScaleChanged(qreal viewScale) override;
    QPen getPen() override { return _pen; }
    void setPen(const QPen &pen) override;

private:
    QRectF _rect;
    qreal _angle;
    QPen _pen;
    qreal _baseThickness;
    qreal _thickness;
};

//箭头
class ImageArrowLabel : public ImageLabel
{
public:
    /*
    @start、@end 图片坐标中从start指向end的箭头"->"
    @arrowSize ">"的尺寸大小
    */
    ImageArrowLabel(QPointF start, QPointF end, QColor color = Qt::red,
                    qreal thickness = 2, qreal arrowSize = 5, QGraphicsItem *parent = nullptr);
    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
    QPointF imagePos() const override;
    void onViewScaleChanged(qreal viewScale) override;
    QPen getPen() override { return _pen; }
    void setPen(const QPen &pen) override;

private:
    QPointF _start;
    QPointF _end;
    QPen _pen;
    qreal _baseThickness;
    qreal _thickness;
    qreal _baseArrowSize;
    qreal _arrowSize;
};


//掩膜区域
class ImageMaskLabel : public ImageLabel
{
public:
    /*
    @topLeft mask的左上角位置，图片坐标系
    @mask 8位灰度图；threshold小于0时保留灰度强度
    @color 掩膜显示颜色
    @threshold 小于0时不二值化；否则mask中大于threshold的像素显示
    @diaphaneity 透明度，范围0-255；0完全透明，255不透明
    */
    ImageMaskLabel(QPointF topLeft, QImage mask, QColor color = Qt::red,
                   int threshold = -1, int diaphaneity = 127, QGraphicsItem *parent = nullptr);
    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
    QPointF imagePos() const override;
    void onViewScaleChanged(qreal viewScale) override { Q_UNUSED(viewScale); }

protected:
    bool setMask(QPointF topLeft, const QImage &mask, QColor color, int threshold, int diaphaneity);
    QPointF _topLeft;
    QImage _overlay;
};


//闭合轮廓
//如果轮廓点数较多导致卡顿，使用ImageContourFastLabel代替
class ImageContourLabel : public ImageLabel
{
public:
    /*
    @points 图片坐标系中的轮廓点，按顺序连接并自动闭合；空点集不绘制
    @color 轮廓线颜色
    @thickness 轮廓线宽，屏幕像素；缩放时线宽按约20%的变化阈值更新
    */
    ImageContourLabel(const QList<QPointF> &points, QColor color = Qt::red,
                      qreal thickness = 2, QGraphicsItem *parent = nullptr);
    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
    QPointF imagePos() const override { return _center; }
    void onViewScaleChanged(qreal viewScale) override;
    QPen getPen() override { return _pen; }
    void setPen(const QPen &pen) override;

private:
    QPolygonF _points;
    QPointF _center;
    QRectF _pointBounds;
    QPen _pen;
    qreal _baseThickness;
    qreal _thickness;
};


//闭合轮廓高性能版本
class ImageContourFastLabel : public ImageMaskLabel
{
public:
    /*
    @points 图片坐标系中的轮廓点，按顺序连接并自动闭合；空点集不绘制
    @color 轮廓线颜色
    @pixel 缩放比例为1时的目标线宽，屏幕像素，可为小数，最小为1
    @diaphaneity 不透明度，范围0-255
    缩放时图像像素线宽取最接近的奇数（相等时向上），且不小于1。
    */
    ImageContourFastLabel(const QList<QPointF> &points, QColor color = Qt::red,
                           qreal pixel = 1, int diaphaneity = 200, QGraphicsItem *parent = nullptr);
    QRectF boundingRect() const override;
    QPointF imagePos() const override { return _center; }
    void onViewScaleChanged(qreal viewScale) override;

private:
    bool ensureCapacity(int radius);
    QImage _skeleton;
    QImage _distances;
    QPointF _skeletonTopLeft;
    QPointF _center;
    QColor _color;
    qreal _basePixel;
    int _diaphaneity;
    int _maskPixel = 0;
    int _capacityRadius = -1;
};



//线及线段
class ImageLineLabel : public ImageLabel
{
public:
    /*
    @p1、p2 线段上起点和端点，图片坐标系
    @inf 为 true 时绘制直线在视口内的部分；两点不能重合
    */
    ImageLineLabel(QPointF p1, QPointF p2, QColor color = Qt::red, qreal thickness = 2,
                   bool inf = false, QGraphicsItem *parent = nullptr);
    /*
    @p 直线上的一点，图片坐标系
    @angle 直线与x方向的夹角，单位为度，参考图片坐标系，顺时针为正
    */
    ImageLineLabel(QPointF p, qreal angle, QColor color = Qt::red, qreal thickness = 2,
                   QGraphicsItem *parent = nullptr);

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
    QPointF imagePos() const override { return _center; }
    void onViewScaleChanged(qreal viewScale) override;
    void onViewRectChanged(const QRectF &viewRect) override;
    QPen getPen() override { return _pen; }
    void setPen(const QPen &pen) override;

private:
    void updateLine(const QRectF &viewRect);
    QPointF _center;
    QPointF _direction;
    QPointF _start;
    QPointF _end;
    QPen _pen;
    qreal _baseThickness;
    qreal _thickness;
    bool _infinite;
    bool _visible;
};