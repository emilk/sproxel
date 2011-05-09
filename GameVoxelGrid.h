#ifndef __GAME_VOXEL_GRID_H__
#define __GAME_VOXEL_GRID_H__

#include <vector>

#include <ImathBox.h>
#include <ImathBoxAlgo.h>
#include <ImathVec.h>
#include <ImathLine.h>
#include <ImathMatrix.h>


//-*****************************************************************************
// Sort util
struct SortElement
{
    Imath::V3i index;
    float distanceToCamera;
};

static bool sortFunction(const SortElement& a, const SortElement& b)
{
    return a.distanceToCamera < b.distanceToCamera;
}


//-*****************************************************************************
template <class T>
class GameVoxelGrid
{
public:
    GameVoxelGrid(const Imath::V3i& cellDim) 
        : m_transform(),
          m_cellDimensions(cellDim)
    {
        resizeData();
    }
    ~GameVoxelGrid() { }

    // General accessors
    const Imath::V3i& cellDimensions() const { return m_cellDimensions; }
    void setCellDimensions(const Imath::V3i& cd) { m_cellDimensions = cd; resizeData(); }

    const Imath::M44d& transform() const { return m_transform; }
    void setTransform(const Imath::M44d& m) { m_transform = m; }

    const Imath::M44d voxelTransform(const Imath::V3i& v) const
    {
        Imath::M44d vMat;
        vMat.setTranslation(voxelCenter(v));
        return vMat * m_transform;
    }

    const Imath::Box3d worldBounds() const
    {
        const Imath::Box3d retBox(Imath::V3d(0,0,0),
                                  Imath::V3d(m_cellDimensions.x,
                                             m_cellDimensions.y,
                                             m_cellDimensions.z));
        // (ImathBoxAlgo) This properly computes the world bounding box
        return Imath::transform(retBox, m_transform);
    }

    void set(const Imath::V3i& cell, const T& value)
    {
        m_data[cell.x][cell.y][cell.z] = value;
    }

    void setAll(const T& value)
    {
        for (int x = 0; x < m_cellDimensions.x; x++)
            for (int y = 0; y < m_cellDimensions.y; y++)
                for (int z = 0; z < m_cellDimensions.z; z++)
                    m_data[x][y][z] = value;
    }

    T get(const Imath::V3i& cell)
    {
        return m_data[cell.x][cell.y][cell.z];
    }

    std::vector<Imath::V3i> rayIntersection(Imath::Line3d worldRay, bool sort)
    {
        // Transform the ray into voxel space
        Imath::Line3d localRay = worldRay * m_transform.inverse();

        std::vector<SortElement> sortList;
        
        // Ray-boxes intersection
        // TODO: Acceleration structure
        for (int x = 0; x < m_cellDimensions.x; x++)
        {
            for (int y = 0; y < m_cellDimensions.y; y++)
            {
                for (int z = 0; z < m_cellDimensions.z; z++)
                {
                    Imath::V3d iPoint;
                    Imath::Box3d vBox(Imath::V3d(x,y,z), Imath::V3d(x+1,y+1,z+1));
                    bool intersects = Imath::intersects(vBox, localRay, iPoint);

                    if (intersects)
                    {
                        SortElement sm;
                        sm.index = Imath::V3i(x,y,z);
                        sm.distanceToCamera = (iPoint - worldRay.pos).length2();
                        sortList.push_back(sm);
                    }
                }
            }
        }
        
        // Sort the list
        if (sort)
            std::sort(sortList.begin(), sortList.end(), sortFunction);

        // Return only what's needed
        std::vector<Imath::V3i> orderedCells;
        orderedCells.resize(sortList.size());
        for (size_t i = 0; i < sortList.size(); i++)
            orderedCells[i] = sortList[i].index;
        return orderedCells;
    }

    GameVoxelGrid& operator=(const GameVoxelGrid& other)
    {
        if (this != &other)
        {
            m_cellDimensions = other.cellDimensions();
            resizeData();

            for (int x = 0; x < m_cellDimensions.x; x++)
            {
                for (int y = 0; y < m_cellDimensions.y; y++)
                {
                    for (int z = 0; z < m_cellDimensions.z; z++)
                    {
                        m_data[x][y][z] = other.m_data[x][y][z];
                    }
                }
            }
        }
        return *this;
    }


private:
    Imath::M44d m_transform;
    Imath::V3i m_cellDimensions;

    Imath::V3d voxelCenter(const Imath::V3i& v) const
    {
        return Imath::V3d((double)v.x + 0.5,
                          (double)v.y + 0.5,
                          (double)v.z + 0.5);
    }

    void resizeData()
    {
        // Does not lose existing data
        m_data.resize(m_cellDimensions.x);
        for (int x = 0; x < m_cellDimensions.x; x++)
        {
            m_data[x].resize(m_cellDimensions.y);
            for (int y = 0; y < m_cellDimensions.y; y++)
            {
                m_data[x][y].resize(m_cellDimensions.z);
            }
        }
    }
    
    std::vector< std::vector < std::vector< T > > > m_data;
};

#endif
