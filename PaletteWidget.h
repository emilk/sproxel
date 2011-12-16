#ifndef __PALETTE_WIDGET_H__
#define __PALETTE_WIDGET_H__

#include <QColor>
#include <QWidget>
#include <QPalette>
#include <ImathColor.h>
#include "VoxelGridGroup.h"


class PaletteWidget : public QWidget
{
    Q_OBJECT

public:
    PaletteWidget(QWidget* parent = NULL) :
        QWidget(parent),
        m_activeColor(1.0f, 1.0f, 1.0f, 1.0f),
        m_passiveColor(0.0f, 0.0f, 0.0f, 1.0f),
        m_hilightIndex(HIT_NONE)
        {
            setMouseTracking(true);
            QPalette pal;
            QColor winColor = pal.color(QPalette::Window);
            m_backgroundColor = Imath::Color4f((float)winColor.red()/255.0f,
                                               (float)winColor.green()/255.0f,
                                               (float)winColor.blue()/255.0f, 1.0f);
        }
    ~PaletteWidget() {}

    QSize minimumSizeHint() const;
    QSize sizeHint() const;

    ColorPalettePtr getPalette() const { return m_palette; }
    void setPalette(ColorPalettePtr pal);

signals:
    void activeColorChanged(const Imath::Color4f& color);

public slots:
    void setActiveColor(const Imath::Color4f& color);
    void setPassiveColor(const Imath::Color4f& color);
    void swapColors();

protected:
    void paintEvent(QPaintEvent *event);

    virtual void mousePressEvent(QMouseEvent* event);
    virtual void mouseMoveEvent(QMouseEvent* event);
    virtual void mouseDoubleClickEvent(QMouseEvent* event);

private:
    Imath::Color4f m_activeColor;
    Imath::Color4f m_passiveColor;
    Imath::Color4f m_backgroundColor;

    ColorPalettePtr m_palette;

    int m_hilightIndex;

    QColor toQColor(const Imath::Color4f& in);
    QColor toQColor(const Imath::Color4f& in, float alpha);
    Imath::Color4f toColor4f(QColor c);

    enum HitType
    {
      HIT_NONE              = -1,
      HIT_ACTIVE_COLOR_BOX  = -2,
      HIT_PASSIVE_COLOR_BOX = -3,
    };

    int clickHit(const QPoint& clickSpot);
};

#endif
