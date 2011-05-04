#ifndef __PALETTE_WIDGET_H__
#define __PALETTE_WIDGET_H__

#include <QWidget>
#include <ImathColor.h>

class PaletteWidget : public QWidget
{
    Q_OBJECT
            
public:
    PaletteWidget(QWidget* parent = NULL) : 
        QWidget(parent),
        m_activeColor(1.0f, 1.0f, 1.0f, 1.0f),
        m_passiveColor(0.0f, 0.0f, 0.0f, 1.0f),
        m_backgroundColor(0.631f, 0.631f, 0.631f, 1.0f)
        {}
    ~PaletteWidget() {}
    
    QSize minimumSizeHint() const;
    QSize sizeHint() const;

signals:
    void activeColorChanged(const Imath::Color4f& color);

public slots:
    void setActiveColor(const Imath::Color4f& color);
    void setPassiveColor(const Imath::Color4f& color);
    void swapColors();

protected:
    void paintEvent(QPaintEvent *event);

    virtual void mousePressEvent(QMouseEvent* event);
    virtual void mouseDoubleClickEvent(QMouseEvent* event);

private:
    Imath::Color4f m_activeColor;
    Imath::Color4f m_passiveColor;
    Imath::Color4f m_backgroundColor;
    
    QColor toQColor(const Imath::Color4f& in);
    
    enum HitType { HIT_NONE, HIT_ACTIVE_COLOR_BOX, HIT_PASSIVE_COLOR_BOX };
    HitType clickHit(const QPoint& clickSpot);
};

#endif
