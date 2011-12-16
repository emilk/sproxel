#include <QImage>
#include "SproxelProject.h"


VoxelGridLayerPtr VoxelGridLayer::fromQImage(QImage readMe)
{
  QString tempStr;
  tempStr = readMe.text("VoxelGridDimX");
  int sizeX = tempStr.toInt();
  tempStr = readMe.text("VoxelGridDimY");
  int sizeY = tempStr.toInt();
  tempStr = readMe.text("VoxelGridDimZ");
  int sizeZ = tempStr.toInt();

  if (sizeX == 0 || sizeY == 0 || sizeZ == 0) return VoxelGridLayerPtr();

  readMe = readMe.mirrored();

  VoxelGridLayerPtr layer(new VoxelGridLayer());
  layer->resize(Imath::Box3i(Imath::V3i(0), Imath::V3i(sizeX, sizeY, sizeZ)-Imath::V3i(1)));

  for (int slice = 0; slice < sizeZ; slice++)
  {
    const int sliceOffset = slice * sizeX;
    for (int y = 0; y < sizeY; y++)
    {
      for (int x = 0; x < sizeX; x++)
      {
        QRgb pixelValue = readMe.pixel(x+sliceOffset, y);
        Imath::Color4f color(
          (float)qRed  (pixelValue) / 255.0f,
          (float)qGreen(pixelValue) / 255.0f,
          (float)qBlue (pixelValue) / 255.0f,
          (float)qAlpha(pixelValue) / 255.0f);
        layer->set(Imath::V3i(x, y, slice), color);
      }
    }
  }

  return layer;
}


QImage VoxelGridLayer::makeQImage() const
{
  const Imath::Box3i dim=bounds();
  const Imath::V3i cellDim = dim.size()+Imath::V3i(1);

  // TODO: Offer other options besides XY slices?  Directionality?  Ordering?
  const int height = cellDim.y;
  const int width = cellDim.x * cellDim.z;
  QImage writeMe(QSize(width, height), QImage::Format_ARGB32);

  for (int slice = 0; slice < cellDim.z; slice++)
  {
    const int sliceOffset = slice * cellDim.x;
    for (int y = 0; y < cellDim.y; y++)
    {
      for (int x = 0; x < cellDim.x; x++)
      {
        const Imath::Color4f colorScaled = getColor(Imath::V3i(x, y, slice)+dim.min) * 255.0f;
        writeMe.setPixel(x+sliceOffset, y, qRgba(
          (int)colorScaled.r,
          (int)colorScaled.g,
          (int)colorScaled.b,
          (int)colorScaled.a));
      }
    }
  }
  writeMe = writeMe.mirrored();   // QT Bug: mirrored() doesn't preserve text.

  QString tempStr;
  writeMe.setText("SproxelFileVersion", "1");
  writeMe.setText("VoxelGridDimX", tempStr.setNum(cellDim.x));
  writeMe.setText("VoxelGridDimY", tempStr.setNum(cellDim.y));
  writeMe.setText("VoxelGridDimZ", tempStr.setNum(cellDim.z));

  return writeMe;
}
