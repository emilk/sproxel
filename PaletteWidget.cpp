#include "PaletteWidget.h"

#include <iostream>
#include <QPainter>
#include <QPaintEvent>

QSize PaletteWidget::minimumSizeHint() const
{
    return QSize(100, 100);
}

QSize PaletteWidget::sizeHint() const
{
    return QSize(100, 200);
}

void PaletteWidget::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    
    painter.fillRect(0, 0, width(), height(), QBrush(QColor(161,161,161,255)));
}

void PaletteWidget::mouseDoubleClickEvent(QMouseEvent*)
{
    // Emit palette changed signal
    emit valueChanged(Imath::Color4f(1.0f, 0.0f, 1.0f, 1.0f));
}
