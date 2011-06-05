#include <QtGui>
#include <QtOpenGL>

#include <cmath>
#include <iostream>
#include <algorithm>

#include <ImathLine.h>
#include <ImathBoxAlgo.h>

#include "GLModelWidget.h"

#define DEBUG_ME (0)

#define DEFAULT_VOXGRID_SZ (8)

Imath::Line3d fakeLine;
Imath::Box3d fakeBounds(Imath::V3d(-50, -50, -50), Imath::V3d(50, 50, 50));

GLModelWidget::GLModelWidget(QWidget *parent)
    : QGLWidget(parent),
      m_cam(),
      m_gvg(Imath::V3i(DEFAULT_VOXGRID_SZ, DEFAULT_VOXGRID_SZ, DEFAULT_VOXGRID_SZ)),
      m_intersects(),
      m_activeVoxel(-1,-1,-1),
      m_activeColor(1.0f, 1.0f, 1.0f, 1.0f),
      m_gridColor(0.0f, 0.0f, 0.0f, 0.0f),
      m_backgroundColor(0.63f, 0.63f, 0.63f, 0.0f),
      m_lastMouse(),
      m_drawGrid(true),
      m_drawVoxelGrid(true),
      m_drawBoundingBox(false),
      m_shiftWrap(true),
      m_currAxis(Y_AXIS),
      m_activeTool(TOOL_SPLAT)
{
    m_gvg.setAll(Imath::Color4f(0.0f, 0.0f, 0.0f, 0.0f));

    Imath::M44d transform;
    Imath::V3d dDims = m_gvg.cellDimensions();
    transform.setTranslation(Imath::V3d(-dDims.x/2.0, 0, -dDims.z/2.0));
    m_gvg.setTransform(transform);

    // Be sure to tell the parent window every time we muck with the scene
    QObject::connect(&m_undoStack, SIGNAL(cleanChanged(bool)),
                     parent, SLOT(reactToModified(bool)));
}


GLModelWidget::~GLModelWidget()
{
    //makeCurrent();
    //glDeleteLists(object, 1);
}


void GLModelWidget::resizeVoxelGrid(Imath::V3i size)
{
    m_gvg = GameVoxelGrid<Imath::Color4f>(size);

    m_gvg.setAll(Imath::Color4f(0.0f, 0.0f, 0.0f, 0.0f));

    Imath::M44d transform;
    Imath::V3d dDims = m_gvg.cellDimensions();
    transform.setTranslation(Imath::V3d(-dDims.x/2.0, 0, -dDims.z/2.0));
    m_gvg.setTransform(transform);

    updateGL();
}


void GLModelWidget::reresVoxelGrid(const float scale)
{
    GameVoxelGrid<Imath::Color4f> tempGVG = m_gvg;

    Imath::V3i newDimensions;
    newDimensions.x = (int)((float)m_gvg.cellDimensions().x * scale);
    newDimensions.y = (int)((float)m_gvg.cellDimensions().y * scale);
    newDimensions.z = (int)((float)m_gvg.cellDimensions().z * scale);
    if (newDimensions.x <= 0 && newDimensions.y <= 0 && newDimensions.z <= 0)
        return;
    m_gvg.setCellDimensions(newDimensions);

    Imath::V3d oldTranslation = m_gvg.transform().translation();

    Imath::M44d transform;
    // TODO: Scaling the GVG is bad.  Maybe scaling the world around the new res would be better.
    //transform.setScale(Imath::V3d(1.0f/scale, 1.0f/scale, 1.0f/scale));
    //transform = m_gvg.transform() * transform;
    transform[3][0] = oldTranslation.x * scale;
    transform[3][1] = oldTranslation.y * scale;
    transform[3][2] = oldTranslation.z * scale;
    m_gvg.setTransform(transform);
    
    // Clear out the active voxel grid
    m_gvg.setAll(Imath::Color4f(0.0f, 0.0f, 0.0f, 0.0f));
    
    // Move the stuff over, "changing resolution"
    // TODO: Make this less naieve.  Right now, when down-res'ing, you get a filled
    //       voxel if there is *anything* in its up-res'ed version.
    //       Could also take neighbors into account (round corners) when up-res'ing, etc.
    for (int x = 0; x < tempGVG.cellDimensions().x; x++)
    {
        for (int y = 0; y < tempGVG.cellDimensions().y; y++)
        {
            for (int z = 0; z < tempGVG.cellDimensions().z; z++)
            {
                const Imath::Color4f curCol = tempGVG.get(Imath::V3i(x,y,z));
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
                                m_gvg.set(lowIndex, curCol);
                            }
                        }
                    }
                }
            }
        }
    }
    
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


void GLModelWidget::initializeGL()
{
    glClearColor(m_backgroundColor.r, m_backgroundColor.g, m_backgroundColor.b, 0.0);
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
    m_cam.apply();

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (m_drawGrid)
    {
        // Shift the grid to the floor of the voxel grid
        Imath::Box3d worldBox = m_gvg.worldBounds();
        glPushMatrix();
        glTranslatef(0, worldBox.min.y, 0);
        glDrawGrid(16, m_gridColor, m_backgroundColor);
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
                    glDrawCubePoly();
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
        glColor4f(1.0f, 1.0f, 0.0f, 1.0f);
        glBegin(GL_LINES);
        glVertex3d(fakeLine.pos.x, fakeLine.pos.y, fakeLine.pos.z);
        glVertex3f(fakeLine.pos.x + fakeLine.dir.x*100, 
                   fakeLine.pos.y + fakeLine.dir.y*100, 
                   fakeLine.pos.z + fakeLine.dir.z*100);
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

    if (DEBUG_ME)
    {
        glColor4f(1.0f, 0.0f, 0.0f, 0.2f);
        glDrawSelectedVoxels();
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
                               const Imath::Color4f gridColor,
                               const Imath::Color4f bgColor)
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

    glNormal3f(-1.0f, 0.0f, 0.0f);
    glVertex3f(-0.5f, 0.5f, 0.5f);
    glVertex3f(-0.5f, 0.5f,-0.5f);
    glVertex3f(-0.5f,-0.5f,-0.5f);
    glVertex3f(-0.5f,-0.5f, 0.5f);

    glNormal3f( 1.0f, 0.0f, 0.0f);
    glVertex3f( 0.5f, 0.5f,-0.5f);
    glVertex3f( 0.5f, 0.5f, 0.5f);
    glVertex3f( 0.5f,-0.5f, 0.5f);
    glVertex3f( 0.5f,-0.5f,-0.5f);
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
                if (DEBUG_ME)
                {
                    if (std::find(m_intersects.begin(), m_intersects.end(), Imath::V3i(x,y,z)) != m_intersects.end())
                        continue;

                    if (std::find(m_intersects.begin(), m_intersects.end(), Imath::V3i(x,y-1,z)) != m_intersects.end())
                        continue;

                    if (std::find(m_intersects.begin(), m_intersects.end(), Imath::V3i(x,y,z-1)) != m_intersects.end())
                        continue;

                    if (std::find(m_intersects.begin(), m_intersects.end(), Imath::V3i(x,y-1,z-1)) != m_intersects.end())
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
                if (DEBUG_ME)
                {
                    if (std::find(m_intersects.begin(), m_intersects.end(), Imath::V3i(x,y,z)) != m_intersects.end())
                        continue;

                    if (std::find(m_intersects.begin(), m_intersects.end(), Imath::V3i(x-1,y,z)) != m_intersects.end())
                        continue;

                    if (std::find(m_intersects.begin(), m_intersects.end(), Imath::V3i(x,y,z-1)) != m_intersects.end())
                        continue;

                    if (std::find(m_intersects.begin(), m_intersects.end(), Imath::V3i(x-1,y,z-1)) != m_intersects.end())
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
                if (DEBUG_ME)
                {
                    if (std::find(m_intersects.begin(), m_intersects.end(), Imath::V3i(x,y,z)) != m_intersects.end())
                        continue;

                    if (std::find(m_intersects.begin(), m_intersects.end(), Imath::V3i(x-1,y,z)) != m_intersects.end())
                        continue;

                    if (std::find(m_intersects.begin(), m_intersects.end(), Imath::V3i(x,y-1,z)) != m_intersects.end())
                        continue;

                    if (std::find(m_intersects.begin(), m_intersects.end(), Imath::V3i(x-1,y-1,z)) != m_intersects.end())
                        continue;
                }

                if (Imath::V3i(x,y,z)   == m_activeVoxel || Imath::V3i(x-1,y,z)   == m_activeVoxel ||
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


void GLModelWidget::glDrawSelectedVoxels()
{
    Imath::Color4f curColor;
    glGetFloatv(GL_CURRENT_COLOR, (float*)&curColor);

    for (unsigned int i = 0; i < m_intersects.size(); i++)
    {
        float scalar = 1.0f - ((float)i / (float)(m_intersects.size()));
        curColor.r = curColor.r * scalar;
        curColor.g = curColor.g * scalar;
        curColor.b = curColor.b * scalar;
        
        const Imath::M44d mat = m_gvg.voxelTransform(m_intersects[i]);
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
        // TODO: Likely won't need to confine this to the "Alt Down" case
        m_lastMouse = event->pos();
    }
    else if (ctrlDown)
    {
        if (event->buttons() & Qt::LeftButton)
        {
            // CTRL+LMB is always replace
            fakeLine = m_cam.unproject(Imath::V2d(event->pos().x(), height() - event->pos().y()));
            m_intersects = m_gvg.rayIntersection(fakeLine, true);
            paintGunReplace(m_intersects, m_activeColor);
            updateGL();
        }
    }
    else
    {
        if (event->buttons() & Qt::LeftButton)
        {
            fakeLine = m_cam.unproject(Imath::V2d(event->pos().x(), height() - event->pos().y()));
            m_intersects = m_gvg.rayIntersection(fakeLine, true);

            switch (m_activeTool)
            {
                case TOOL_SPLAT: paintGunBlast(m_intersects, m_activeColor); updateGL(); break;
                case TOOL_FLOOD: paintGunFlood(m_intersects, m_activeColor); updateGL(); break;
                case TOOL_RAY: break;
                case TOOL_ERASER: paintGunDelete(m_intersects); updateGL(); break;
                case TOOL_REPLACE: paintGunReplace(m_intersects, m_activeColor); updateGL(); break;
                case TOOL_SLAB: paintGunFillSlice(m_intersects, m_activeColor); updateGL(); break;
                case TOOL_DROPPER:
                    Imath::Color4f result = colorPick(m_intersects);
                    if (result.a != 0.0f)
                        emit colorSampled(result);
                    break;
            }
        }
        else if (event->buttons() & Qt::MidButton)
        {
            // Middle button is always the color picker
            fakeLine = m_cam.unproject(Imath::V2d(event->pos().x(), height() - event->pos().y()));
            m_intersects = m_gvg.rayIntersection(fakeLine, true);
            Imath::Color4f result = colorPick(m_intersects);
            if (result.a != 0.0f)
                emit colorSampled(result);
        }
        else if (event->buttons() & Qt::RightButton)
        {
            // Right button is always delete
            fakeLine = m_cam.unproject(Imath::V2d(event->pos().x(), height() - event->pos().y()));
            m_intersects = m_gvg.rayIntersection(fakeLine, true);
            paintGunDelete(m_intersects); 
            updateGL();
        }
        return;
    }
}


void GLModelWidget::mouseMoveEvent(QMouseEvent *event)
{
    const bool altDown = event->modifiers() & Qt::AltModifier;
    //const bool ctrlDown = event->modifiers() & Qt::ControlModifier;
    
    if (altDown)
    {
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
        // Left click means shoot a ray
        if (event->buttons() & Qt::LeftButton)
        {
            fakeLine = m_cam.unproject(Imath::V2d(event->pos().x(), height() - event->pos().y()));
            m_intersects = m_gvg.rayIntersection(fakeLine, true);
            updateGL();
        }
    }
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


void GLModelWidget::rayGunBlast(const std::vector<Imath::V3i>& sortedInput, const Imath::Color4f& color)
{
    m_undoStack.beginMacro("Ray Blast");
    for (size_t i = 0; i < sortedInput.size(); i++)
    {
        setVoxelColor(sortedInput[i], color);
    }
    m_undoStack.endMacro();
}


void GLModelWidget::paintGunBlast(const std::vector<Imath::V3i>& sortedInput, const Imath::Color4f& color)
{
    for (size_t i = 0; i < sortedInput.size(); i++)
    {
        if (m_gvg.get(sortedInput[i]).a != 0.0f)
        {
            // Hit a voxel at the near edge of the grid?  Abort.
            if (i == 0) return;
            
            // Hit a voxel in the middle?  Set the previous voxel and return.
            setVoxelColor(sortedInput[i-1], color); 
            break;
        }
        
        // Didn't hit anything?  Just fill in the last voxel.
        if (i == sortedInput.size()-1)
        {
            setVoxelColor(sortedInput[i], color);
            break;
        }
    }
}


void GLModelWidget::paintGunReplace(const std::vector<Imath::V3i>& sortedInput, const Imath::Color4f& color)
{
    // Get the first voxel hit
    Imath::V3i hit(-1,-1,-1);
    for (size_t i = 0; i < sortedInput.size(); i++)
    {
        if (m_gvg.get(sortedInput[i]).a != 0.0f)
        {
            hit = sortedInput[i];
            break;
        }
    }

    // Crap out if there was no hit
    if (hit == Imath::V3i(-1,-1,-1))
        return;
    
    setVoxelColor(hit, color);
}


void GLModelWidget::paintGunFillSlice(const std::vector<Imath::V3i>& sortedInput, const Imath::Color4f& color)
{
    // TODO: Should this do an intersection at all, or maybe just fill in the row based on the first hit?
    
    // Sanity
    if (sortedInput.size() == 0)
        return;

    // Get the position to fill from
    Imath::V3i fillPos(-1,-1,-1);
    for (size_t i = 0; i < sortedInput.size(); i++)
    {
        if (m_gvg.get(sortedInput[i]).a != 0.0f)
        {
            // Hit a voxel at the near edge of the grid?  Start there.
            if (i == 0)
            {
                fillPos = sortedInput[0];
                break;
            }
            else
            {
                // Hit a voxel in the middle?
                fillPos = sortedInput[i-1];
                break;
            }
        }
    }

    // Didn't hit anything?  Just fill in the last voxel.
    if (fillPos == Imath::V3i(-1,-1,-1))
    {
        fillPos  = sortedInput[0];
    }

    // Fill
    switch (m_currAxis)
    {
        case X_AXIS:
            m_undoStack.beginMacro("Fill X Slice");
            for (int y=0; y < m_gvg.cellDimensions().y; y++)
            {
                for (int z=0; z < m_gvg.cellDimensions().z; z++)
                {
                    setVoxelColor(Imath::V3i(fillPos.x, y, z), color);
                }
            }
            m_undoStack.endMacro();
            break;
        case Y_AXIS:
            m_undoStack.beginMacro("Fill Y Slice");
            for (int x=0; x < m_gvg.cellDimensions().x; x++)
            {
                for (int z=0; z < m_gvg.cellDimensions().z; z++)
                {
                    setVoxelColor(Imath::V3i(x, fillPos.y, z), color);
                }
            }
            m_undoStack.endMacro();
            break;
        case Z_AXIS:
            m_undoStack.beginMacro("Fill Z Slice");
            for (int x=0; x < m_gvg.cellDimensions().x; x++)
            {
                for (int y=0; y < m_gvg.cellDimensions().y; y++)
                {
                    setVoxelColor(Imath::V3i(x, y, fillPos.z), color);
                }
            }
            m_undoStack.endMacro();
            break;
    }
}


void GLModelWidget::setNeighborsRecurse(const Imath::V3i& alreadySet, 
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
        if (doUs[i].x < 0 || doUs[i].x >= m_gvg.cellDimensions().x) continue;
        if (doUs[i].y < 0 || doUs[i].y >= m_gvg.cellDimensions().y) continue;
        if (doUs[i].z < 0 || doUs[i].z >= m_gvg.cellDimensions().z) continue;
        
        // Recurse
        if (m_gvg.get(doUs[i]) == repColor)
        {
            setVoxelColor(doUs[i], newColor);
            setNeighborsRecurse(doUs[i], repColor, newColor);
        }
    }
}


void GLModelWidget::paintGunFlood(const std::vector<Imath::V3i>& sortedInput, const Imath::Color4f& color)
{
    // Get the first voxel hit
    Imath::V3i hit(-1,-1,-1);
    for (size_t i = 0; i < sortedInput.size(); i++)
    {
        if (m_gvg.get(sortedInput[i]).a != 0.0f)
        {
            hit = sortedInput[i];
            break;
        }
    }
    
    // Dump out if there was no hit
    if (hit == Imath::V3i(-1,-1,-1))
        return;

    // Get the color we're replacing.
    const Imath::Color4f repColor = m_gvg.get(hit);

    // Die early if there's nothing to do
    if (repColor == color)
        return;
    
    // Recurse
    m_undoStack.beginMacro("Flood Fill");
    setVoxelColor(hit, color);
    setNeighborsRecurse(hit, repColor, color);
    m_undoStack.endMacro();
}


Imath::Color4f GLModelWidget::colorPick(const std::vector<Imath::V3i>& sortedInput)
{
    for (size_t i = 0; i < sortedInput.size(); i++)
    {
        const Imath::Color4f color = m_gvg.get(sortedInput[i]);
        if (color.a != 0.0f)
            return color;
    }
    return Imath::Color4f(0.0f, 0.0f, 0.0f, 0.0f);
}


void GLModelWidget::paintGunDelete(const std::vector<Imath::V3i>& sortedInput)
{
    for (size_t i = 0; i < sortedInput.size(); i++)
    {
        if (m_gvg.get(sortedInput[i]).a != 0.0f)
        {
            setVoxelColor(sortedInput[i], Imath::Color4f(0.0f, 0.0f, 0.0f, 0.0f));
            break;
        }
    }
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
    Imath::V3d camYVec; m_cam.transform().multDirMatrix(Imath::V3d(0.0, 1.0, 0.0), camYVec);

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

    // Transform and update - todo: functionize
    Imath::M44d transform;
    Imath::V3d dDims = m_gvg.cellDimensions();
    transform.setTranslation(Imath::V3d(-dDims.x/2.0, 0, -dDims.z/2.0));
    m_gvg.setTransform(transform);
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

    // Transform and update - todo: functionize
    Imath::M44d transform;
    Imath::V3d dDims = m_gvg.cellDimensions();
    transform.setTranslation(Imath::V3d(-dDims.x/2.0, 0, -dDims.z/2.0));
    m_gvg.setTransform(transform);
    updateGL();
    return true;
}


bool GLModelWidget::saveGridPNG(const std::string& filename)
{
    // TODO: Offer other options besides XY slices?  Directionality?  Ordering?
    const int height = m_gvg.cellDimensions().y;
    const int width = m_gvg.cellDimensions().x * m_gvg.cellDimensions().z;
    QImage writeMe(QSize(width, height), QImage::QImage::Format_ARGB32);

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
    writeMe.setText("VoxelGridDimX", tempStr.setNum(m_gvg.cellDimensions().x));
    writeMe.setText("VoxelGridDimY", tempStr.setNum(m_gvg.cellDimensions().y));
    writeMe.setText("VoxelGridDimZ", tempStr.setNum(m_gvg.cellDimensions().z));

    return writeMe.save(QString(filename.c_str()));
}


void GLModelWidget::shiftVoxels(const Axis axis, const bool up, const bool wrap)
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


    m_undoStack.beginMacro("Shift");

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

    m_undoStack.endMacro();
    updateGL();
}


void GLModelWidget::setVoxelColor(const Imath::V3i& index, const Imath::Color4f color)
{
    // Validity check
    const Imath::V3i& cd = m_gvg.cellDimensions();
    if (index.x < 0     || index.y < 0     || index.z < 0 ||
        index.x >= cd.x || index.y >= cd.y || index.z >= cd.z)
        return;
    
    m_undoStack.push(new CmdSetVoxelColor(&m_gvg, index, color));
}
