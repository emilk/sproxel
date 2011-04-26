#-------------------------------------------------
#
# SPROXEL sprite-ish voxel editor
#
#-------------------------------------------------

QT       += core gui opengl

TARGET = sproxel
TEMPLATE = app

INCLUDEPATH += /usr/local/include/OpenEXR
LIBS += -lImath -lIex

SOURCES += \
    GLCamera.cpp \
    GLModelWidget.cpp \
    MainWindow.cpp \
    main.cpp

HEADERS  += \
    Foundation.h \
    GLCamera.h \
    GLModelWidget.h \
    GameVoxelGrid.h \
    MainWindow.h
