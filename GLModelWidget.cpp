#include <QtGui>
#include <QtOpenGL>

#include <map>
#include <cmath>
#include <iostream>
#include <algorithm>

#include <ImathLine.h>
#include <ImathBoxAlgo.h>

#include "GLModelWidget.h"

#define DEBUG_ME (0)
#define DEFAULT_VOXGRID_SZ (8)

Imath::Box3d fakeBounds(Imath::V3d(-50, -50, -50), Imath::V3d(50, 50, 50));

GLModelWidget::GLModelWidget(QWidget* parent, const QSettings* appSettings)
    : QGLWidget(parent),
      m_cam(),
      m_undoManager(),
      m_gvg(Imath::V3i(DEFAULT_VOXGRID_SZ, DEFAULT_VOXGRID_SZ, DEFAULT_VOXGRID_SZ)),
      m_previews(),
      m_activeVoxel(-1,-1,-1),
      m_activeColor(1.0f, 1.0f, 1.0f, 1.0f),
      m_lastMouse(),
      m_drawGrid(true),
      m_drawVoxelGrid(true),
      m_drawBoundingBox(false),
      m_shiftWrap(true),
      m_currAxis(Y_AXIS),
      m_activeTool(NULL),
      p_appSettings(appSettings)
{
    // Default empty grid
    m_gvg.setAll(Imath::Color4f(0.0f, 0.0f, 0.0f, 0.0f));
    centerGrid();

    // Always shoot rays through the scene - even when a mouse button isn't pressed
    setMouseTracking(true);

    // Be sure to tell the parent window every time we muck with the scene
    QObject::connect(&m_undoManager, SIGNAL(cleanChanged(bool)),
                     parent, SLOT(reactToModified(bool)));
}


GLModelWidget::~GLModelWidget()
{
    //makeCurrent();
    //glDeleteLists(object, 1);

    delete m_activeTool;
}


void GLModelWidget::resizeAndClearVoxelGrid(const Imath::V3i& size)
{
    m_gvg = SproxelGrid(size);

    m_gvg.setAll(Imath::Color4f(0.0f, 0.0f, 0.0f, 0.0f));

    centerGrid();
    updateGL();
}


void GLModelWidget::resizeAndShiftVoxelGrid(const Imath::V3i& sizeInc, 
                                            const Imath::V3i& shift)
{
    SproxelGrid newGrid = SproxelGrid(m_gvg.cellDimensions() + sizeInc);
    newGrid.setAll(Imath::Color4f(0.0f, 0.0f, 0.0f, 0.0f));
    
    for (int x = 0; x < m_gvg.cellDimensions().x; x++)
    {
        const int xDest = x + shift.x;
        for (int y = 0; y < m_gvg.cellDimensions().y; y++)
        {
            const int yDest = y + shift.y;
            for (int z = 0; z < m_gvg.cellDimensions().z; z++)
            {
                const int zDest = z + shift.z;
                if ((xDest < 0 || xDest >= newGrid.cellDimensions().x) ||
                    (yDest < 0 || yDest >= newGrid.cellDimensions().y) ||
                    (zDest < 0 || zDest >= newGrid.cellDimensions().z))
                    continue;

                newGrid.set(Imath::V3i(xDest, yDest, zDest), 
                            m_gvg.get(Imath::V3i(x, y, z)));
            }
        }
    }
    
    m_undoManager.changeEntireVoxelGrid(m_gvg, newGrid);
    centerGrid();
    updateGL();
}


void GLModelWidget::reresVoxelGrid(const float scale)
{
    SproxelGrid newGrid = m_gvg;

    Imath::V3i newDimensions;
    newDimensions.x = (int)((float)newGrid.cellDimensions().x * scale);
    newDimensions.y = (int)((float)newGrid.cellDimensions().y * scale);
    newDimensions.z = (int)((float)newGrid.cellDimensions().z * scale);
    if (newDimensions.x <= 0 && newDimensions.y <= 0 && newDimensions.z <= 0)
        return;
    newGrid.setCellDimensions(newDimensions);

    Imath::V3d oldTranslation = newGrid.transform().translation();

    Imath::M44d transform;
    // TODO: Scaling the GVG is bad.  Maybe scaling the world around the new res would be better.
    //transform.setScale(Imath::V3d(1.0f/scale, 1.0f/scale, 1.0f/scale));
    //transform = newGrid.transform() * transform;
    transform[3][0] = oldTranslation.x * scale;
    transform[3][1] = oldTranslation.y * scale;
    transform[3][2] = oldTranslation.z * scale;
    newGrid.setTransform(transform);
    
    // Clear out the new voxel grid
    newGrid.setAll(Imath::Color4f(0.0f, 0.0f, 0.0f, 0.0f));
    
    // Move the stuff over, "changing resolution"
    // TODO: Make this less naieve.  Right now, when down-res'ing, you get a filled
    //       voxel if there is *anything* in its up-res'ed version.
    //       Could also take neighbors into account (round corners) when up-res'ing, etc.
    for (int x = 0; x < m_gvg.cellDimensions().x; x++)
    {
        for (int y = 0; y < m_gvg.cellDimensions().y; y++)
        {
            for (int z = 0; z < m_gvg.cellDimensions().z; z++)
            {
                const Imath::Color4f curCol = m_gvg.get(Imath::V3i(x,y,z));
                if (curCol.a != 0.0f)
                {
                    for (int xx = 0; xx < scale; xx++)
                    {
                        for (int yy = 0; yy < scale; yy++)
                        {
                            for (int zz = 0; zz < scale; zz++)
                            {
                                const Imath::V3i lowIndex((int)((float)x*scale)+xx,
                                                          (int)((float)y*scale)+yy,
                                                          (int)((float)z*scale)+zz);
                                newGrid.set(lowIndex, curCol);
                            }
                        }
                    }
                }
            }
        }
    }
    
    m_undoManager.changeEntireVoxelGrid(m_gvg, newGrid);
    updateGL();
}


QSize GLModelWidget::minimumSizeHint() const
{
    return QSize(50, 50);
}


QSize GLModelWidget::sizeHint() const
{
    return QSize(400, 400);
}


void GLModelWidget::setActiveTool(const SproxelTool tool)
{ 
    delete m_activeTool;
    switch (tool)
    {
        case TOOL_RAY: break;   // TODO: Add icon and implement!
        case TOOL_SPLAT: m_activeTool = new SplatToolState(&m_undoManager); break;
        case TOOL_FLOOD: m_activeTool = new FloodToolState(&m_undoManager); break;
        case TOOL_DROPPER: m_activeTool = new DropperToolState(&m_undoManager); break;
        case TOOL_ERASER: m_activeTool = new EraserToolState(&m_undoManager); break;
        case TOOL_REPLACE: m_activeTool = new ReplaceToolState(&m_undoManager); break;
        case TOOL_SLAB: m_activeTool = new SlabToolState(&m_undoManager); break;
    }
}


void GLModelWidget::initializeGL()
{
    QColor bg = p_appSettings->value("GLModelWidget/backgroundColor", QColor(161,161,161)).value<QColor>();
    glClearColor(bg.redF(), bg.greenF(), bg.blueF(), 0.0);

    glShadeModel(GL_FLAT);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    m_cam.setSize(400, 400);
    m_cam.lookAt(Imath::V3d(28, 21, 28), Imath::V3d(0.0, 0.0, 0.0));
    m_cam.setFovy(37.849289);
}


void GLModelWidget::resizeGL(int width, int height)
{
    m_cam.setSize(width, height);
    m_cam.autoSetClippingPlanes(fakeBounds);
}


void GLModelWidget::paintGL()
{
    QColor bg = p_appSettings->value("GLModelWidget/backgroundColor", QColor(161,161,161)).value<QColor>();
    glClearColor(bg.redF(), bg.greenF(), bg.blueF(), 0.0);

    m_cam.apply();

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (m_drawGrid)
    {
        // Shift the grid to the floor of the voxel grid
        // TODO: Eventually stop moving this to the floor and just keep it at 0,0,0
        Imath::Box3d worldBox = m_gvg.worldBounds();
        glPushMatrix();
        glTranslatef(0, worldBox.min.y, 0);

        // Grid drawing with color conversion
        QColor tempG  = p_appSettings->value("GLModelWidget/gridColor", QColor(0,0,0)).value<QColor>();
        QColor tempBG = p_appSettings->value("GLModelWidget/backgroundColor", QColor(161,161,161)).value<QColor>();
        glDrawGrid(p_appSettings->value("GLModelWidget/gridSize", 16).toInt(),
                   p_appSettings->value("GLModelWidget/gridCellSize", 1).toInt(),
                   Imath::Color4f(tempG.redF(),  tempG.greenF(),  tempG.blueF(),  1.0f),
                   Imath::Color4f(tempBG.redF(), tempBG.greenF(), tempBG.blueF(), 1.0f));
        glPopMatrix();
    }
    
    glDrawAxes();


    // Draw colored centers
    //glEnable(GL_BLEND); 
    //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glEnable(GL_LIGHTING);
    glEnable(GL_COLOR_MATERIAL);
    glEnable(GL_LIGHT0);
        
    GLfloat ambient[] = {0.0, 0.0, 0.0, 1.0};
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
    
    Imath::V3f camPos = m_cam.translation();
    GLfloat lightDir[4];
    lightDir[0] = camPos.x;
    lightDir[1] = camPos.y;
    lightDir[2] = camPos.z;
    lightDir[3] = 0.0; // w=0.0 means directional

    glLightfv(GL_LIGHT0, GL_POSITION, lightDir);

    GLfloat diffuse[] = {0.8, 0.8, 0.8, 1.0};
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);

    const Imath::V3i& dim = m_gvg.cellDimensions();
    for (int x = 0; x < dim.x; x++)
    {
        for (int y = 0; y < dim.y; y++)
        {
            for (int z = 0; z < dim.z; z++)
            {
                Imath::Color4f cellColor = m_gvg.get(Imath::V3i(x,y,z));
                const Imath::M44d mat = m_gvg.voxelTransform(Imath::V3i(x,y,z));

                if (cellColor.a != 0.0)
                {
                    glColor4f(cellColor.r, cellColor.g, cellColor.b, 0.2f);

                    glPushMatrix();
                    glMultMatrixd(glMatrix(mat));

                    if (p_appSettings->value("GLModelWidget/drawVoxelOutlines", 1).toBool())
                    {
                        // TODO: Make line width a setting
                        // TODO: Learn how to fix these polygon offset values to work properly.
                        //glLineWidth(1.5);
                        glEnable(GL_POLYGON_OFFSET_FILL);
                        glPolygonOffset(1.0, 1.0);
                        glDrawCubePoly();
                        glDisable(GL_POLYGON_OFFSET_FILL);

                        glEnable(GL_POLYGON_OFFSET_LINE);
                        glPolygonOffset(1.0, -5.0);
                        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                        glColor3f(1.0f - cellColor.r, 
                                  1.0f - cellColor.g,
                                  1.0f - cellColor.b);
                        glDrawCubePoly();
                        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                        glDisable(GL_POLYGON_OFFSET_LINE);
                        //glLineWidth(1.0);
                    }
                    else
                    {
                        glDrawCubePoly();
                    }

                    glPopMatrix();
                }
            }
        }
    }
    glDisable(GL_LIGHT0);
    glDisable(GL_COLOR_MATERIAL);
    glDisable(GL_LIGHTING);

    //glDisable(GL_BLEND);


    // DRAW UNPROJECTED LINE
    if (DEBUG_ME)
    {
        const Imath::Line3d& lastRay = m_activeTool->ray();
        glColor4f(1.0f, 1.0f, 0.0f, 1.0f);
        glBegin(GL_LINES);
        glVertex3d(lastRay.pos.x, lastRay.pos.y, lastRay.pos.z);
        glVertex3f(lastRay.pos.x + lastRay.dir.x * 100.0, 
                   lastRay.pos.y + lastRay.dir.y * 100.0, 
                   lastRay.pos.z + lastRay.dir.z * 100.0);
        glEnd();
    }

    // Grid stuff
    glEnable(GL_BLEND); 
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    if (m_drawVoxelGrid)
    {
        glColor4f(1.0f, 1.0f, 1.0f, 0.2f);
        glDrawVoxelGrid();
    }

    if (p_appSettings->value("GLModelWidget/previewEnabled", 1).toBool())
    {
        glColor4f(1.0f, 0.0f, 0.0f, 0.2f);
        glDrawPreviewVoxels();
    }

    if (m_activeVoxel != Imath::V3i(-1,-1,-1))
    {
        glColor4f(0.0f, 0.0f, 1.0f, 0.2f);
        glDrawActiveVoxel();
    }

    glDisable(GL_BLEND); 

    // Draw text stuff
    QFont font;
    font.setPointSize(10);
    glColor3f(1.0f, 1.0f, 1.0f);
    const char *sliceName[3] = { "Axis X, Slice YZ",
                                 "Axis Y, Slice XZ",
                                 "Axis Z, Slice XY" };
    renderText(10, 20, QString(sliceName[m_currAxis]), font);
    //renderText(10, 32, QString("%1, %2, %3")
    //                   .arg(m_activeVoxel.x)
    //                   .arg(m_activeVoxel.y)
    //                   .arg(m_activeVoxel.z));

    // DRAW BOUNDING BOX
    if (m_drawBoundingBox)
    {
        Imath::Box3d ext = dataBounds();
        Imath::V3d& min = ext.min;
        Imath::V3d& max = ext.max;

        if (!ext.isEmpty())
        {
            glColor3f(1.0f, 0.0f, 0.0f);
            glBegin(GL_LINE_LOOP);
            glVertex3f(min.x, min.y, min.z);
            glVertex3f(max.x, min.y, min.z);
            glVertex3f(max.x, min.y, max.z);
            glVertex3f(min.x, min.y, max.z);
            glEnd();

            glBegin(GL_LINE_LOOP);
            glVertex3f(min.x, max.y, min.z);
            glVertex3f(max.x, max.y, min.z);
            glVertex3f(max.x, max.y, max.z);
            glVertex3f(min.x, max.y, max.z);
            glEnd();

            glBegin(GL_LINES);
            glVertex3f(min.x, min.y, min.z);
            glVertex3f(min.x, max.y, min.z);
            glVertex3f(max.x, min.y, min.z);
            glVertex3f(max.x, max.y, min.z);
            glVertex3f(min.x, min.y, max.z);
            glVertex3f(min.x, max.y, max.z);
            glVertex3f(max.x, min.y, max.z);
            glVertex3f(max.x, max.y, max.z);
            glEnd();
        }
    }
    
    glLoadIdentity();
}


// Transform an Imath::M44d into a matrix usable by OpenGL.
double* GLModelWidget::glMatrix(const Imath::M44d& m)
{
    return (double*)(&m);
}


void GLModelWidget::glDrawGrid(const int size, 
                               const int gridCellSize,
                               const Imath::Color4f& gridColor,
                               const Imath::Color4f& bgColor)
{
    // TODO: Query and restore depth test
    glDisable(GL_DEPTH_TEST);

    // Lighter grid lines
    const Imath::Color4f lightColor = ((bgColor - gridColor) * 0.80) + gridColor;

    glBegin(GL_LINES);
    glColor4f(lightColor.r, lightColor.g, lightColor.b, 1.0f);
    for (int i = -size; i <= size; i++)
    {
        if (i == 0) continue;
        if (i % gridCellSize) continue;
        glVertex3f(i, 0,  size);
        glVertex3f(i, 0, -size);
        glVertex3f( size, 0, i);
        glVertex3f(-size, 0, i);
    }
    glEnd();

    // Darker main lines
    // TODO: Query and restore line width
    glLineWidth(2);
    glBegin(GL_LINES);
    glColor4f(gridColor.r, gridColor.g, gridColor.b, 1.0f);
    glVertex3f( size, 0, 0);
    glVertex3f(-size, 0, 0);
    glVertex3f(0, 0,  size);
    glVertex3f(0, 0, -size);
    glEnd();
    glLineWidth(1);

    glEnable(GL_DEPTH_TEST);
}


void GLModelWidget::glDrawAxes()
{
    // A little heavy-handed, but it gets the job done.
    GLCamera localCam;
    localCam.setSize(50, 50);
    localCam.setFovy(15);
    localCam.setRotation(m_cam.rotation());
    const Imath::V3d distance = (m_cam.translation() - m_cam.pointOfInterest()).normalized();
    localCam.setTranslation(distance*15.0);
    localCam.setCenterOfInterest(m_cam.centerOfInterest());

    // Set the new camera
    glLoadIdentity();
    localCam.apply();

    // Draw the axes
    glDisable(GL_DEPTH_TEST);
    glBegin(GL_LINES);
    glColor3f(1.0f, 0.0f, 0.0f);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(1.0f, 0.0f, 0.0f);
    glColor3f(0.0f, 1.0f, 0.0f);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(0.0f, 1.0f, 0.0f);
    glColor3f(0.0f, 0.0f, 1.0f);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(0.0f, 0.0f, 1.0f);
    glEnd();
    glEnable(GL_DEPTH_TEST);

    // Restore old camera
    glLoadIdentity();
    m_cam.apply();
}


void GLModelWidget::glDrawCubeWire()
{
    glBegin(GL_LINE_LOOP);
    glVertex3f(-0.5, -0.5, -0.5);
    glVertex3f( 0.5, -0.5, -0.5);
    glVertex3f( 0.5, -0.5,  0.5);
    glVertex3f(-0.5, -0.5,  0.5);
    glEnd();

    glBegin(GL_LINE_LOOP);
    glVertex3f(-0.5,  0.5, -0.5);
    glVertex3f( 0.5,  0.5, -0.5);
    glVertex3f( 0.5,  0.5,  0.5);
    glVertex3f(-0.5,  0.5,  0.5);
    glEnd();

    glBegin(GL_LINES);
    glVertex3f(-0.5, -0.5, -0.5);
    glVertex3f(-0.5,  0.5, -0.5);
    glVertex3f( 0.5, -0.5, -0.5);
    glVertex3f( 0.5,  0.5, -0.5);
    glVertex3f( 0.5, -0.5,  0.5);
    glVertex3f( 0.5,  0.5,  0.5);
    glVertex3f(-0.5, -0.5,  0.5);
    glVertex3f(-0.5,  0.5,  0.5);
    glEnd();
}


void GLModelWidget::glDrawCubePoly()
{
    glBegin(GL_QUADS);
    glNormal3f( 0.0f, 1.0f, 0.0f);
    glVertex3f( 0.5f, 0.5f,-0.5f);
    glVertex3f(-0.5f, 0.5f,-0.5f);
    glVertex3f(-0.5f, 0.5f, 0.5f);
    glVertex3f( 0.5f, 0.5f, 0.5f);
    
    glNormal3f( 0.0f,-1.0f, 0.0f);
    glVertex3f( 0.5f,-0.5f, 0.5f);
    glVertex3f(-0.5f,-0.5f, 0.5f);
    glVertex3f(-0.5f,-0.5f,-0.5f);
    glVertex3f( 0.5f,-0.5f,-0.5f);

    glNormal3f( 0.0f, 0.0f, 1.0f);
    glVertex3f( 0.5f, 0.5f, 0.5f);
    glVertex3f(-0.5f, 0.5f, 0.5f);
    glVertex3f(-0.5f,-0.5f, 0.5f);
    glVertex3f( 0.5f,-0.5f, 0.5f);

    glNormal3f( 0.0f, 0.0f,-1.0f);
    glVertex3f( 0.5f,-0.5f,-0.5f);
    glVertex3f(-0.5f,-0.5f,-0.5f);
    glVertex3f(-0.5f, 0.5f,-0.5f);
    glVertex3f( 0.5f, 0.5f,-0.5f);

    glNormal3f( 1.0f, 0.0f, 0.0f);
    glVertex3f( 0.5f, 0.5f,-0.5f);
    glVertex3f( 0.5f, 0.5f, 0.5f);
    glVertex3f( 0.5f,-0.5f, 0.5f);
    glVertex3f( 0.5f,-0.5f,-0.5f);

    glNormal3f(-1.0f, 0.0f, 0.0f);
    glVertex3f(-0.5f, 0.5f, 0.5f);
    glVertex3f(-0.5f, 0.5f,-0.5f);
    glVertex3f(-0.5f,-0.5f,-0.5f);
    glVertex3f(-0.5f,-0.5f, 0.5f);
    glEnd();
}


void GLModelWidget::glDrawVoxelGrid()
{
    const Imath::V3i& dim = m_gvg.cellDimensions();

    // TODO: Optimize the intersects stuff (or just ignore it)
    for (int x = 0; x < dim.x; x++)
    {
        for (int y = 0; y < dim.y+1; y++)
        {
            for (int z = 0; z < dim.z+1; z++)
            {
                if (p_appSettings->value("GLModelWidget/previewEnabled", 1).toBool())
                {
                    if (std::find(m_previews.begin(), m_previews.end(), Imath::V3i(x,y,z)) != m_previews.end())
                        continue;

                    if (std::find(m_previews.begin(), m_previews.end(), Imath::V3i(x,y-1,z)) != m_previews.end())
                        continue;

                    if (std::find(m_previews.begin(), m_previews.end(), Imath::V3i(x,y,z-1)) != m_previews.end())
                        continue;

                    if (std::find(m_previews.begin(), m_previews.end(), Imath::V3i(x,y-1,z-1)) != m_previews.end())
                        continue;
                }

                if (Imath::V3i(x,y,z) == m_activeVoxel   || Imath::V3i(x,y-1,z) == m_activeVoxel ||
                    Imath::V3i(x,y,z-1) == m_activeVoxel || Imath::V3i(x,y-1,z-1) == m_activeVoxel)
                {
                    continue;
                }

                const Imath::M44d mat = m_gvg.voxelTransform(Imath::V3i(x,y,z));
                
                glPushMatrix();
                glMultMatrixd(glMatrix(mat));

                glBegin(GL_LINES);
                glVertex3f(-0.5, -0.5, -0.5);
                glVertex3f( 0.5, -0.5, -0.5);
                glEnd();

                glPopMatrix();
            }
        }
    }

    for (int x = 0; x < dim.x+1; x++)
    {
        for (int y = 0; y < dim.y; y++)
        {
            for (int z = 0; z < dim.z+1; z++)
            {
                if (p_appSettings->value("GLModelWidget/previewEnabled", 1).toBool())
                {
                    if (std::find(m_previews.begin(), m_previews.end(), Imath::V3i(x,y,z)) != m_previews.end())
                        continue;

                    if (std::find(m_previews.begin(), m_previews.end(), Imath::V3i(x-1,y,z)) != m_previews.end())
                        continue;

                    if (std::find(m_previews.begin(), m_previews.end(), Imath::V3i(x,y,z-1)) != m_previews.end())
                        continue;

                    if (std::find(m_previews.begin(), m_previews.end(), Imath::V3i(x-1,y,z-1)) != m_previews.end())
                        continue;
                }

                if (Imath::V3i(x,y,z) == m_activeVoxel   || Imath::V3i(x-1,y,z) == m_activeVoxel ||
                    Imath::V3i(x,y,z-1) == m_activeVoxel || Imath::V3i(x-1,y,z-1) == m_activeVoxel)
                {
                    continue;
                }

                const Imath::M44d mat = m_gvg.voxelTransform(Imath::V3i(x,y,z));
                
                glPushMatrix();
                glMultMatrixd(glMatrix(mat));

                glBegin(GL_LINES);
                glVertex3f(-0.5, -0.5, -0.5);
                glVertex3f(-0.5,  0.5, -0.5);
                glEnd();

                glPopMatrix();
            }
        }
    }

    for (int x = 0; x < dim.x+1; x++)
    {
        for (int y = 0; y < dim.y+1; y++)
        {
            for (int z = 0; z < dim.z; z++)
            {
                if (p_appSettings->value("GLModelWidget/previewEnabled", 1).toBool())
                {
                    if (std::find(m_previews.begin(), m_previews.end(), Imath::V3i(x,y,z)) != m_previews.end())
                        continue;

                    if (std::find(m_previews.begin(), m_previews.end(), Imath::V3i(x-1,y,z)) != m_previews.end())
                        continue;

                    if (std::find(m_previews.begin(), m_previews.end(), Imath::V3i(x,y-1,z)) != m_previews.end())
                        continue;

                    if (std::find(m_previews.begin(), m_previews.end(), Imath::V3i(x-1,y-1,z)) != m_previews.end())
                        continue;
                }

                if (Imath::V3i(x,y,z) == m_activeVoxel   || Imath::V3i(x-1,y,z) == m_activeVoxel ||
                    Imath::V3i(x,y-1,z) == m_activeVoxel || Imath::V3i(x-1,y-1,z) == m_activeVoxel)
                {
                    continue;
                }

                const Imath::M44d mat = m_gvg.voxelTransform(Imath::V3i(x,y,z));
                
                glPushMatrix();
                glMultMatrixd(glMatrix(mat));

                glBegin(GL_LINES);
                glVertex3f(-0.5, -0.5, -0.5);
                glVertex3f(-0.5, -0.5,  0.5);
                glEnd();

                glPopMatrix();
            }
        }
    }
}


void GLModelWidget::glDrawActiveVoxel()
{
    const Imath::M44d mat = m_gvg.voxelTransform(m_activeVoxel);
    glPushMatrix();
    glMultMatrixd(glMatrix(mat));
    glDrawCubeWire();
    glPopMatrix();
}


void GLModelWidget::glDrawPreviewVoxels()
{
    Imath::Color4f curColor;
    glGetFloatv(GL_CURRENT_COLOR, (float*)&curColor);

    for (unsigned int i = 0; i < m_previews.size(); i++)
    {
        float scalar = 1.0f - ((float)i / (float)(m_previews.size()));
        curColor.r = curColor.r * scalar;
        curColor.g = curColor.g * scalar;
        curColor.b = curColor.b * scalar;
        
        const Imath::M44d mat = m_gvg.voxelTransform(m_previews[i]);
        glPushMatrix();
        glMultMatrixd(glMatrix(mat));
        glColor4f(curColor.r, curColor.g, curColor.b, curColor.a);
        glDrawCubeWire();
        glPopMatrix();
    }
}


void GLModelWidget::glDrawVoxelCenter(const size_t x, const size_t y, const size_t z)
{
    const Imath::V3d location = m_gvg.voxelTransform(Imath::V3i(x,y,z)).translation();

    glPointSize(5);
    glBegin(GL_POINTS);
    glVertex3f(location.x, location.y, location.z);
    glEnd();
    glPointSize(1);
}


void GLModelWidget::mousePressEvent(QMouseEvent *event)
{
    const bool altDown = event->modifiers() & Qt::AltModifier;
    const bool ctrlDown = event->modifiers() & Qt::ControlModifier;
    //const bool shiftDown = event->modifiers() & Qt::ShiftModifier;
    
    if (altDown)
    {
        m_lastMouse = event->pos();
    }
    else if (ctrlDown)
    {
        if (event->buttons() & Qt::LeftButton)
        {
            const Imath::Line3d localLine = 
                    m_cam.unproject(Imath::V2d(event->pos().x(), height() - event->pos().y()));

            // TODO: Figure out how to restore old tool properly in ReleaseEvent
            // CTRL+LMB is always replace - switch tools and execute
            SproxelTool currentTool = m_activeTool->type();
            setActiveTool(TOOL_REPLACE);
            m_activeTool->set(&m_gvg, localLine, m_activeColor);
            m_activeTool->execute();
            setActiveTool(currentTool);
            updateGL();
        }
    }
    else
    {
        Imath::Line3d localLine = 
                m_cam.unproject(Imath::V2d(event->pos().x(), height() - event->pos().y()));

        if (event->buttons() & Qt::LeftButton)
        {
            bool draggingEnabled = p_appSettings->value("GLModelWidget/dragEnabled", 1).toBool();
            m_activeTool->setDragSupport(draggingEnabled);
            m_activeTool->set(&m_gvg, localLine, m_activeColor);

            if (m_activeTool->type() == TOOL_SLAB)
                dynamic_cast<SlabToolState*>(m_activeTool)->setAxis(currentAxis());
            
            else if (m_activeTool->type() == TOOL_DROPPER)
            {
                // TODO: Coalesce this dropper code so i don't repeat it everywhere
                std::vector<Imath::V3i> ints = m_activeTool->voxelsAffected();
                if (ints.size() != 0)
                {
                    Imath::Color4f result = m_gvg.get(ints[0]);
                    emit colorSampled(result);
                }
                return;
            }
            
            m_activeTool->execute();
            updateGL();
        }
        else if (event->buttons() & Qt::MidButton)
        {
            // Middle button is always the color picker
            // TODO: Restore old tool properly in ReleaseEvent
            SproxelTool currentTool = m_activeTool->type();
            setActiveTool(TOOL_DROPPER);
            m_activeTool->set(&m_gvg, localLine, m_activeColor);
            
            // TODO: Coalesce this dropper code so i don't repeat it everywhere
            std::vector<Imath::V3i> ints = m_activeTool->voxelsAffected();
            if (ints.size() != 0)
            {
                Imath::Color4f result = m_gvg.get(ints[0]);
                emit colorSampled(result);
            }
            setActiveTool(currentTool);
            m_activeTool->setDragSupport(p_appSettings->value("GLModelWidget/dragEnabled", 1).toInt());
        }
        else if (event->buttons() & Qt::RightButton)
        {
            // Right button is always delete
            // TODO: Restore old tool properly in ReleaseEvent
            SproxelTool currentTool = m_activeTool->type();
            setActiveTool(TOOL_ERASER);
            m_activeTool->set(&m_gvg, localLine, m_activeColor);
            m_activeTool->execute();
            setActiveTool(currentTool);
            m_activeTool->setDragSupport(p_appSettings->value("GLModelWidget/dragEnabled", 1).toInt());
            updateGL();
        }
        return;
    }
}


void GLModelWidget::mouseMoveEvent(QMouseEvent *event)
{
    const bool altDown = event->modifiers() & Qt::AltModifier;

    if (altDown)
    {
        // Camera movement
        const int dx = event->pos().x() - m_lastMouse.x();
        const int dy = event->pos().y() - m_lastMouse.y();
        m_lastMouse = event->pos();

        if ((event->buttons() & (Qt::LeftButton | Qt::MidButton)) == (Qt::LeftButton | Qt::MidButton) ||
            (event->buttons() & (Qt::RightButton)))
        {
            m_cam.dolly(Imath::V2d(dx, dy));
            m_cam.autoSetClippingPlanes(fakeBounds);
        }
        else if (event->buttons() & Qt::LeftButton)
        {
            m_cam.rotate(Imath::V2d(dx, dy));
            m_cam.autoSetClippingPlanes(fakeBounds);
        }
        else if (event->buttons() & Qt::MidButton)
        {
            m_cam.track(Imath::V2d(dx, dy));
            m_cam.autoSetClippingPlanes(fakeBounds);
        }
        updateGL();
    }
    else
    {
        const Imath::Line3d localLine = 
                m_cam.unproject(Imath::V2d(event->pos().x(), height() - event->pos().y()));

        m_activeTool->set(&m_gvg, localLine, m_activeColor);

        // Left click means you're tooling.
        if (event->buttons() & Qt::LeftButton)
        {
            // If your active tool supports drag, tool away
            if (m_activeTool->supportsDrag())
            {
                // You want your preview to update even when you're tooling
                m_previews = m_activeTool->voxelsAffected();
                
                // Tool execution
                m_activeTool->execute();
                
                // TODO: Coalesce this dropper code so i don't repeat it everywhere
                if (m_activeTool->type() == TOOL_DROPPER)
                {
                    std::vector<Imath::V3i> ints = m_activeTool->voxelsAffected();
                    if (ints.size() != 0)
                    {
                        Imath::Color4f result = m_gvg.get(ints[0]);
                        emit colorSampled(result);
                    }
                }
                updateGL();
            }
        }
        else
        {
            // Tool preview
            if (p_appSettings->value("GLModelWidget/previewEnabled", 1).toBool())
            {
                if (m_activeTool->type() == TOOL_SLAB)
                    dynamic_cast<SlabToolState*>(m_activeTool)->setAxis(currentAxis());

                m_previews = m_activeTool->voxelsAffected();
                updateGL();
            }
        }
    }
}


void GLModelWidget::mouseReleaseEvent(QMouseEvent*)
{
    m_activeTool->decrementClicks();
}


Imath::Box3d GLModelWidget::dataBounds()
{
    Imath::Box3d retBox;
    
    for (int x = 0; x < m_gvg.cellDimensions().x; x++)
    {
        for (int y = 0; y < m_gvg.cellDimensions().y; y++)
        {
            for (int z = 0; z < m_gvg.cellDimensions().z; z++)
            {
                if (m_gvg.get(Imath::V3i(x,y,z)).a != 0.0f)
                {
                    retBox.extendBy(Imath::V3d(x,  y,  z));
                    retBox.extendBy(Imath::V3d(x+1,y+1,z+1));
                }
            }
        }
    }
    
    // (ImathBoxAlgo) This properly computes the world bounding box
    return Imath::transform(retBox, m_gvg.transform());
}


void GLModelWidget::centerGrid()
{
    Imath::M44d transform;
    Imath::V3d dDims = m_gvg.cellDimensions();
    transform.setTranslation(Imath::V3d(-dDims.x/2.0, 0, -dDims.z/2.0));
    m_gvg.setTransform(transform);
}


void GLModelWidget::frame(bool fullExtents)
{
    // Frame on data extents if they're present.  Otherwise grid world bounds
    Imath::Box3d ext = dataBounds();
    if (ext.isEmpty() || fullExtents)
        ext = m_gvg.worldBounds();

    m_cam.frame(ext);
    m_cam.autoSetClippingPlanes(fakeBounds);
    updateGL();
}


void GLModelWidget::handleArrows(QKeyEvent *event)
{
    const bool altDown = event->modifiers() & Qt::AltModifier;
    const bool ctrlDown = event->modifiers() & Qt::ControlModifier;
    
    // If you're holding alt, you're moving the camera
    if (altDown)
    {
        // TODO: Movement speed - inverse axes - multiple keys
        if (event->key() == Qt::Key_Left)  m_cam.rotate(Imath::V2d(-19,  0));
        if (event->key() == Qt::Key_Right) m_cam.rotate(Imath::V2d( 19,  0));
        if (event->key() == Qt::Key_Up)    m_cam.rotate(Imath::V2d( 0, -19));
        if (event->key() == Qt::Key_Down)  m_cam.rotate(Imath::V2d( 0,  19));
        
        updateGL();
        return;
    }
    
    
    // If you hit an arrow key and you're invisible, make visible
    if (m_activeVoxel == Imath::V3i(-1,-1,-1))
        m_activeVoxel = Imath::V3i(0,0,0);


    // Which way does camera up go?
    int udInc = 0;
    int* camUD = NULL;
    Imath::V3d camYVec; 
    m_cam.transform().multDirMatrix(Imath::V3d(0.0, 1.0, 0.0), camYVec);

    // TODO: Optimize since these are all obvious dot product results
    Imath::V3d objectXVec; m_gvg.transform().multDirMatrix(Imath::V3d(1.0, 0.0, 0.0), objectXVec);
    Imath::V3d objectYVec; m_gvg.transform().multDirMatrix(Imath::V3d(0.0, 1.0, 0.0), objectYVec);
    Imath::V3d objectZVec; m_gvg.transform().multDirMatrix(Imath::V3d(0.0, 0.0, 1.0), objectZVec);
    
    double xDot = camYVec.dot(objectXVec);
    double yDot = camYVec.dot(objectYVec);
    double zDot = camYVec.dot(objectZVec);
    
    if (fabs(xDot) > fabs(yDot) && fabs(xDot) > fabs(zDot))
    {
        camUD = &m_activeVoxel.x; 
        if (xDot > 0) udInc = 1;
        else          udInc = -1;
    }
    else if (fabs(zDot) > fabs(yDot) && fabs(zDot) > fabs(xDot))
    {
        camUD = &m_activeVoxel.z; 
        if (zDot > 0) udInc = 1;
        else          udInc = -1;
    }
    else if (fabs(yDot) > fabs(xDot) && fabs(yDot) > fabs(zDot))
    {
        camUD = &m_activeVoxel.y; 
        if (yDot > 0) udInc = 1;
        else          udInc = -1;
    }


    // Which way does camera right go?    
    int rlInc = 0;
    int* camRL = NULL;
    Imath::V3d camXVec; m_cam.transform().multDirMatrix(Imath::V3d(1.0, 0.0, 0.0), camXVec);
    xDot = camXVec.dot(objectXVec);
    yDot = camXVec.dot(objectYVec);
    zDot = camXVec.dot(objectZVec);
    
    if (fabs(xDot) >= fabs(yDot) && fabs(xDot) >= fabs(zDot))
    {
        camRL = &m_activeVoxel.x; 
        if (xDot > 0) rlInc = 1;
        else          rlInc = -1;
    }
    else if (fabs(zDot) >= fabs(yDot) && fabs(zDot) >= fabs(xDot))
    {
        camRL = &m_activeVoxel.z; 
        if (zDot > 0) rlInc = 1;
        else          rlInc = -1;
    }
    else if (fabs(yDot) >= fabs(xDot) && fabs(yDot) >= fabs(zDot))
    {
        camRL = &m_activeVoxel.y; 
        if (yDot > 0) rlInc = 1;
        else          rlInc = -1;
    }


    // Which way does camera depth go?
    int fbInc = 0;
    int* camFB = NULL;
    Imath::V3d camZVec; m_cam.transform().multDirMatrix(Imath::V3d(0.0, 0.0, -1.0), camZVec);
    xDot = camZVec.dot(objectXVec);
    yDot = camZVec.dot(objectYVec);
    zDot = camZVec.dot(objectZVec);
    
    if (fabs(xDot) >= fabs(yDot) && fabs(xDot) >= fabs(zDot))
    {
        camFB = &m_activeVoxel.x; 
        if (xDot > 0) fbInc = 1;
        else          fbInc = -1;
    }
    else if (fabs(zDot) >= fabs(yDot) && fabs(zDot) >= fabs(xDot))
    {
        camFB = &m_activeVoxel.z; 
        if (zDot > 0) fbInc = 1;
        else          fbInc = -1;
    }
    else if (fabs(yDot) >= fabs(xDot) && fabs(yDot) >= fabs(zDot))
    {
        camFB = &m_activeVoxel.y; 
        if (yDot > 0) fbInc = 1;
        else          fbInc = -1;
    }

    // Apply the results   
    switch (event->key())
    {
        case Qt::Key_Left:  *camRL += -rlInc; break;
        case Qt::Key_Right: *camRL +=  rlInc; break;
        case Qt::Key_Up:    if (ctrlDown) *camFB +=  fbInc; else *camUD +=  udInc; break;
        case Qt::Key_Down:  if (ctrlDown) *camFB += -fbInc; else *camUD += -udInc; break;
        default: break;
    }
    
    // Clamp on the edges
    if (m_activeVoxel.x >= m_gvg.cellDimensions().x) m_activeVoxel.x = m_gvg.cellDimensions().x - 1;
    if (m_activeVoxel.y >= m_gvg.cellDimensions().y) m_activeVoxel.y = m_gvg.cellDimensions().y - 1;
    if (m_activeVoxel.z >= m_gvg.cellDimensions().z) m_activeVoxel.z = m_gvg.cellDimensions().z - 1;
    if (m_activeVoxel.x < 0) m_activeVoxel.x = 0;
    if (m_activeVoxel.y < 0) m_activeVoxel.y = 0;
    if (m_activeVoxel.z < 0) m_activeVoxel.z = 0;

    
    updateGL();
}


bool GLModelWidget::loadGridCSV(const std::string& filename)
{
    int fscanfStatus = 0;
    FILE* fp = fopen(filename.c_str(), "rb");
    if (!fp) return false;

    // Read the dimensions
    Imath::V3i size;
    fscanfStatus = fscanf(fp, "%d,%d,%d\n", &size.x, &size.y, &size.z);
    m_gvg.setCellDimensions(size);
    
    // Read the data
    Imath::Color4f color;
    const Imath::V3i& cellDim = m_gvg.cellDimensions();
    for (int y = cellDim.y-1; y >= 0; y--)
    {
        for (int z = 0; z < cellDim.z; z++)
        {
            for (int x = 0; x < cellDim.x; x++)
            {
                int r, g, b, a;
                fscanfStatus = fscanf(fp, "#%02X%02X%02X%02X,", &r, &g, &b, &a);

                color.r = r / (float)0xff;
                color.g = g / (float)0xff;
                color.b = b / (float)0xff;
                color.a = a / (float)0xff;
                m_gvg.set(Imath::V3i(x,y,z), color);

                if (x != cellDim.x-1)
                    fscanfStatus = fscanf(fp, ",");
            }
            fscanfStatus = fscanf(fp, "\n");
        }
        fscanfStatus = fscanf(fp, "\n");
    }
    fclose(fp);

    centerGrid();
    updateGL();
    return true;
}


bool GLModelWidget::saveGridCSV(const std::string& filename)
{
    FILE* fp = fopen(filename.c_str(), "wb");
    if (!fp) return false;
    
    const Imath::V3i& cellDim = m_gvg.cellDimensions();
    fprintf(fp, "%d,%d,%d\n", cellDim.x, cellDim.y, cellDim.z);
    
    // The csv is laid out human-readable (top->bottom, Y-up, XZ, etc)
    for (int y = cellDim.y-1; y >= 0; y--)
    {
        for (int z = 0; z < cellDim.z; z++)
        {
            for (int x = 0; x < cellDim.x; x++)
            {
                const Imath::V3i curLoc(x,y,z);
                Imath::Color4f col = m_gvg.get(curLoc);
                fprintf(fp, "#%02X%02X%02X%02X",
                        (int)(col.r*0xff),
                        (int)(col.g*0xff),
                        (int)(col.b*0xff),
                        (int)(col.a*0xff));
                if (x != cellDim.x-1)
                    fprintf(fp, ",");
            }
            fprintf(fp, "\n");
        }
        fprintf(fp, "\n");
    }
    
    fclose(fp);
    return true;
}


bool GLModelWidget::loadGridPNG(const std::string& filename)
{
    QImage readMe;
    if (!readMe.load(QString(filename.c_str()), "PNG"))
        return false;
    
    QString tempStr;
    tempStr = readMe.text("VoxelGridDimX");
    int sizeX = tempStr.toInt();
    tempStr = readMe.text("VoxelGridDimY");
    int sizeY = tempStr.toInt();
    tempStr = readMe.text("VoxelGridDimZ");
    int sizeZ = tempStr.toInt();
    if (sizeX == 0 || sizeY == 0 || sizeZ == 0)
        return false;

    readMe = readMe.mirrored();

    // Clear and load
    m_gvg.setCellDimensions(Imath::V3i(sizeX, sizeY, sizeZ));
    m_gvg.setAll(Imath::Color4f(0.0f, 0.0f, 0.0f, 0.0f));

    for (int slice = 0; slice < m_gvg.cellDimensions().z; slice++)
    {
        const int sliceOffset = slice * m_gvg.cellDimensions().x;
        for (int y = 0; y < m_gvg.cellDimensions().y; y++)
        {
            for (int x = 0; x < m_gvg.cellDimensions().x; x++)
            {
                QRgb pixelValue = readMe.pixel(x+sliceOffset, y);
                Imath::Color4f color((float)qRed(pixelValue) / 255.0f, 
                                     (float)qGreen(pixelValue) / 255.0f, 
                                     (float)qBlue(pixelValue) / 255.0f, 
                                     (float)qAlpha(pixelValue) / 255.0f);
                m_gvg.set(Imath::V3i(x, y, slice), color);
            }
        }
    }

    centerGrid();
    updateGL();
    return true;
}


bool GLModelWidget::saveGridPNG(const std::string& filename)
{
    // TODO: Offer other options besides XY slices?  Directionality?  Ordering?
    const int height = m_gvg.cellDimensions().y;
    const int width = m_gvg.cellDimensions().x * m_gvg.cellDimensions().z;
    QImage writeMe(QSize(width, height), QImage::Format_ARGB32);

    for (int slice = 0; slice < m_gvg.cellDimensions().z; slice++)
    {
        const int sliceOffset = slice * m_gvg.cellDimensions().x;
        for (int y = 0; y < m_gvg.cellDimensions().y; y++)
        {
            for (int x = 0; x < m_gvg.cellDimensions().x; x++)
            {
                const Imath::Color4f& colorScaled = m_gvg.get(Imath::V3i(x, y, slice)) * 255.0f;
                writeMe.setPixel(x+sliceOffset, y, qRgba((int)colorScaled.r,
                                                         (int)colorScaled.g,
                                                         (int)colorScaled.b,
                                                         (int)colorScaled.a));
            }
        }
    }
    writeMe = writeMe.mirrored();   // QT Bug: mirrored() doesn't preserve text.

    QString tempStr;
    writeMe.setText("SproxelFileVersion", "1");
    writeMe.setText("VoxelGridDimX", tempStr.setNum(m_gvg.cellDimensions().x));
    writeMe.setText("VoxelGridDimY", tempStr.setNum(m_gvg.cellDimensions().y));
    writeMe.setText("VoxelGridDimZ", tempStr.setNum(m_gvg.cellDimensions().z));

    return writeMe.save(QString(filename.c_str()));
}


bool GLModelWidget::importImageIntoGrid(const std::string& filename)
{
    QImage imported;
    if (!imported.load(filename.c_str()))
        return false;
    imported = imported.mirrored();
    
    // For now we always import into the Z axis and resize if we need to
    const int imageSizeX = imported.width();
    const int imageSizeY = imported.height();
    
    const Imath::V3i oldDims = m_gvg.cellDimensions();
    Imath::V3i newDims = m_gvg.cellDimensions();

    if (imageSizeX > oldDims.x) newDims.x = imageSizeX;
    if (imageSizeY > oldDims.y) newDims.y = imageSizeY;
    m_gvg.setCellDimensions(newDims);
    
    // Clear out the uninitialized new region : TODO: FUnctionize
    for (int x = 0; x < newDims.x; x++)
    {
        for (int y = 0; y < newDims.y; y++)
        {
            for (int z = 0; z < newDims.z; z++)
            {
                if (x < oldDims.x && y < oldDims.y && z < oldDims.z)
                    continue;
                m_gvg.set(Imath::V3i(x, y, z), 
                          Imath::Color4f(0.0f, 0.0f, 0.0f, 0.0f));
            }
        }
    }
    
    // Splat the data in
    for (int x = 0; x < imageSizeX; x++)
    {
        for (int y = 0; y < imageSizeY; y++)
        {
            const Imath::V3i locale(x, y, 0);
            const QColor qcolor = imported.pixel(x, y);
            const Imath::Color4f color(qcolor.redF(),
                                       qcolor.greenF(),
                                       qcolor.blueF(),
                                       qcolor.alphaF());
            m_gvg.set(locale, color);
        }
    }
    
    centerGrid();
    updateGL();
    return true;
}


void GLModelWidget::objWritePoly(FILE* fp, bool asTriangles, 
                                 const int& v0, const int& v1, const int& v2, const int& v3)
{
    if (!asTriangles)
    {
        fprintf(fp, "f %d %d %d %d\n", v0, v1, v2, v3);
    }
    else
    {
        fprintf(fp, "f %d %d %d\n", v0, v1, v2);
        fprintf(fp, "f %d %d %d\n", v2, v3, v0);
    }
}

bool GLModelWidget::exportGridOBJ(const std::string& filename, bool asTriangles)
{
    // Get file basename and extension
    QFileInfo fi(QString(filename.c_str()));
    QString basename = fi.completeBaseName();
    QString basedir = fi.absolutePath();

    // Shorthand
    const int sx = m_gvg.cellDimensions().x;
    const int sy = m_gvg.cellDimensions().y;
    const int sz = m_gvg.cellDimensions().z;

    // Create and write the material file
    std::map<std::string, std::string> mtlMap;

    // Build up the material lists
    for (int y = 0; y < sy; y++)
    {
        for (int z = 0; z < sz; z++)
        {
            for (int x = 0; x < sx; x++)
            {
                const Imath::Color4f& color = m_gvg.get(Imath::V3i(x, y, z));
                
                if (color.a == 0.0f) continue;
                
                char mtlName[64];
                sprintf(mtlName, "mtl%d", (int)mtlMap.size());

                char colorString[64];
                sprintf(colorString, "Kd %.4f %.4f %.4f", color.r, color.g, color.b);
                mtlMap.insert(std::pair<std::string, std::string>(std::string(colorString), std::string(mtlName)));
            }
        }
    }

    // Write .mtl file
    QString mtlFilename = basedir + "/" + basename + ".mtl";
    FILE* fp = fopen(mtlFilename.toAscii().constData(), "wb");
    if (!fp) return false;

    for(std::map<std::string, std::string>::iterator p = mtlMap.begin();
        p != mtlMap.end();
        ++p)
    {
        fprintf(fp, "newmtl %s\n", p->second.c_str());
        fprintf(fp, "illum 4\n");
        fprintf(fp, "%s\n", p->first.c_str());
        fprintf(fp, "Ka 0.00 0.00 0.00\n");
        fprintf(fp, "Tf 1.00 1.00 1.00\n");
        fprintf(fp, "Ni 1.00\n");
        fprintf(fp, "\n");
    }
    fclose(fp);


    // Create and write the obj file
    fp = fopen(filename.c_str(), "wb");
    if (!fp) return false;

    // Geometry
    const int vertListLength = (sx+1) * (sy+1) * (sz+1);
    int* vertList = new int[vertListLength];
    memset(vertList, 0, sizeof(int)*vertListLength);

    // Material library
    fprintf(fp, "mtllib %s.mtl\n", basename.toAscii().constData());

    // The object's name
    fprintf(fp, "g %s\n", basename.toAscii().constData());

    // Populate the vert list
    int vertIndex = 1;
    for (int y = 0; y < (sy+1); y++)
    {
        for (int z = 0; z < (sz+1); z++)
        {
            for (int x = 0; x < (sx+1); x++)
            {
                int neighbors = 0;                
                if ((x!=0)  && (y!=0)  && (z!=0)  && (m_gvg.get(Imath::V3i(x-1, y-1, z-1)).a != 0.0f)) neighbors++;
                if ((x!=0)  && (y!=0)  && (z!=sz) && (m_gvg.get(Imath::V3i(x-1, y-1, z)).a   != 0.0f)) neighbors++;
                if ((x!=0)  && (y!=sy) && (z!=0)  && (m_gvg.get(Imath::V3i(x-1, y,   z-1)).a != 0.0f)) neighbors++;
                if ((x!=0)  && (y!=sy) && (z!=sz) && (m_gvg.get(Imath::V3i(x-1, y,   z)).a   != 0.0f)) neighbors++;
                if ((x!=sx) && (y!=0)  && (z!=0)  && (m_gvg.get(Imath::V3i(x,   y-1, z-1)).a != 0.0f)) neighbors++;
                if ((x!=sx) && (y!=0)  && (z!=sz) && (m_gvg.get(Imath::V3i(x,   y-1, z)).a   != 0.0f)) neighbors++;
                if ((x!=sx) && (y!=sy) && (z!=0)  && (m_gvg.get(Imath::V3i(x,   y,   z-1)).a != 0.0f)) neighbors++;
                if ((x!=sx) && (y!=sy) && (z!=sz) && (m_gvg.get(Imath::V3i(x,   y,   z)).a   != 0.0f)) neighbors++;

                if (neighbors == 0 || neighbors == 8)
                    continue;
                
                const int vlIndex = (y*(sz+1)*(sx+1)) + (z*(sx+1)) + (x);
                vertList[vlIndex] = vertIndex;
                vertIndex++;
            }
        }
    }

    // Write the verts to the OBJ
    for (int y = 0; y < (sy+1); y++)
    {
        for (int z = 0; z < (sz+1); z++)
        {
            for (int x = 0; x < (sx+1); x++)
            {
                Imath::V3i voxelToCheck = Imath::V3i(x,y,z);
                if (x == sx) voxelToCheck.x--;
                if (y == sy) voxelToCheck.y--;
                if (z == sz) voxelToCheck.z--;

                const Imath::M44d mat = m_gvg.voxelTransform(voxelToCheck);
                
                Imath::V3d vert;
                mat.multVecMatrix(Imath::V3d((x == sx) ? 0.5f : -0.5f, 
                                             (y == sy) ? 0.5f : -0.5f, 
                                             (z == sz) ? 0.5f : -0.5f), vert);

                const int vlIndex = (y*(sz+1)*(sx+1)) + (z*(sx+1)) + (x);
                if (vertList[vlIndex] != 0)
                {
                    fprintf(fp, "v %f %f %f\n", vert.x, vert.y, vert.z);
                }
            }
        }
    }

    // Create all faces
    for (int y = 0; y < sy; y++)
    {
        for (int z = 0; z < sz; z++)
        {
            for (int x = 0; x < sx; x++)
            {
                const Imath::Color4f& color = m_gvg.get(Imath::V3i(x, y, z));
                if (color.a == 0.0f) 
                    continue;
                
                // Check for crossings
                bool crossNegX = false;
                bool crossPosX = false;
                if (x == 0)
                    crossNegX = true;
                else if (color.a != m_gvg.get(Imath::V3i(x-1,y,z)).a)
                    crossNegX = true;
                
                if (x == sx-1)
                    crossPosX = true;
                else if (color.a != m_gvg.get(Imath::V3i(x+1,y,z)).a)
                    crossPosX = true;
                
                bool crossNegY = false;
                bool crossPosY = false;
                if (y == 0)
                    crossNegY = true;
                else if (color.a != m_gvg.get(Imath::V3i(x,y-1,z)).a)
                    crossNegY = true;
                
                if (y == sy-1)
                    crossPosY = true;
                else if (color.a != m_gvg.get(Imath::V3i(x,y+1,z)).a)
                    crossPosY = true;
                
                bool crossNegZ = false;
                bool crossPosZ = false;
                if (z == 0)
                    crossNegZ = true;
                else if (color.a != m_gvg.get(Imath::V3i(x,y,z-1)).a)
                    crossNegZ = true;
                
                if (z == sz-1)
                    crossPosZ = true;
                else if (color.a != m_gvg.get(Imath::V3i(x,y,z+1)).a)
                    crossPosZ = true;
                
                // If there are any crossings, you will need a material
                if (crossNegX || crossPosX || crossNegY || crossPosY || crossNegZ || crossPosZ)
                {
                    char colorString[64];
                    sprintf(colorString, "Kd %.4f %.4f %.4f", color.r, color.g, color.b);
                    const std::string mtl = mtlMap.find(colorString)->second;
                    fprintf(fp, "usemtl %s\n", mtl.c_str());
                }
                
                // Fill in the voxels
                const int* vl = vertList;
                const int  vi       = ((y)  *(sz+1)*(sx+1)) + ((z)  *(sx+1)) + (x);
                const int  viNextZ  = ((y)  *(sz+1)*(sx+1)) + ((z+1)*(sx+1)) + (x);
                const int  viNextY  = ((y+1)*(sz+1)*(sx+1)) + ((z)  *(sx+1)) + (x);
                const int  viNextZY = ((y+1)*(sz+1)*(sx+1)) + ((z+1)*(sx+1)) + (x);

                if (crossNegX) 
                    objWritePoly(fp, asTriangles, vl[vi],   vl[viNextZ],   vl[viNextZY],   vl[viNextY]);
                if (crossPosX) 
                    objWritePoly(fp, asTriangles, vl[vi+1], vl[viNextY+1], vl[viNextZY+1], vl[viNextZ+1]);

                if (crossNegY)
                    objWritePoly(fp, asTriangles, vl[vi],      vl[vi+1],     vl[viNextZ+1],  vl[viNextZ]);
                if (crossPosY)
                    objWritePoly(fp, asTriangles, vl[viNextY], vl[viNextZY], vl[viNextZY+1], vl[viNextY+1]);

                if (crossNegZ)
                    objWritePoly(fp, asTriangles, vl[vi],      vl[viNextY],  vl[viNextY+1],  vl[vi+1]);
                if (crossPosZ)
                    objWritePoly(fp, asTriangles, vl[viNextZ], vl[viNextZ+1], vl[viNextZY+1], vl[viNextZY]);
            }
        }
    }

    delete[] vertList;    
    fclose(fp);
    return true;
}


void GLModelWidget::shiftVoxels(const SproxelAxis axis, const bool up, const bool wrap)
{
    // Simplifiers for which way to shift
    size_t tan0AxisDim = 0;
    size_t tan1AxisDim = 0;
    size_t primaryAxisDim = 0;
    switch (axis)
    {
        case X_AXIS: primaryAxisDim = m_gvg.cellDimensions().x;
                        tan0AxisDim = m_gvg.cellDimensions().y;
                        tan1AxisDim = m_gvg.cellDimensions().z; break;
        case Y_AXIS: primaryAxisDim = m_gvg.cellDimensions().y;
                        tan0AxisDim = m_gvg.cellDimensions().x;
                        tan1AxisDim = m_gvg.cellDimensions().z; break;
        case Z_AXIS: primaryAxisDim = m_gvg.cellDimensions().z;
                        tan0AxisDim = m_gvg.cellDimensions().x;
                        tan1AxisDim = m_gvg.cellDimensions().y; break;
    }

    // Simplifiers for wrapping
    size_t backupIndex = 0;
    size_t clearIndex = primaryAxisDim - 1;
    if (up) std::swap(backupIndex, clearIndex);


    m_undoManager.beginMacro("Shift");

    // Backup the necessary slice
    Imath::Color4f* sliceBackup = NULL;
    if (wrap)
    {
        sliceBackup = new Imath::Color4f[tan0AxisDim * tan1AxisDim];
        for (size_t a = 0; a < tan0AxisDim; a++)
        {
            for (size_t b = 0; b < tan1AxisDim; b++)
            {
                Imath::V3i index(-1, -1, -1);
                switch (axis)
                {
                    case X_AXIS: index = Imath::V3i(backupIndex, a, b); break;
                    case Y_AXIS: index = Imath::V3i(a, backupIndex, b); break;
                    case Z_AXIS: index = Imath::V3i(a, b, backupIndex); break;
                }
                sliceBackup[a + (b * tan0AxisDim)] = m_gvg.get(index);
            }
        }
    }

    // Shift everyone over
    if (up)
    {
        for (size_t a = backupIndex; a > clearIndex; a--)
        {
            for (size_t b = 0; b < tan0AxisDim; b++)
            {
                for (size_t c = 0; c < tan1AxisDim; c++)
                {
                    Imath::V3i nextIndex(-1, -1, -1);
                    switch (axis)
                    {
                        case X_AXIS: setVoxelColor(Imath::V3i(a, b, c), m_gvg.get(Imath::V3i(a-1, b, c))); break;
                        case Y_AXIS: setVoxelColor(Imath::V3i(b, a, c), m_gvg.get(Imath::V3i(b, a-1, c))); break;
                        case Z_AXIS: setVoxelColor(Imath::V3i(b, c, a), m_gvg.get(Imath::V3i(b, c, a-1))); break;
                    }
                    
                }
            }
        }
    }
    else
    {
        for (size_t a = backupIndex; a < clearIndex; a++)
        {
            for (size_t b = 0; b < tan0AxisDim; b++)
            {
                for (size_t c = 0; c < tan1AxisDim; c++)
                {
                    Imath::V3i nextIndex(-1, -1, -1);
                    switch (axis)
                    {
                        case X_AXIS: setVoxelColor(Imath::V3i(a, b, c), m_gvg.get(Imath::V3i(a+1, b, c))); break;
                        case Y_AXIS: setVoxelColor(Imath::V3i(b, a, c), m_gvg.get(Imath::V3i(b, a+1, c))); break;
                        case Z_AXIS: setVoxelColor(Imath::V3i(b, c, a), m_gvg.get(Imath::V3i(b, c, a+1))); break;
                    }
                }
            }
        }
    }

    // Either clear or set the wrap-to slice
    for (size_t a = 0; a < tan0AxisDim; a++)
    {
        for (size_t b = 0; b < tan1AxisDim; b++)
        {
            Imath::V3i workIndex(-1, -1, -1);
            switch (axis)
            {
                case X_AXIS: workIndex = Imath::V3i(clearIndex, a, b); break;
                case Y_AXIS: workIndex = Imath::V3i(a, clearIndex, b); break;
                case Z_AXIS: workIndex = Imath::V3i(a, b, clearIndex); break;
            }
            
            if (wrap) 
                setVoxelColor(workIndex, sliceBackup[a + (b * tan0AxisDim)]);
            else
                setVoxelColor(workIndex, Imath::Color4f(0.0f, 0.0f, 0.0f, 0.0f));
        }
    }
    delete[] sliceBackup;

    m_undoManager.endMacro();
    updateGL();
}


void GLModelWidget::rotateVoxels(const SproxelAxis axis, const int dir)
{
    SproxelGrid newGrid = m_gvg;

    // Set some new dimensions
    Imath::V3i newDim(0,0,0);
    Imath::V3i oldDim = m_gvg.cellDimensions();
    switch (axis)
    {
        case X_AXIS: newDim.x = oldDim.x; newDim.y = oldDim.z; newDim.z = oldDim.y; break;
        case Y_AXIS: newDim.x = oldDim.z; newDim.y = oldDim.y; newDim.z = oldDim.x; break;
        case Z_AXIS: newDim.x = oldDim.y; newDim.y = oldDim.x; newDim.z = oldDim.z; break;
    }
    newGrid.setCellDimensions(newDim);

    // Do the rotation
    for (int x = 0; x < newGrid.cellDimensions().x; x++)
    {
        for (int y = 0; y < newGrid.cellDimensions().y; y++)
        {
            for (int z = 0; z < newGrid.cellDimensions().z; z++)
            {
                Imath::V3i oldLocation(0,0,0);
                switch (axis)
                {
                    case X_AXIS: if (dir > 0) oldLocation = Imath::V3i(x, oldDim.y-1-z, y);
                                 else         oldLocation = Imath::V3i(x, z, oldDim.z-1-y);
                                 break;
                    case Y_AXIS: if (dir > 0) oldLocation = Imath::V3i(z, y, oldDim.z-1-x);
                                 else         oldLocation = Imath::V3i(oldDim.x-1-z, y, x);
                                 break;
                    case Z_AXIS: if (dir > 0) oldLocation = Imath::V3i(oldDim.x-1-y, x, z);
                                 else         oldLocation = Imath::V3i(y, oldDim.y-1-x, z);
                                 break;
                }
                newGrid.set(Imath::V3i(x,y,z), m_gvg.get(oldLocation));
            }
        }
    }

    m_undoManager.changeEntireVoxelGrid(m_gvg, newGrid);

    centerGrid();
    updateGL();
}

void GLModelWidget::mirrorVoxels(const SproxelAxis axis)
{
    SproxelGrid backup = m_gvg;

    m_undoManager.beginMacro("Mirror");    
    
    for (int x = 0; x < m_gvg.cellDimensions().x; x++)
    {
        for (int y = 0; y < m_gvg.cellDimensions().y; y++)
        {
            for (int z = 0; z < m_gvg.cellDimensions().z; z++)
            {
                Imath::V3i oldLocation(-1, -1, -1);
                switch (axis)
                {
                    case X_AXIS: oldLocation = Imath::V3i(backup.cellDimensions().x-x-1, y, z); break;
                    case Y_AXIS: oldLocation = Imath::V3i(x, backup.cellDimensions().y-y-1, z); break;
                    case Z_AXIS: oldLocation = Imath::V3i(x, y, backup.cellDimensions().z-z-1); break;
                }
                setVoxelColor(Imath::V3i(x,y,z), backup.get(oldLocation));
            }
        }
    }
    
    m_undoManager.endMacro();
    updateGL();
}


// This is only here for the MainWindow now.  Should be changed.
void GLModelWidget::setVoxelColor(const Imath::V3i& index, const Imath::Color4f color)
{
    // Validity check
    const Imath::V3i& cd = m_gvg.cellDimensions();
    if (index.x < 0     || index.y < 0     || index.z < 0 ||
        index.x >= cd.x || index.y >= cd.y || index.z >= cd.z)
        return;
    
    m_undoManager.setVoxelColor(m_gvg, index, color);
}
