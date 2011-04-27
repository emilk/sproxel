#ifndef __GL_MODEL_WIDGET_H__
#define __GL_MODEL_WIDGET_H__

#include <vector>

#include <QGLWidget>

#include "GLCamera.h"
#include "GameVoxelGrid.h"

#include <ImathBox.h>
#include <ImathVec.h>
#include <ImathColor.h>


class GLModelWidget : public QGLWidget
{
    Q_OBJECT

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

    void setVoxelColor(const Imath::V3i& index, const Imath::Color4f color);
    void setActiveColor(const Imath::Color4f& c) { m_activeColor = c; }

    // Accessors
    const Imath::V3i& activeVoxel() const { return m_activeVoxel; }
    const Imath::Color4f& activeColor() const { return m_activeColor; }
    bool drawGrid() const { return m_drawGrid; }
    bool drawVoxelGrid() const { return m_drawVoxelGrid; }
    bool drawBoundingBox() const { return m_drawBoundingBox; }
    int currentAxis() const { return m_currAxis; }


public slots:
    void setDrawGrid(const bool value) { m_drawGrid = value; updateGL(); }
    void setDrawVoxelGrid(const bool value) { m_drawVoxelGrid = value; updateGL(); }
    void setDrawBoundingBox(const bool value) { m_drawBoundingBox = value; updateGL(); }
    void setCurrentAxis( int val ) { m_currAxis = val; updateGL(); }
    
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

    int m_currAxis;

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
};

#endif
