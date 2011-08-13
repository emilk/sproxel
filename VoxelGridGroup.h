#ifndef __VOXEL_GRID_GROUP_H__
#define __VOXEL_GRID_GROUP_H__


#include <ImathBox.h>
#include <ImathVec.h>
#include <ImathColor.h>
#include <QString>

#include "GameVoxelGrid.h"


//ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//


typedef Imath::Color4f SproxelColor;
typedef unsigned char  SproxelIndex;


typedef GameVoxelGrid<SproxelColor> RgbVoxelGrid;
typedef GameVoxelGrid<SproxelIndex> IndVoxelGrid;


//ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//


inline float color_diff(const SproxelColor &a, const SproxelColor &b)
{
  SproxelColor d=a-b; d*=d;
  return d.r+d.g+d.b+d.a;
}


class ColorPalette
{
protected:
  std::vector<SproxelColor> m_colors;
  QString m_name;

public:

  ColorPalette() {}

  template<class I> ColorPalette(I first, I last) : m_colors(first, last) {}


  const QString& name() const { return m_name; }
  void setName(const QString &n) { m_name=n; }

  void resize(int new_size)
  {
    if (new_size<0) new_size=0;
    m_colors.resize(new_size, SproxelColor(0, 0, 0, 0));
  }

  SproxelColor color(int i) const
  {
    if (i<0 || i>=m_colors.size()) return SproxelColor(0, 0, 0, 0);
    return m_colors[i];
  }

  void setColor(int i, const SproxelColor &c)
  {
    if (i<0) return;
    if (i>=m_colors.size()) resize(i+1);
    m_colors[i]=c;
  }

  int bestMatch(const SproxelColor &c) const
  {
    int bi=-1;
    float bd=FLT_MAX;

    for (int i=0; i<m_colors.size(); ++i)
    {
      float d=color_diff(m_colors[i], c);
      if (d<bd) { bd=d; bi=i; }
    }

    return bi;
  }
};


//ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//


class VoxelGridLayer
{
protected:
  RgbVoxelGrid *m_rgb;
  IndVoxelGrid *m_ind;
  ColorPalette *m_palette;

  Imath::V3i m_offset;
  QString m_name;
  bool m_visible;

  void init()
  {
    m_rgb=NULL;
    m_ind=NULL;
    m_palette=NULL;
    m_offset=Imath::V3i(0);
    m_name="layer";
    m_visible=true;
  }

public:

  enum DataType { TYPE_RGB, TYPE_IND };


  VoxelGridLayer() { init(); }

  VoxelGridLayer(const RgbVoxelGrid &grid, const Imath::V3i ofs=Imath::V3i(0))
  {
    init();
    m_rgb=new RgbVoxelGrid(grid);
    m_offset=ofs;
  }

  VoxelGridLayer(const IndVoxelGrid &grid, ColorPalette *pal=NULL, const Imath::V3i ofs=Imath::V3i(0))
  {
    init();
    m_ind=new IndVoxelGrid(grid);
    m_palette=pal;
    m_offset=ofs;
  }

  void clear()
  {
    if (m_rgb) { delete m_rgb; m_rgb=NULL; }
    if (m_ind) { delete m_ind; m_ind=NULL; }
    init();
  }

  ~VoxelGridLayer()
  {
    clear();
  }

  VoxelGridLayer(const VoxelGridLayer &from) :
    m_rgb    (from.m_rgb    ),
    m_ind    (from.m_ind    ),
    m_palette(from.m_palette),
    m_offset (from.m_offset ),
    m_name   (from.m_name   ),
    m_visible(from.m_visible)
  {
    if (m_rgb) m_rgb=new RgbVoxelGrid(*m_rgb);
    if (m_ind) m_ind=new IndVoxelGrid(*m_ind);
  }

  VoxelGridLayer& operator = (const VoxelGridLayer &from)
  {
    if (&from==this) return *this;

    clear();

    if (from.m_rgb) m_rgb=new RgbVoxelGrid(*from.m_rgb);
    if (from.m_ind) m_ind=new IndVoxelGrid(*from.m_ind);
    m_palette=from.m_palette;
    m_offset =from.m_offset ;
    m_name   =from.m_name   ;
    m_visible=from.m_visible;

    return *this;
  }

  const Imath::V3i& offset() const { return m_offset; }
  void setOffset(const Imath::V3i &o) { m_offset=o; }

  bool isVisible() const { return m_visible; }
  void setVisible(bool v) { m_visible=v; }

  const QString& name() const { return m_name; }
  void setName(const QString &n) { m_name=n; }

  ColorPalette* palette() const { return m_palette; }
  void setPalette(ColorPalette *p) { m_palette=p; }

  Imath::V3i size() const
  {
    if (m_ind) return m_ind->cellDimensions();
    if (m_rgb) return m_rgb->cellDimensions();
    return Imath::V3i(0);
  }

  Imath::Box3i bounds() const
  {
    return Imath::Box3i(m_offset, m_offset+size()-Imath::V3i(1));
  }

  void resize(const Imath::Box3i &new_box)
  {
    // expand grid and adjust offset to match new box
    Imath::Box3i curBox=bounds();

    if (m_ind)
    {
      m_ind->resize(new_box.size()+Imath::V3i(1), new_box.min-curBox.min, 0);
      m_offset=new_box.min;
    }
    else if (m_rgb)
    {
      m_rgb->resize(new_box.size()+Imath::V3i(1), new_box.min-curBox.min, SproxelColor(0, 0, 0, 0));
      m_offset=new_box.min;
    }
    else if (!new_box.isEmpty())
    {
      // no grids yet - create a default RGB one
      m_rgb=new RgbVoxelGrid(new_box.size()+Imath::V3i(1));
      m_rgb->setAll(SproxelColor(0, 0, 0, 0));
      m_offset=new_box.min;
    }
  }

  int getInd(const Imath::V3i &at) const
  {
    if (!m_ind) return -1;
    if (!bounds().intersects(at)) return -1;
    return m_ind->get(at-m_offset);
  }

  SproxelColor getColor(const Imath::V3i &at) const
  {
    if (!bounds().intersects(at)) return SproxelColor(0, 0, 0, 0);
    if (m_ind && m_palette) return m_palette->color(m_ind->get(at-m_offset));
    if (m_rgb) return m_rgb->get(at-m_offset);
    return SproxelColor(0, 0, 0, 0);
  }

  void set(const Imath::V3i &at, const SproxelColor &color, int index=-1)
  {
    // expand grid to include target voxel
    Imath::Box3i box=bounds();
    box.extendBy(at);
    resize(box);

    if (m_ind)
    {
      if (index<0)
      {
        if (m_palette)
        {
          //== TODO: could expand palette instead, up to some maximum number of colors
          index=m_palette->bestMatch(color);
        }
        else
          index=0;
      }

      // set voxel index
      m_ind->set(at-m_offset, index);
    }
    else if (m_rgb)
    {
      // set voxel color
      m_rgb->set(at-m_offset, color);
    }
  }

  DataType dataType() const { return m_ind ? TYPE_IND : TYPE_RGB; }
};


//ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//


class VoxelGridGroup
{
private:
  Imath::M44d m_transform;
  std::vector<VoxelGridLayer*> m_layers;
  int m_curLayer;

public:

  VoxelGridGroup(VoxelGridLayer *layer=NULL) : m_transform(), m_curLayer(-1)
  {
    if (layer)
    {
      m_layers.push_back(layer);
      m_curLayer=0;
    }
  }

  VoxelGridGroup(const Imath::V3i &size) : m_transform()
  {
    VoxelGridLayer *layer=new VoxelGridLayer();
    layer->resize(Imath::Box3i(Imath::V3i(0), size-Imath::V3i(1)));
    layer->setName("main layer");

    m_layers.push_back(layer);
    m_curLayer=0;
  }

  VoxelGridGroup(const VoxelGridGroup &from)
  {
    m_transform=from.m_transform;
    m_curLayer =from.m_curLayer ;

    m_layers.reserve(from.m_layers.size());
    for (int i=0; i<from.m_layers.size(); ++i)
      m_layers.push_back(new VoxelGridLayer(*from.m_layers[i]));
  }

  VoxelGridGroup& operator = (const VoxelGridGroup &from)
  {
    if (&from==this) return *this;

    clear();

    m_transform=from.m_transform;
    m_curLayer =from.m_curLayer ;

    m_layers.reserve(from.m_layers.size());
    for (int i=0; i<from.m_layers.size(); ++i)
      m_layers.push_back(new VoxelGridLayer(*from.m_layers[i]));

    return *this;
  }

  void clear()
  {
    m_transform.makeIdentity();
    m_curLayer=-1;

    for (int i=0; i<m_layers.size(); ++i) if (m_layers[i]) delete(m_layers[i]);
    m_layers.clear();
  }

  ~VoxelGridGroup() { clear(); }

  // General accessors
  const Imath::M44d& transform() const { return m_transform; }
  void setTransform(const Imath::M44d& m) { m_transform = m; }


  int curLayerIndex() const { return m_curLayer; }

  VoxelGridLayer* curLayer() const
  {
    if (m_curLayer<0) return NULL;
    return m_layers[m_curLayer];
  }

  void setCurLayer(int index)
  {
    if (index<0 || index>=m_layers.size()) index=-1;
    m_curLayer=index;
  }


  Imath::Box3i bounds() const
  {
    Imath::Box3i bbox;
    for (int i=0; i<m_layers.size(); ++i) bbox.extendBy(m_layers[i]->bounds());
    return bbox;
  }


  // Layer accessors
  int numLayers() const { return m_layers.size(); }

  VoxelGridLayer* layer(int i) const { return m_layers[i]; }

  VoxelGridLayer* insertLayerAbove(int i, VoxelGridLayer *layer=NULL)
  {
    if (!layer) layer=new VoxelGridLayer();
    m_layers.insert(m_layers.begin()+i, layer);
    if (m_curLayer>=i) ++m_curLayer;
    return layer;
  }

  void deleteLayer(int i)
  {
    if (i<0 || i>=m_layers.size()) return;
    if (m_layers[i]) delete(m_layers[i]);
    m_layers.erase(m_layers.begin()+i);

    if (m_curLayer>i) --m_curLayer;
    if (m_curLayer>=m_layers.size()) m_curLayer=m_layers.size()-1;
  }

  bool layerVisible(int i)
  {
    if (i<0 || i>=m_layers.size()) return false;
    return m_layers[i]->isVisible();
  }

  QString layerName(int i)
  {
    if (i<0 || i>=m_layers.size()) return "";
    return m_layers[i]->name();
  }


  Imath::V3d voxelCenter(const Imath::V3i& v) const
  {
    return Imath::V3d(
      (double)v.x + 0.5,
      (double)v.y + 0.5,
      (double)v.z + 0.5
    );
  }

  const Imath::M44d voxelTransform(const Imath::V3i& v) const
  {
    Imath::M44d vMat;
    vMat.setTranslation(voxelCenter(v));
    return vMat * m_transform;
  }

  const Imath::Box3d worldBounds() const
  {
    const Imath::Box3i box=bounds();
    const Imath::Box3d retBox(box.min, box.max);

    // (ImathBoxAlgo) This properly computes the world bounding box
    return Imath::transform(retBox, m_transform);
  }


  SproxelColor get(const Imath::V3i &at)
  {
    SproxelColor result(0, 0, 0, 0);

    for (int i=0; i<m_layers.size(); ++i)
    {
      SproxelColor c=m_layers[i]->getColor(at);
      if (c.a!=0) { result=c; break; }
    }

    return result;
  }

  void set(const Imath::V3i &at, const SproxelColor &color, int index=-1)
  {
    VoxelGridLayer *layer=curLayer();
    if (layer) layer->set(at, color, index);
  }

  //== setAll

  //== ray
};


#endif
