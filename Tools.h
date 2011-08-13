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
    ToolState(UndoManager* um) : m_clicksRemain(0),
                                 p_undoManager(um), 
                                 m_ray(Imath::Line3d()), 
                                 m_color(Imath::Color4f(0.0f, 0.0f, 0.0f, 0.0f)), 
                                 p_gvg(NULL),
                                 m_supportsDrag(false) {}

    virtual ~ToolState() {}
    
    virtual void execute() = 0;
    virtual SproxelTool type() = 0;
    virtual std::vector<Imath::V3i> voxelsAffected() = 0;
    
    virtual void set(SproxelGrid* gvg,
                     const Imath::Line3d& ray, 
                     const Imath::Color4f& color)
    {
        m_ray = ray;
        m_color = color;
        p_gvg = gvg;
    }
    
    virtual void decrementClicks() { m_clicksRemain--; }
    virtual int clicksRemaining() { return m_clicksRemain; }

    virtual void setDragSupport(bool support) { m_supportsDrag = support; }
    virtual bool supportsDrag() { return m_supportsDrag; }

protected:
    int m_clicksRemain;
        
    UndoManager* p_undoManager;
    Imath::Line3d m_ray;
    Imath::Color4f m_color;
    SproxelGrid* p_gvg;
    bool m_supportsDrag;
};


////////////////////////////////////////////////////////////////////////////////
// Splat tool
////////////////////////////////////////////////////////////////////////////////
class SplatToolState : public ToolState
{
public:
    SplatToolState(UndoManager* um) : ToolState(um)
    {
        m_clicksRemain = 1;
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
    FloodToolState(UndoManager* um) : ToolState(um)
    {
        m_clicksRemain = 1;
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
    EraserToolState(UndoManager* um) : ToolState(um)
    {
        m_clicksRemain = 1;
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
    ReplaceToolState(UndoManager* um) : ToolState(um)
    {
        m_clicksRemain = 1;
    }
    
    ~ReplaceToolState() {}
    
    void execute();
    SproxelTool type() { return TOOL_REPLACE; }
    std::vector<Imath::V3i> voxelsAffected();
};


////////////////////////////////////////////////////////////////////////////////
// Ray tool
////////////////////////////////////////////////////////////////////////////////
class RayToolState : public ToolState
{
public:
    RayToolState(UndoManager* um) : ToolState(um)
    {
        m_clicksRemain = 1;
    }
    ~RayToolState() {}
    
    void execute();
    SproxelTool type() { return TOOL_RAY; }
    std::vector<Imath::V3i> voxelsAffected();
};


////////////////////////////////////////////////////////////////////////////////
// Slab tool
////////////////////////////////////////////////////////////////////////////////
class SlabToolState : public ToolState
{
public:
    SlabToolState(UndoManager* um) : ToolState(um)
    {
        m_workingAxis = Y_AXIS;
        m_clicksRemain = 1;
    }
    
    ~SlabToolState() {}

    virtual void set(SproxelGrid* gvg,
                     const Imath::Line3d& ray, 
                     const Imath::Color4f& color, 
                     SproxelAxis axis)
    {
        m_ray = ray;
        m_color = color;
        p_gvg = gvg;
        m_workingAxis = axis;
    }
    
    void execute();
    SproxelTool type() { return TOOL_SLAB; }
    std::vector<Imath::V3i> voxelsAffected();

private:
    SproxelAxis m_workingAxis;
};


////////////////////////////////////////////////////////////////////////////////
// Dropper tool
////////////////////////////////////////////////////////////////////////////////
class DropperToolState : public ToolState
{
public:
    DropperToolState(UndoManager* um) : ToolState(um)
    {
        m_clicksRemain = 1;
    }
    
    ~DropperToolState() {}
    
    void execute();
    SproxelTool type() { return TOOL_DROPPER; }
    std::vector<Imath::V3i> voxelsAffected();
};


#endif
