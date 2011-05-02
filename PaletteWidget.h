#ifndef __PALETTE_WIDGET_H__
#define __PALETTE_WIDGET_H__

#include <QWidget>
#include <ImathColor.h>

class PaletteWidget : public QWidget
{
    Q_OBJECT
            
public:
    PaletteWidget(QWidget* parent = NULL) : QWidget(parent) {}
    ~PaletteWidget() {}
    
    QSize minimumSizeHint() const;
    QSize sizeHint() const;

signals:
    void valueChanged(Imath::Color4f color);

protected:
    void paintEvent(QPaintEvent *event);

    void mouseDoubleClickEvent(QMouseEvent* event);

};

#endif
