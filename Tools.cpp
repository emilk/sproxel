#include "Tools.h"
#include "RayWalk.h"
#include "GLModelWidget.h"

////////////////////////////////////////
void SplatToolState::execute()
{
    p_undoManager->beginMacro("Splat");
    std::vector<Imath::V3i> voxels = voxelsAffected();
    for (size_t i = 0; i < voxels.size(); i++)
    {
        p_undoManager->setVoxelColor(p_gvg, voxels[i], m_color, m_index);
    }
    p_undoManager->endMacro();
    decrementClicks();
}


std::vector<Imath::V3i> SplatToolState::voxelsAffected()
{
    std::vector<Imath::V3i> voxels;

    // Intersect and check
    std::vector<Imath::V3i> intersects = rayIntersection(m_ray);
    if (intersects.size() == 0)
        return voxels;

    for (size_t i = 0; i < intersects.size(); i++)
    {
        if (p_gvg->get(intersects[i]).a != 0.0f)
        {
            // Hit a voxel at the close edge of the grid?  Abort.
            if (i == 0)
                break;

            // Hit a voxel in the middle?  Return previous voxel.
            voxels.push_back(intersects[i-1]);
            break;
        }

        // Didn't hit anything?  Just return the last voxel.
        if (i == intersects.size()-1)
        {
            voxels.push_back(intersects[i]);
            break;
        }
    }

    return voxels;
}


////////////////////////////////////////
void FloodToolState::execute()
{
    std::vector<Imath::V3i> voxels = voxelsAffected();
    if (voxels.size() == 0) return;
    const Imath::V3i& hit = voxels[0];

    // Get the color we're replacing.
    const Imath::Color4f repColor = p_gvg->get(hit);

    // Die early if there's nothing to do
    if (repColor == m_color)
        return;

    // Recurse
    p_undoManager->beginMacro("Flood Fill");
    p_undoManager->setVoxelColor(p_gvg, hit, m_color, m_index);
    setNeighborsRecurse(hit, repColor, m_color, m_index);
    p_undoManager->endMacro();
    decrementClicks();
}


void FloodToolState::setNeighborsRecurse(const Imath::V3i& alreadySet,
                                         const Imath::Color4f& repColor,
                                         const Imath::Color4f& newColor,
                                         int newIndex)
{
    // Directions
    Imath::V3i doUs[6];
    doUs[0] = Imath::V3i(alreadySet.x+1, alreadySet.y,   alreadySet.z);
    doUs[3] = Imath::V3i(alreadySet.x-1, alreadySet.y,   alreadySet.z);
    doUs[1] = Imath::V3i(alreadySet.x,   alreadySet.y+1, alreadySet.z);
    doUs[4] = Imath::V3i(alreadySet.x,   alreadySet.y-1, alreadySet.z);
    doUs[2] = Imath::V3i(alreadySet.x,   alreadySet.y,   alreadySet.z+1);
    doUs[5] = Imath::V3i(alreadySet.x,   alreadySet.y,   alreadySet.z-1);

    for (int i = 0; i < 6; i++)
    {
        // Bounds protection
        if (!p_gvg->bounds().intersects(doUs[i])) continue;

        // Recurse
        if (p_gvg->get(doUs[i]) == repColor)
        {
            p_undoManager->setVoxelColor(p_gvg, doUs[i], newColor, newIndex);
            setNeighborsRecurse(doUs[i], repColor, newColor, newIndex);
        }
    }
}


std::vector<Imath::V3i> FloodToolState::voxelsAffected()
{
    // TODO: It may make the most sense to recurse in here, but it could be slow
    std::vector<Imath::V3i> voxels;

    // Intersect and check
    std::vector<Imath::V3i> intersects = rayIntersection(m_ray);
    if (intersects.size() == 0)
        return voxels;

    // Get the first voxel hit
    for (size_t i = 0; i < intersects.size(); i++)
    {
        if (p_gvg->get(intersects[i]).a != 0.0f)
        {
            voxels.push_back(intersects[i]);
            break;
        }
    }

    return voxels;
}


////////////////////////////////////////
void EraserToolState::execute()
{
    p_undoManager->beginMacro("Eraser");
    std::vector<Imath::V3i> voxels = voxelsAffected();
    for (size_t i = 0; i < voxels.size(); i++)
    {
        p_undoManager->setVoxelColor(p_gvg, voxels[i],
                                     Imath::Color4f(0.0f, 0.0f, 0.0f, 0.0f), 0);
    }
    p_undoManager->endMacro();
    decrementClicks();
}


std::vector<Imath::V3i> EraserToolState::voxelsAffected()
{
    std::vector<Imath::V3i> voxels;

    // Intersect and check
    std::vector<Imath::V3i> intersects = rayIntersection(m_ray);
    if (intersects.size() == 0)
        return voxels;

    // Get the first voxel hit
    for (size_t i = 0; i < intersects.size(); i++)
    {
        if (p_gvg->get(intersects[i]).a != 0.0f)
        {
            voxels.push_back(intersects[i]);
            break;
        }
    }

    return voxels;
}


////////////////////////////////////////
void ReplaceToolState::execute()
{
    p_undoManager->beginMacro("Replace");
    std::vector<Imath::V3i> voxels = voxelsAffected();
    for (size_t i = 0; i < voxels.size(); i++)
    {
        // Don't replace if you're already identical
        if (p_gvg->get(voxels[i]) != m_color)
            p_undoManager->setVoxelColor(p_gvg, voxels[i], m_color, m_index);
    }
    p_undoManager->endMacro();
    decrementClicks();
}


std::vector<Imath::V3i> ReplaceToolState::voxelsAffected()
{
    std::vector<Imath::V3i> voxels;

    // Intersect and check
    std::vector<Imath::V3i> intersects = rayIntersection(m_ray);
    if (intersects.size() == 0)
        return voxels;

    // Get the first voxel hit
    for (size_t i = 0; i < intersects.size(); i++)
    {
        if (p_gvg->get(intersects[i]).a != 0.0f)
        {
            voxels.push_back(intersects[i]);
            break;
        }
    }

    return voxels;
}


////////////////////////////////////////
void RayToolState::execute()
{
    std::vector<Imath::V3i> voxels = voxelsAffected();

    p_undoManager->beginMacro("Ray Blast");
    for (size_t i = 0; i < voxels.size(); i++)
    {
        p_undoManager->setVoxelColor(p_gvg, voxels[i], m_color, m_index);
    }
    p_undoManager->endMacro();
    decrementClicks();
}


std::vector<Imath::V3i> RayToolState::voxelsAffected()
{
    return rayIntersection(m_ray);
}


////////////////////////////////////////
void SlabToolState::execute()
{
    std::vector<Imath::V3i> voxels = voxelsAffected();

    switch (m_workingAxis)
    {
        case X_AXIS: p_undoManager->beginMacro("Fill X Slice"); break;
        case Y_AXIS: p_undoManager->beginMacro("Fill Y Slice"); break;
        case Z_AXIS: p_undoManager->beginMacro("Fill Z Slice"); break;
    }
    for (size_t i = 0; i < voxels.size(); i++)
    {
        p_undoManager->setVoxelColor(p_gvg, voxels[i], m_color, m_index);
    }
    p_undoManager->endMacro();
    decrementClicks();
}


std::vector<Imath::V3i> SlabToolState::voxelsAffected()
{
    std::vector<Imath::V3i> voxels;

    // TODO: Should this do an intersection at all, or maybe just fill in the
    //       row based on the first hit?
    std::vector<Imath::V3i> intersects = rayIntersection(m_ray);
    if (intersects.size() == 0)
        return voxels;

    // Get the position to fill from
    Imath::V3i fillPos=intersects[0];

    for (size_t i = 0; i < intersects.size(); i++)
    {
        if (p_gvg->get(intersects[i]).a != 0.0f)
        {
            // Hit a voxel at the near edge of the grid?  Start there.
            if (i == 0)
            {
                fillPos = intersects[0];
                break;
            }
            else
            {
                // Hit a voxel in the middle?
                fillPos = intersects[i-1];
                break;
            }
        }
    }

    // Fill out the slab
    Imath::Box3i dim=p_gvg->bounds();

    switch (m_workingAxis)
    {
        case X_AXIS:
            for (int y=dim.min.y; y <= dim.max.y; y++)
            {
                for (int z=dim.min.z; z <= dim.max.z; z++)
                {
                    voxels.push_back(Imath::V3i(fillPos.x, y, z));
                }
            }
            break;
        case Y_AXIS:
            for (int x=dim.min.x; x <= dim.max.x; x++)
            {
                for (int z=dim.min.z; z <= dim.max.z; z++)
                {
                    voxels.push_back(Imath::V3i(x, fillPos.y, z));
                }
            }
            break;
        case Z_AXIS:
            for (int x=dim.min.x; x <= dim.max.x; x++)
            {
                for (int y=dim.min.y; y <= dim.max.y; y++)
                {
                    voxels.push_back(Imath::V3i(x, y, fillPos.z));
                }
            }
            break;
    }
    return voxels;
}


////////////////////////////////////////
void LineToolState::execute()
{
    std::vector<Imath::V3i> voxels = voxelsAffected();

    // First click sets the start point
    if (m_clicksRemain == 2)
    {
        if (voxels.size())
        {
            m_startPoint = voxels[0];
            decrementClicks();
        }
    }

    // Second click fills in the line
    else if (m_clicksRemain == 1)
    {
        p_undoManager->beginMacro("Line");
        for (size_t i = 0; i < voxels.size(); i++)
        {
            p_undoManager->setVoxelColor(p_gvg, voxels[i], m_color, m_index);
        }
        p_undoManager->endMacro();
        decrementClicks();
    }
}


std::vector<Imath::V3i> LineToolState::voxelsAffected()
{
    std::vector<Imath::V3i> voxels;

    // Intersect and check
    std::vector<Imath::V3i> intersects = rayIntersection(m_ray);
    if (intersects.size() == 0)
        return voxels;

    Imath::V3i intersect;
    for (size_t i = 0; i < intersects.size(); i++)
    {
        if (p_gvg->get(intersects[i]).a != 0.0f)
        {
            // Hit a voxel at the close edge of the grid?  Abort.
            if (i == 0)
                return voxels;

            // Hit a voxel in the middle?  Return previous voxel.
            intersect = intersects[i-1];
            break;
        }

        // Didn't hit anything?  Just return the last voxel.
        if (i == intersects.size()-1)
        {
            intersect = intersects[i];
            break;
        }
    }

    if (m_clicksRemain == 2)
    {
        voxels.push_back(intersect);
    }
    else if (m_clicksRemain == 1)
    {
        Imath::Line3d ray;
        ray.pos = p_gvg->voxelTransform(m_startPoint).translation();
        ray.dir = (p_gvg->voxelTransform(intersect).translation() - ray.pos).normalized();

        if (ray.pos == intersect)
        {
            // An infinitely small ray is bad
            voxels.push_back(intersect);
        }
        else
        {
            // Trim off the end, making it a ray segment instead of an entire ray
            std::vector<Imath::V3i> initialInts = rayIntersection(ray);
            for (size_t i = 0; i < initialInts.size(); i++)
            {
                voxels.push_back(initialInts[i]);
                if (initialInts[i] == intersect)
                    break;
            }
        }
    }

    return voxels;
}


////////////////////////////////////////
void BoxToolState::execute()
{
    std::vector<Imath::V3i> voxels = voxelsAffected();

    // First click sets the start point
    if (m_clicksRemain == 2)
    {
        if (voxels.size())
        {
            m_startPoint = voxels[0];
            decrementClicks();
        }
    }

    // Second click fills in the box
    else if (m_clicksRemain == 1)
    {
        p_undoManager->beginMacro("Box");
        for (size_t i = 0; i < voxels.size(); i++)
        {
            p_undoManager->setVoxelColor(p_gvg, voxels[i], m_color, m_index);
        }
        p_undoManager->endMacro();
        decrementClicks();
    }
}


std::vector<Imath::V3i> BoxToolState::voxelsAffected()
{
    std::vector<Imath::V3i> voxels;

    // Intersect and check
    std::vector<Imath::V3i> intersects = rayIntersection(m_ray);
    if (intersects.size() == 0)
        return voxels;

    Imath::V3i intersect;
    for (size_t i = 0; i < intersects.size(); i++)
    {
        if (p_gvg->get(intersects[i]).a != 0.0f)
        {
            // Hit a voxel at the close edge of the grid?  Abort.
            if (i == 0)
                return voxels;

            // Hit a voxel in the middle?  Return previous voxel.
            intersect = intersects[i-1];
            break;
        }

        // Didn't hit anything?  Just return the last voxel.
        if (i == intersects.size()-1)
        {
            intersect = intersects[i];
            break;
        }
    }

    if (m_clicksRemain == 2)
    {
        voxels.push_back(intersect);
    }
    else if (m_clicksRemain == 1)
    {
      Imath::Box3i box(m_startPoint);
      box.extendBy(intersect);

      for (int z=box.min.z; z<=box.max.z; ++z)
        for (int y=box.min.y; y<=box.max.y; ++y)
          for (int x=box.min.x; x<=box.max.x; ++x)
            voxels.push_back(Imath::V3i(x, y, z));
    }

    return voxels;
}


////////////////////////////////////////
void ExtrudeToolState::execute()
{
  std::vector<Imath::V3i> voxels = voxelsAffected();

  p_undoManager->beginMacro("Extrude");

  for (size_t i = 0; i < voxels.size(); i++)
  {
    Imath::V3i sp=voxels[i]-m_dir;
    SproxelColor color=p_gvg->get(sp);
    int index=p_gvg->getInd(sp);
    p_undoManager->setVoxelColor(p_gvg, voxels[i], color, index);
  }

  p_undoManager->endMacro();

  decrementClicks();
}


void ExtrudeToolState::fillExtrudeMap(GameVoxelGrid<char> &map, const Imath::V3i &mapOfs,
  const Imath::V3i &pos, int axis)
{
  Imath::V3i mp=pos+mapOfs;
  if (mp.x<0 || mp.y<0 || mp.z<0) return;

  const Imath::V3i &size=map.cellDimensions();
  if (mp.x>=size.x || mp.y>=size.y || mp.z>=size.z) return;

  if (map.get(mp)!=0) return; // already marked

  if (p_gvg->get(pos).a!=0 || p_gvg->get(pos-m_dir).a==0)
  {
    // out of extruded area, mark and stop
    map.set(mp, 2);
    return;
  }

  int paxis=1, saxis=2;
  if (axis==1) paxis=0;
  else if (axis==2) saxis=0;

  // mark area
  map.set(mp, 1);

  //== TODO: use linear fill instead, for less recursion depth
  Imath::V3i newPos=pos;
  newPos[paxis]--;
  fillExtrudeMap(map, mapOfs, newPos, axis);
  newPos[paxis]+=2;
  fillExtrudeMap(map, mapOfs, newPos, axis);
  newPos[paxis]--;
  newPos[saxis]--;
  fillExtrudeMap(map, mapOfs, newPos, axis);
  newPos[saxis]+=2;
  fillExtrudeMap(map, mapOfs, newPos, axis);
}


std::vector<Imath::V3i> ExtrudeToolState::voxelsAffected()
{
  std::vector<Imath::V3i> voxels;

  std::vector<Imath::V3i> intersects = rayIntersection(m_ray);
  if (intersects.size() == 0) return voxels;

  // Get the position to fill from
  Imath::V3i fillPos=intersects[0];

  for (size_t i = 0; i < intersects.size(); i++)
  {
    if (p_gvg->get(intersects[i]).a != 0.0f)
    {
      if (i==0) return voxels; // can't extrude beyond bounds

      fillPos=intersects[i-1];
      m_dir=fillPos-intersects[i];
      break;
    }
  }

  // determine axis
  int axis=0;
  if (m_dir.y!=0) axis=1;
  else if (m_dir.z!=0) axis=2;

  // allocate map
  Imath::Box3i bounds=p_gvg->bounds();
  Imath::V3i mapSize=bounds.size()+Imath::V3i(1);
  mapSize[axis]=1;
  Imath::V3i mapOfs=-bounds.min;
  mapOfs[axis]=-fillPos[axis];

  GameVoxelGrid<char> map(mapSize);
  map.setAll(0);

  fillExtrudeMap(map, mapOfs, fillPos, axis);

  // get voxels from map
  for (int z=0; z<mapSize.z; ++z)
    for (int y=0; y<mapSize.y; ++y)
      for (int x=0; x<mapSize.x; ++x)
      {
        if (map.get(Imath::V3i(x, y, z))!=1) continue;
        voxels.push_back(Imath::V3i(x, y, z)-mapOfs);
      }

  return voxels;
}


////////////////////////////////////////
void DropperToolState::execute()
{
    decrementClicks();
}


std::vector<Imath::V3i> DropperToolState::voxelsAffected()
{
    std::vector<Imath::V3i> voxels;

    // Intersect and check
    std::vector<Imath::V3i> intersects = rayIntersection(m_ray);
    if (intersects.size() == 0)
        return voxels;

    // Get the first voxel hit
    for (size_t i = 0; i < intersects.size(); i++)
    {
        if (p_gvg->get(intersects[i]).a != 0.0f)
        {
            voxels.push_back(intersects[i]);
            break;
        }
    }

    return voxels;
}


