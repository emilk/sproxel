#include <QVBoxLayout>
#include "ProjectWidget.h"


#define ICON_SIZE 60
#define WIDGET_W ((ICON_SIZE+10)*1+40)
#define WIDGET_H ((ICON_SIZE+20)*1+30)


//ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//


void SpriteListModel::updateIcon(int i)
{
  if (!m_project) return;

  QImage img(ICON_SIZE, ICON_SIZE, QImage::Format_ARGB32_Premultiplied);

  img.fill(p_appSettings->value("GLModelWidget/backgroundColor", QColor(0, 0, 0)).value<QColor>().rgb());

  VoxelGridGroupPtr spr=m_project->sprites[i];
  Imath::Box3i bounds=spr->bounds();
  Imath::V3i size=bounds.size()+Imath::V3i(1);

  float sx=size.x/float(ICON_SIZE);
  float sy=size.y/float(ICON_SIZE);

  float scale=sx;
  if (sy>scale) scale=sy;
  if (scale<0.25f) scale=0.25f;

  int ox=bounds.min.x-int((ICON_SIZE*scale-size.x)*0.5f);
  int oy=bounds.min.y-int((ICON_SIZE*scale-size.y)*0.5f);

  for (int y=0; y<ICON_SIZE; ++y)
  {
    int gy=int(y*scale)+oy;
    for (int x=0; x<ICON_SIZE; ++x)
    {
      int gx=int(x*scale)+ox;

      uint color=qRgba(0, 0, 0, 0);
      for (int gz=bounds.max.z; gz>=bounds.min.z; --gz)
      {
        SproxelColor c=spr->get(Imath::V3i(gx, gy, gz));
        if (c.a==0) continue;

        color=qRgba(int(c.r*255), int(c.g*255), int(c.b*255), int(c.a*255));
        break;
      }

      if (color) img.setPixel(x, y, color);
    }
  }

  img=img.mirrored();

  m_icons[i].convertFromImage(img);

  QModelIndex index=createIndex(i, 0);
  emit dataChanged(index, index);
}


void SpriteListModel::onSpriteChanged(VoxelGridGroupPtr spr)
{
  if (!m_project) return;

  for (int i=0; i<m_project->sprites.size(); ++i)
    if (m_project->sprites[i]==spr)
    {
      m_icons[i]=QPixmap();
      break;
    }
}


void SpriteListModel::onPaletteChanged(ColorPalettePtr pal)
{
  if (!m_project) return;

  for (int i=0; i<m_project->sprites.size(); ++i)
    if (m_project->sprites[i]->hasPalette(pal))
      m_icons[i]=QPixmap();
}


void SpriteListModel::currentChanged(const QModelIndex &current, const QModelIndex &)
{
  if (current.isValid())
  {
    emit spriteSelected(m_project->sprites[current.row()]);
  }
}


//ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//


ProjectWidget::ProjectWidget(QWidget* parent, UndoManager *um, QSettings *sett)
  : QWidget(parent), p_undoManager(um)
{
  QVBoxLayout *layout=new QVBoxLayout(this);

  m_sprListView=new QListView;
  m_sprListView->setUniformItemSizes(true);
  m_sprListView->setViewMode(QListView::IconMode);
  m_sprListView->setIconSize(QSize(ICON_SIZE, ICON_SIZE));
  m_sprListView->setGridSize(QSize(ICON_SIZE+10, ICON_SIZE+20));
  //m_sprListView->setSpacing(10);
  m_sprListView->setMovement(QListView::Snap);
  m_sprListView->setResizeMode(QListView::Adjust);

  m_sprListModel=new SpriteListModel(sett, this);
  m_sprListView->setModel(m_sprListModel);

  layout->addWidget(m_sprListView);

  connect(p_undoManager, SIGNAL(spriteChanged(VoxelGridGroupPtr)),
    m_sprListModel, SLOT(onSpriteChanged(VoxelGridGroupPtr)));
  connect(p_undoManager, SIGNAL(spriteChanged(VoxelGridGroupPtr)),
    this, SLOT(update()));

  connect(p_undoManager, SIGNAL(paletteChanged(ColorPalettePtr)),
    m_sprListModel, SLOT(onPaletteChanged(ColorPalettePtr)));

  connect(m_sprListView->selectionModel(), SIGNAL(currentChanged(const QModelIndex &, const QModelIndex &)),
    m_sprListModel, SLOT(currentChanged(const QModelIndex &, const QModelIndex &)));

  connect(m_sprListModel, SIGNAL(spriteSelected(VoxelGridGroupPtr)),
    this, SIGNAL(spriteSelected(VoxelGridGroupPtr)));
}


QSize ProjectWidget::minimumSizeHint() const
{
  return QSize(WIDGET_W, WIDGET_H);
}


QSize ProjectWidget::sizeHint() const
{
  return QSize(WIDGET_W, WIDGET_H*2);
}


void ProjectWidget::setProject(SproxelProjectPtr prj)
{
  m_project=prj;
  m_sprListModel->setProject(prj);
  m_sprListView->setCurrentIndex(m_sprListModel->index(0, 0));
}


void ProjectWidget::paintEvent(QPaintEvent *event)
{
  m_sprListModel->updateIcons();
  QWidget::paintEvent(event);
}


void ProjectWidget::newSprite()
{
  //==
}


void ProjectWidget::deleteSelected()
{
  //==
}


void ProjectWidget::duplicateSelected()
{
  //==
}
