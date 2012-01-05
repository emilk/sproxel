#include <Python.h>
#include <QtDebug>
#include <QFileInfo>
#include <QDir>
#include <QMessageBox>
#include "MainWindow.h"
#include "script.h"
#include "pyConsole.h"


extern void init_sproxel_bindings();


#define GLUE(a, b) a##b


//ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//


QDir exe_dir;

static PyObject *glue=NULL;

PyObject *py_save_project=NULL, *py_load_project=NULL;


//ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//


#define TOPYT(cls) \
  static PyObject * GLUE(cls, _toPy_py) = NULL; \
  PyObject * GLUE(cls, _toPy) (class cls *p) \
  { \
    if (!p) { Py_RETURN_NONE; } \
    if (!GLUE(cls, _toPy_py)) { PyErr_SetString(PyExc_StandardError, "No " #cls "_toPy in SproxelGlue"); return NULL; } \
    PyObject *c=PyCapsule_New(p, NULL, NULL); if (!c) return NULL; \
    PyObject *w=PyObject_CallFunctionObjArgs(GLUE(cls, _toPy_py), c, NULL); \
    Py_DECREF(c); \
    return w; \
  }


#define TOCPP(cls) \
  static PyObject * GLUE(cls, _toCpp_py) = NULL; \
  class cls * GLUE(cls, _toCpp) (PyObject *w) \
  { \
    if (!w) { PyErr_SetString(PyExc_StandardError, "NULL PyObject in " #cls "_toCpp"); return NULL; } \
    if (w==Py_None) return NULL; \
    if (!GLUE(cls, _toCpp_py)) { PyErr_SetString(PyExc_StandardError, "No " #cls "_toCpp in SproxelGlue"); return NULL; } \
    PyObject *c=PyObject_CallFunctionObjArgs(GLUE(cls, _toCpp_py), w, NULL); if (!c) return NULL; \
    void *p=PyCapsule_GetPointer(c, NULL); \
    Py_DECREF(c); \
    return (class cls*)p; \
  }


#include "glue/classGlue.h"


//ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//


void init_script(const char *exe_path)
{
  exe_dir=QFileInfo(exe_path).absoluteDir();

  qDebug() << "prog name:" << exe_path;
  Py_SetProgramName((char*)exe_path);

  qDebug() << "init py...";
  Py_Initialize();
  qDebug() << "init py ok";

  init_python_console();
  init_sproxel_bindings();

  pycon("Importing PySide.SproxelGlue...");
  glue=PyImport_ImportModule("PySide.SproxelGlue");
  if (!glue)
  {
    pycon("Failed to import PySide.SproxelGlue");
    QMessageBox::critical(NULL, "Sproxel Error", "Failed to import PySide.SproxelGlue");
  }
  else
  {
    pycon("Imported PySide.SproxelGlue, getting methods...");

    bool gotErrors=false;

    #define TOPYT(cls) \
      GLUE(cls, _toPy_py ) = PyObject_GetAttrString(glue, #cls "_toPy" ); \
      if (PyErr_Occurred()) { PyErr_Print(); gotErrors=true; }

    #define TOCPP(cls) \
      GLUE(cls, _toCpp_py) = PyObject_GetAttrString(glue, #cls "_toCpp"); \
      if (PyErr_Occurred()) { PyErr_Print(); gotErrors=true; }

    #include "glue/classGlue.h"

    pycon("Glue methods: %s", gotErrors ? "some missing" : "all OK");
    if (gotErrors) QMessageBox::critical(NULL, "Sproxel Error", "Some PySide.SproxelGlue methods are missing.");
  }

  PyObject *mod=PyImport_ImportModule("sproxel_utils");
  if (!mod)
  {
    PyErr_Print();
    QMessageBox::critical(NULL, "Sproxel Error", "Failed to import sproxel_utils");
  }
  else
  {
    // check required methods
    bool gotErrors=false;

    py_save_project=PyObject_GetAttrString(mod, "save_project");
    if (PyErr_Occurred()) { PyErr_Print(); gotErrors=true; }

    py_load_project=PyObject_GetAttrString(mod, "load_project");
    if (PyErr_Occurred()) { PyErr_Print(); gotErrors=true; }

    pycon("Scripted methods: %s", gotErrors ? "some missing" : "all OK");
    if (gotErrors) QMessageBox::critical(NULL, "Sproxel Error", "Some scripted methods are missing.");

    Py_DECREF(mod); mod=NULL;
  }
}


void close_script()
{
  close_python_console();

  Py_XDECREF(py_save_project); py_save_project=NULL;
  Py_XDECREF(py_load_project); py_load_project=NULL;

  #define TOPYT(cls) Py_XDECREF(GLUE(cls, _toPy_py )); GLUE(cls, _toPy_py ) = NULL;
  #define TOCPP(cls) Py_XDECREF(GLUE(cls, _toCpp_py)); GLUE(cls, _toCpp_py) = NULL;
  #include "glue/classGlue.h"

  Py_XDECREF(glue); glue=NULL;

  qDebug() << "fin py...";
  Py_Finalize();
  qDebug() << "fin py ok";
}


//ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//


void script_set_main_window(MainWindow *mwin)
{
  pycon("Adding Sproxel objects to script...");

  PyObject *mod=PyImport_AddModule("__main__");
  PyObject *o;
  bool gotErrors=false;

  o=QMainWindow_toPy(mwin);
  if (!o) { gotErrors=true; PyErr_Print(); o=Py_None; Py_INCREF(o); }
  PyModule_AddObject(mod, "main_window", o);

  //== TODO: add more components to script

  pycon("Sproxel objects: %s", gotErrors ? "FAILED to add to script" : "all added");
}


//ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//


bool run_script(const QString &fn)
{
  QString filename=exe_dir.filePath(fn);
  FILE *file=_wfopen((wchar_t*)filename.unicode(), L"r");
  if (file)
  {
    pycon("Starting script %S", filename.unicode());

    PyObject *mod=PyImport_AddModule("__main__");
    PyObject *modDict=PyModule_GetDict(mod);

    PyObject *o=PyRun_File(file, qPrintable(filename), Py_file_input, modDict, modDict);
    fclose(file);
    Py_XDECREF(o);

    if (PyErr_Occurred())
    {
      PyErr_Print();
      return false;
    }

    return true;
  }
  else
  {
    pycon("Failed to open script file %S", filename.unicode());
    return false;
  }
}
