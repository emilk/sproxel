#include <iostream>
#include <QPainter>
#include <QPaintEvent>
#include <QColorDialog>
#include "PaletteWidget.h"


#define CBOX_W 40
#define CBOX_H 40
#define CBOX_AX 17
#define CBOX_AY  5
#define CBOX_PX 42
#define CBOX_PY 30

#define HR_Y 75

#define PAL_X  5
#define PAL_Y 85
#define PAL_NX  8
#define PAL_NY 32
#define PBOX_W 11
#define PBOX_H 11


QSize PaletteWidget::minimumSizeHint() const
{
  return QSize(100, PAL_Y+PAL_NY*PBOX_H+PBOX_H/2);
}


QSize PaletteWidget::sizeHint() const
{
  return QSize(100, PAL_Y+PAL_NY*PBOX_H+PBOX_H/2);
}


QColor PaletteWidget::toQColor(const Imath::Color4f& in)
{
    return QColor((int)(in.r * 255.0f),
                  (int)(in.g * 255.0f),
                  (int)(in.b * 255.0f),
                  (int)(in.a * 255.0f));
}


QColor PaletteWidget::toQColor(const Imath::Color4f& in, float a)
{
    return QColor((int)(in.r * 255.0f),
                  (int)(in.g * 255.0f),
                  (int)(in.b * 255.0f),
                  (int)(   a * 255.0f));
}


Imath::Color4f PaletteWidget::toColor4f(QColor c)
{
  return Imath::Color4f(
    c.red  ()/255.0f,
    c.green()/255.0f,
    c.blue ()/255.0f,
    c.alpha()/255.0f);
}


void PaletteWidget::setPalette(ColorPalettePtr pal)
{
  m_palette=pal;
  update();
}


void PaletteWidget::paintEvent(QPaintEvent*)
{
  QPainter painter(this);

  // Some useful colors|tools
  Imath::Color4f brighterBackground = m_backgroundColor * 1.05f;
  brighterBackground.a = 1.0f;
  QPen brighterBackgroundPen = QPen(toQColor(brighterBackground));

  // BG clear
  painter.fillRect(0, 0, width(), height(), QBrush(toQColor(m_backgroundColor)));

  // Active and passive colors get drawn in the upper-left
  painter.fillRect(CBOX_PX  , CBOX_PY  , CBOX_W  , CBOX_H  , QBrush(QColor(0,0,0)));
  painter.fillRect(CBOX_PX+1, CBOX_PY+1, CBOX_W-2, CBOX_H-2, QBrush(QColor(255, 255, 255)));
  painter.fillRect(CBOX_PX+2, CBOX_PY+2, CBOX_W-4, CBOX_H-4, QBrush(toQColor(m_passiveColor, 1)));

  painter.fillRect(CBOX_AX  , CBOX_AY  , CBOX_W  , CBOX_H  , QBrush(QColor(0,0,0)));
  painter.fillRect(CBOX_AX+1, CBOX_AY+1, CBOX_W-2, CBOX_H-2, QBrush(QColor(255, 255, 255)));
  painter.fillRect(CBOX_AX+2, CBOX_AY+2, CBOX_W-4, CBOX_H-4, QBrush(toQColor(m_activeColor, 1)));

  // Horizontal rule
  painter.setPen(brighterBackgroundPen);
  painter.drawLine(QPoint(0, HR_Y), QPoint(width(), HR_Y));

  // Palette grid
  if (m_palette)
  {
    //painter.fillRect(PAL_X-1, PAL_Y-1, PAL_NX*PBOX_W+1, PAL_NY*PBOX_H+1, QBrush(QColor(0, 0, 0)));

    for (int y=0; y<PAL_NY; ++y)
      for (int x=0; x<PAL_NX; ++x)
      {
        Imath::Color4f c=m_palette->color(y*PAL_NX+x);
        painter.fillRect(PAL_X+x*PBOX_W, PAL_Y+y*PBOX_H, PBOX_W, PBOX_H, toQColor(c, 1));
      }

    if (m_hilightIndex>=0)
    {
      int x=PAL_X+(m_hilightIndex%PAL_NX)*PBOX_W;
      int y=PAL_Y+(m_hilightIndex/PAL_NX)*PBOX_H;
      painter.fillRect(x  , y  , PBOX_W  , PBOX_H  , QColor(0, 0, 0));
      painter.fillRect(x+1, y+1, PBOX_W-2, PBOX_H-2, QColor(255, 255, 255));
      painter.fillRect(x+2, y+2, PBOX_W-4, PBOX_H-4, toQColor(m_palette->color(m_hilightIndex), 1));
    }
  }
}


void PaletteWidget::mousePressEvent(QMouseEvent* event)
{
  QColor color;

  int ci=clickHit(event->pos());

  m_hilightIndex=ci; update();

  switch(ci)
  {
    case HIT_NONE:
      break;

    case HIT_ACTIVE_COLOR_BOX:
      color = QColorDialog::getColor(toQColor(m_activeColor), this,
        "Select active color", QColorDialog::ShowAlphaChannel);

      if (color.isValid()) setActiveColor(toColor4f(color), -1);
      break;

    case HIT_PASSIVE_COLOR_BOX:
      color = QColorDialog::getColor(toQColor(m_passiveColor), this,
        "Select passive color", QColorDialog::ShowAlphaChannel);

      if (color.isValid()) setPassiveColor(toColor4f(color), -1);
      break;

    default:
      if (event->button()==Qt::LeftButton)
      {
        if (m_palette) setActiveColor(m_palette->color(ci), ci);
      }
      else if (event->button()==Qt::RightButton)
      {
        if (m_palette)
        {
          QString tmpStr; tmpStr.setNum(ci);
          color = QColorDialog::getColor(toQColor(m_palette->color(ci)), this,
            "Select palette color #"+tmpStr, QColorDialog::ShowAlphaChannel);

          if (color.isValid()) m_palette->setColor(ci, toColor4f(color));
        }
      }
      break;
  }
}


void PaletteWidget::mouseMoveEvent(QMouseEvent* event)
{
  int ci=clickHit(event->pos());
  if (m_hilightIndex!=ci)
  {
    int x=PAL_X+(m_hilightIndex%PAL_NX)*PBOX_W;
    int y=PAL_Y+(m_hilightIndex/PAL_NX)*PBOX_H;
    m_hilightIndex=ci;
    repaint(x, y, PBOX_W, PBOX_H);

    x=PAL_X+(ci%PAL_NX)*PBOX_W;
    y=PAL_Y+(ci/PAL_NX)*PBOX_H;
    repaint(x, y, PBOX_W, PBOX_H);
  }
}


void PaletteWidget::mouseDoubleClickEvent(QMouseEvent* /*event*/)
{
  // Emit palette changed signal
  // emit activeColorChanged(Imath::Color4f(1.0f, 0.0f, 1.0f, 1.0f));

  //std::cout << event->pos().x() << " " << event->pos().y() << std::endl;
  //std::cout << clickHit(event->pos()) << std::endl;
}


int PaletteWidget::clickHit(const QPoint& p)
{
  if (p.x() >= CBOX_AX && p.y() >= CBOX_AY && p.x() < CBOX_AX+CBOX_W && p.y() < CBOX_AY+CBOX_H)
    return HIT_ACTIVE_COLOR_BOX;
  else if (p.x() >= CBOX_PX && p.y() >= CBOX_PY && p.x() < CBOX_PX+CBOX_W && p.y() < CBOX_PY+CBOX_H)
    return HIT_PASSIVE_COLOR_BOX;

  int nx=p.x()-PAL_X, ny=p.y()-PAL_Y;
  if (nx>=0 && ny>=0)
  {
    nx/=PBOX_W;
    ny/=PBOX_H;
    if (nx<PAL_NX && ny<PAL_NY) return ny*PAL_NX+nx;
  }

  return HIT_NONE;
}


void PaletteWidget::setActiveColor(const Imath::Color4f& color, int index)
{
    m_activeColor = color;
    m_activeIndex = index;
    emit activeColorChanged(m_activeColor, m_activeIndex);
    repaint(0, 0, width(), HR_Y);
}


void PaletteWidget::setPassiveColor(const Imath::Color4f& color, int index)
{
    m_passiveColor = color;
    m_passiveIndex = index;
    //emit activeColorChanged(m_activeColor);
    repaint(0, 0, width(), HR_Y);
}


void PaletteWidget::swapColors()
{
    Imath::Color4f copy = m_activeColor; m_activeColor = m_passiveColor; m_passiveColor = copy;
    int            icpy = m_activeIndex; m_activeIndex = m_passiveIndex; m_passiveIndex = icpy;
    emit activeColorChanged(m_activeColor, m_activeIndex);
    repaint(0, 0, width(), HR_Y);
}
