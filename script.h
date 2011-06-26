#ifndef __SPROXEL_SCRIPT_H__
#define __SPROXEL_SCRIPT_H__


#include <Python.h>


void init_script(char *exe_path);
void close_script();

void script_set_main_window(class MainWindow *);

bool run_script(const class QString &filename);


#define GLUE(a, b) a##b
#define TOPYT(cls) PyObject * GLUE(cls, _toPy ) (class cls *);
#define TOCPP(cls) class cls* GLUE(cls, _toCpp) (PyObject *);
#include "glue/classGlue.h"
#undef GLUE


extern class QDir exe_dir;


#endif
