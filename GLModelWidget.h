#ifndef __GL_MODEL_WIDGET_H__
#define __GL_MODEL_WIDGET_H__

#include <vector>

#include <QGLWidget>
#include <QUndoStack>
#include <QUndoCommand>

#include "GLCamera.h"
#include "GameVoxelGrid.h"

#include <ImathBox.h>
#include <ImathVec.h>
#include <ImathColor.h>


class GLModelWidget : public QGLWidget
{
    Q_OBJECT

public:
    enum Axis { X_AXIS, Y_AXIS, Z_AXIS };

public:
    GLModelWidget(QWidget *parent = 0);
    ~GLModelWidget();

    QSize minimumSizeHint() const;
    QSize sizeHint() const;


public:
    void frame();
    void handleArrows(QKeyEvent *event);

    bool loadGridCSV(const std::string& filename);
    bool saveGridCSV(const std::string& filename);

    void resizeVoxelGrid(Imath::V3i size);

    void setVoxelColor(const Imath::V3i& index, const Imath::Color4f color);

    // Accessors
    const Imath::V3i& activeVoxel() const { return m_activeVoxel; }
    const Imath::Color4f& activeColor() const { return m_activeColor; }
    bool drawGrid() const { return m_drawGrid; }
    bool drawVoxelGrid() const { return m_drawVoxelGrid; }
    bool drawBoundingBox() const { return m_drawBoundingBox; }
    Axis currentAxis() const { return m_currAxis; }


signals:
    void colorSampled(const Imath::Color4f& color);
        
public slots:
    void setDrawGrid(const bool value) { m_drawGrid = value; updateGL(); }
    void setDrawVoxelGrid(const bool value) { m_drawVoxelGrid = value; updateGL(); }
    void setDrawBoundingBox(const bool value) { m_drawBoundingBox = value; updateGL(); }
    void setCurrentAxis(const Axis val) { m_currAxis = val; updateGL(); }
    void setActiveColor(const Imath::Color4f& c) { m_activeColor = c; }
    
    void undo() { m_undoStack.undo(); updateGL(); }
    void redo() { m_undoStack.redo(); updateGL(); }
    
protected:
    void initializeGL();
    void paintGL();
    void resizeGL(int width, int height);
    
    virtual void mousePressEvent(QMouseEvent* event);
    virtual void mouseMoveEvent(QMouseEvent* event);


private:
    GLCamera m_cam;

    GameVoxelGrid<Imath::Color4f> m_gvg;
    std::vector<Imath::V3i> m_intersects;

    Imath::V3i m_activeVoxel;
    Imath::Color4f m_activeColor;

    bool m_altDown;
    bool m_ctrlDown;
    QPoint m_lastMouse;
    bool m_drawGrid;
    bool m_drawVoxelGrid;
    bool m_drawBoundingBox;

    Axis m_currAxis;

    double* glMatrix(const Imath::M44d& m);

    void rayGunBlast(const std::vector<Imath::V3i>& sortedInput, const Imath::Color4f& color);
    void paintGunBlast(const std::vector<Imath::V3i>& sortedInput, const Imath::Color4f& color);
    void paintGunReplace(const std::vector<Imath::V3i>& sortedInput, const Imath::Color4f& color);
    void paintGunFlood(const std::vector<Imath::V3i>& sortedInput, const Imath::Color4f& color);
    Imath::Color4f colorPick(const std::vector<Imath::V3i>& sortedInput);
    void paintGunDelete(const std::vector<Imath::V3i>& sortedInput);
    void paintGunFillSlice(const std::vector<Imath::V3i>& sortedInput, const Imath::Color4f& color);

    // Paint Gun Flood helper
    void setNeighborsRecurse(const Imath::V3i& alreadySet, 
                             const Imath::Color4f& repColor, 
                             const Imath::Color4f& newColor);

    Imath::Box3d dataBounds();

    void glDrawAxes();
    void glDrawGrid(const int size);

    void glDrawCubeWire();
    void glDrawCubePoly();

    void glDrawVoxelGrid();
    void glDrawActiveVoxel();
    void glDrawSelectedVoxels();
    void glDrawVoxelCenter(const size_t sx, const size_t sy, const size_t sz);
    
    QUndoStack m_undoStack;
};


// The only real command this GUI does - any more should be broken out into their own file.
class CmdSetVoxelColor : public QUndoCommand
{
public:
    CmdSetVoxelColor(GameVoxelGrid<Imath::Color4f>* gvg, const Imath::V3i& index, const Imath::Color4f color) : 
        m_pGvg(gvg),
        m_index(),
        m_newColor(),
        m_oldColor()
    {
        m_index.push_back(index);
        m_newColor.push_back(color);
        m_oldColor.push_back(gvg->get(index));
        setText("Set voxel");
    }
    
    virtual void redo()
    {
        for (size_t i = 0; i < m_index.size(); i++)
            m_pGvg->set(m_index[i], m_newColor[i]);
    }
    
    virtual void undo()
    {
        for (size_t i = 0; i < m_index.size(); i++)
            m_pGvg->set(m_index[i], m_oldColor[i]);
    }
    
protected:
    virtual bool mergeMeWith(QUndoCommand* other)
    {
        if (other->id() != id())
            return false;
        
        const CmdSetVoxelColor* otherSet = static_cast<const CmdSetVoxelColor*>(other);
        m_index.push_back(otherSet->m_index[0]);
        m_newColor.push_back(otherSet->m_newColor[0]);
        m_oldColor.push_back(otherSet->m_oldColor[0]);
        return true;
    }

private:
    GameVoxelGrid<Imath::Color4f>* m_pGvg;
    std::vector<Imath::V3i> m_index;
    std::vector<Imath::Color4f> m_newColor;
    std::vector<Imath::Color4f> m_oldColor;
};

#endif
