#ifndef __SPROXEL_PROJECT_H__
#define __SPROXEL_PROJECT_H__


#include "VoxelGridGroup.h"


class SproxelProject : public QSharedData
{
public:
  QVector<VoxelGridGroupPtr> sprites;
  QVector<ColorPalettePtr> palettes;
  ColorPalettePtr mainPalette;


  SproxelProject()
  {
    mainPalette=new ColorPalette();
    mainPalette->resize(256);
    mainPalette->setName("main");
    palettes.push_back(mainPalette);
  }
};


typedef QExplicitlySharedDataPointer<SproxelProject> SproxelProjectPtr;


#endif
