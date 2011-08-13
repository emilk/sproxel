#include "Tools.h"
#include "GLModelWidget.h"

////////////////////////////////////////
void SplatToolState::execute()
{
    std::vector<Imath::V3i> voxels = voxelsAffected();
    for (size_t i = 0; i < voxels.size(); i++)
    {
        p_undoManager->setVoxelColor(*p_gvg, voxels[i], m_color); 
    }

    m_totalClicks--;
}


std::vector<Imath::V3i> SplatToolState::voxelsAffected()
{
    std::vector<Imath::V3i> voxels;
    
    // Intersect and check
    std::vector<Imath::V3i> intersects = 
        p_gvg->rayIntersection(m_ray, true);
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
    p_undoManager->setVoxelColor(*p_gvg, hit, m_color);
    setNeighborsRecurse(hit, repColor, m_color);
    p_undoManager->endMacro();
    
    m_totalClicks--;
}


void FloodToolState::setNeighborsRecurse(const Imath::V3i& alreadySet, 
                                         const Imath::Color4f& repColor, 
                                         const Imath::Color4f& newColor)
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
        if (doUs[i].x < 0 || doUs[i].x >= p_gvg->cellDimensions().x) continue;
        if (doUs[i].y < 0 || doUs[i].y >= p_gvg->cellDimensions().y) continue;
        if (doUs[i].z < 0 || doUs[i].z >= p_gvg->cellDimensions().z) continue;
        
        // Recurse
        if (p_gvg->get(doUs[i]) == repColor)
        {
            p_undoManager->setVoxelColor(*p_gvg, doUs[i], newColor);
            setNeighborsRecurse(doUs[i], repColor, newColor);
        }
    }
}


std::vector<Imath::V3i> FloodToolState::voxelsAffected()
{
    // TODO: It may make the most sense to recurse in here, but it could be slow
    std::vector<Imath::V3i> voxels;

    // Intersect and check
    std::vector<Imath::V3i> intersects = 
        p_gvg->rayIntersection(m_ray, true);
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
    std::vector<Imath::V3i> voxels = voxelsAffected();
    for (size_t i = 0; i < voxels.size(); i++)
    {
        p_undoManager->setVoxelColor(*p_gvg, voxels[i], 
                                     Imath::Color4f(0.0f, 0.0f, 0.0f, 0.0f)); 
    }

    m_totalClicks--;
}


std::vector<Imath::V3i> EraserToolState::voxelsAffected()
{
    std::vector<Imath::V3i> voxels;

    // Intersect and check
    std::vector<Imath::V3i> intersects = 
        p_gvg->rayIntersection(m_ray, true);
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
    std::vector<Imath::V3i> voxels = voxelsAffected();
    for (size_t i = 0; i < voxels.size(); i++)
    {
        p_undoManager->setVoxelColor(*p_gvg, voxels[i], m_color); 
    }
    
    m_totalClicks--;
}


std::vector<Imath::V3i> ReplaceToolState::voxelsAffected()
{
    std::vector<Imath::V3i> voxels;

    // Intersect and check
    std::vector<Imath::V3i> intersects = 
        p_gvg->rayIntersection(m_ray, true);
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
        p_undoManager->setVoxelColor(*p_gvg, voxels[i], m_color); 
    }
    p_undoManager->endMacro();
    
    m_totalClicks--;
}


std::vector<Imath::V3i> SlabToolState::voxelsAffected()
{
    std::vector<Imath::V3i> voxels;

    // TODO: Should this do an intersection at all, or maybe just fill in the 
    //       row based on the first hit?
    std::vector<Imath::V3i> intersects = 
        p_gvg->rayIntersection(m_ray, true);
    if (intersects.size() == 0)
        return voxels;

    // Get the position to fill from
    Imath::V3i fillPos(-1,-1,-1);
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

    // Didn't hit anything?  Just fill in the first voxel.
    if (fillPos == Imath::V3i(-1,-1,-1))
    {
        fillPos = intersects[0];
    }

    // Fill out the slab
    switch (m_workingAxis)
    {
        case X_AXIS:
            for (int y=0; y < p_gvg->cellDimensions().y; y++)
            {
                for (int z=0; z < p_gvg->cellDimensions().z; z++)
                {
                    voxels.push_back(Imath::V3i(fillPos.x, y, z));
                }
            }
            break;
        case Y_AXIS:
            for (int x=0; x < p_gvg->cellDimensions().x; x++)
            {
                for (int z=0; z < p_gvg->cellDimensions().z; z++)
                {
                    voxels.push_back(Imath::V3i(x, fillPos.y, z));
                }
            }
            break;
        case Z_AXIS:
            for (int x=0; x < p_gvg->cellDimensions().x; x++)
            {
                for (int y=0; y < p_gvg->cellDimensions().y; y++)
                {
                    voxels.push_back(Imath::V3i(x, y, fillPos.z));
                }
            }
            break;
    }
    return voxels;
}
