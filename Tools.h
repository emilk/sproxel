#ifndef __TOOLS_H__
#define __TOOLS_H__

#include "Global.h"
#include "UndoManager.h"

#include <ImathVec.h>
#include <ImathColor.h>

////////////////////////////////////////////////////////////////////////////////
// Abstract base class for Sproxel tools and their execution states
////////////////////////////////////////////////////////////////////////////////
class ToolState
{
public:
    ToolState(UndoManager* um,
              const Imath::Line3d& ray, 
              const Imath::Color4f& color, 
              SproxelGrid* gvg) : m_totalClicks(0), 
                                  p_undoManager(um), 
                                  m_ray(ray), 
                                  m_color(color), 
                                  p_gvg(gvg) {}
    virtual ~ToolState() {}
    
    virtual void execute() = 0;
    virtual SproxelTool type() = 0;
    virtual std::vector<Imath::V3i> voxelsAffected() = 0;
    virtual bool supportsDrag() { return false; }
    
    int clicksRemaining()
    {
        return m_totalClicks;
    }
    
    virtual ToolState& operator=(const ToolState& right)
    {
        m_totalClicks = right.m_totalClicks;
        p_undoManager = right.p_undoManager;
        m_color = right.m_color;
        p_gvg = right.p_gvg;
        return *this;
    }
    
protected:
    int m_totalClicks;
        
    UndoManager* p_undoManager;
    Imath::Line3d m_ray;
    Imath::Color4f m_color;
    SproxelGrid* p_gvg;
};


////////////////////////////////////////////////////////////////////////////////
// Splat tool
////////////////////////////////////////////////////////////////////////////////
class SplatToolState : public ToolState
{
public:
    SplatToolState(UndoManager* um,
                   const Imath::Line3d& ray, 
                   const Imath::Color4f& color, 
                   SproxelGrid* gvg) : ToolState(um, ray, color, gvg)
    {
        m_totalClicks = 1;
    }
    
    ~SplatToolState() {}
    
    void execute();
    SproxelTool type() { return TOOL_SPLAT; }
    std::vector<Imath::V3i> voxelsAffected();
};


////////////////////////////////////////////////////////////////////////////////
// Flood tool
////////////////////////////////////////////////////////////////////////////////
class FloodToolState : public ToolState
{
public:
    FloodToolState(UndoManager* um,
                   const Imath::Line3d& ray, 
                   const Imath::Color4f& color, 
                   SproxelGrid* gvg) : ToolState(um, ray, color, gvg)
    {
        m_totalClicks = 1;
    }
    
    ~FloodToolState() {}
    
    void execute();
    SproxelTool type() { return TOOL_FLOOD; }
    std::vector<Imath::V3i> voxelsAffected();

private:
    void setNeighborsRecurse(const Imath::V3i& alreadySet,
                             const Imath::Color4f& repColor,
                             const Imath::Color4f& newColor);
};


////////////////////////////////////////////////////////////////////////////////
// Eraser tool
////////////////////////////////////////////////////////////////////////////////
class EraserToolState : public ToolState
{
public:
    EraserToolState(UndoManager* um,
                    const Imath::Line3d& ray, 
                    const Imath::Color4f& color, 
                    SproxelGrid* gvg) : ToolState(um, ray, color, gvg)
    {
        m_totalClicks = 1;
    }
    
    ~EraserToolState() {}
    
    void execute();
    SproxelTool type() { return TOOL_ERASER; }
    std::vector<Imath::V3i> voxelsAffected();
};


////////////////////////////////////////////////////////////////////////////////
// Replace tool
////////////////////////////////////////////////////////////////////////////////
class ReplaceToolState : public ToolState
{
public:
    ReplaceToolState(UndoManager* um,
                     const Imath::Line3d& ray, 
                     const Imath::Color4f& color, 
                     SproxelGrid* gvg) : ToolState(um, ray, color, gvg)
    {
        m_totalClicks = 1;
    }
    
    ~ReplaceToolState() {}
    
    void execute();
    SproxelTool type() { return TOOL_REPLACE; }
    std::vector<Imath::V3i> voxelsAffected();
};


////////////////////////////////////////////////////////////////////////////////
// Slab tool
////////////////////////////////////////////////////////////////////////////////
class SlabToolState : public ToolState
{
public:
    SlabToolState(UndoManager* um,
                  const Imath::Line3d& ray, 
                  const Imath::Color4f& color, 
                  SproxelGrid* gvg,
                  const int axis) : ToolState(um, ray, color, gvg)
    {
        m_workingAxis = axis;
        m_totalClicks = 1;
    }
    
    ~SlabToolState() {}
    
    void execute();
    SproxelTool type() { return TOOL_SLAB; }
    std::vector<Imath::V3i> voxelsAffected();

    virtual SlabToolState& operator=(const SlabToolState& right)
    {
        m_totalClicks = right.m_totalClicks;
        p_undoManager = right.p_undoManager;
        m_color = right.m_color;
        p_gvg = right.p_gvg;
        m_workingAxis = right.m_workingAxis;
        return *this;
    }

private:
    int m_workingAxis;      // Why can't i do : GLModelWidget::Axis?
};


////////////////////////////////////////////////////////////////////////////////
// Dropper tool
////////////////////////////////////////////////////////////////////////////////
// class DropperToolState : public ToolState
// {
// public:
//     DropperToolState(UndoManager* um,
//                      const Imath::Line3d& ray, 
//                      const Imath::Color4f& color, 
//                      SproxelGrid* gvg) : ToolState(um, ray, color, gvg)
//     {
//         m_totalClicks = 1;
//     }
//     
//     ~DropperToolState() {}
//     
//     void execute();
//     virtual std::vector<Imath::V3i> voxelsAffected();
// };


#endif
