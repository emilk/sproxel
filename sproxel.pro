#-------------------------------------------------
#
# SPROXEL sprite-ish voxel editor
#
#-------------------------------------------------

QT       += core gui opengl

TARGET = sproxel
TEMPLATE = app

unix:!macx {
  CONFIG += link_pkgconfig
  PKGCONFIG += IlmBase
}

macx {
  INCLUDEPATH += /usr/local/include/OpenEXR
  LIBS += -lImath -lIex
}

win32 {
  INCLUDEPATH += ../IlmBase/include
  CONFIG(release) {
    LIBS += -L../IlmBase/lib/Release
  } else {
    LIBS += -L../IlmBase/lib/Debug
  }
  LIBS += -lImath -lIex
  DEFINES += NOMINMAX
  QMAKE_CXXFLAGS += -wd4996 -wd4305
}

SOURCES += \
    GLCamera.cpp \
    GLModelWidget.cpp \
    MainWindow.cpp \
    main.cpp \
    NewGridDialog.cpp \
    PreferencesDialog.cpp \
    PaletteWidget.cpp

HEADERS  += \
    Foundation.h \
    GLCamera.h \
    GLModelWidget.h \
    GameVoxelGrid.h \
    MainWindow.h \
    NewGridDialog.h \
    PreferencesDialog.h \
    PaletteWidget.h

FORMS += \
    NewGridDialog.ui

RESOURCES += \
    sproxel.qrc
