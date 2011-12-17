#include <Python.h>
#include <QImage>
#include <QBuffer>
#include "pyConsole.h"
#include "script.h"
#include "VoxelGridGroup.h"
#include "SproxelProject.h"
#include "MainWindow.h"


//ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//


PyObject* qstr_to_py(const QString &str)
{
  return PyUnicode_FromUnicode((const Py_UNICODE*)str.constData(), str.length());
}


bool py_to_qstr(PyObject *o, QString &str)
{
  PyObject *u=NULL;

  if (!PyUnicode_Check(o))
  {
    u=PyUnicode_FromObject(o);
    if (!u) return false;
    o=u;
  }

  str.setUtf16(PyUnicode_AsUnicode(o), PyUnicode_GetSize(o));

  Py_XDECREF(u);
  return true;
}


bool py_to_color(PyObject *o, SproxelColor &c)
{
  if (PyInt_Check(o) || PyLong_Check(o))
  {
    unsigned i=PyInt_AsUnsignedLongMask(o);
    if (PyErr_Occurred())
    {
      PyErr_Clear();
      i=PyLong_AsUnsignedLongMask(o);
      if (PyErr_Occurred()) return false;
    }
    if ((i&0xFF000000)==0 && i!=0) i|=0xFF000000;
    c.a=((i>>24)&0xFF)/255.0f;
    c.r=((i>>16)&0xFF)/255.0f;
    c.g=((i>> 8)&0xFF)/255.0f;
    c.b=((i    )&0xFF)/255.0f;
    return true;
  }

  c.a=1;
  if (!PyArg_ParseTuple(o, "fff|f", &c.r, &c.g, &c.b, &c.a)) return false;
  return true;
}


//  Layer  ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//


extern PyTypeObject sproxelPyLayerType;


struct PyLayer
{
  PyObject_HEAD
  VoxelGridLayerPtr layer;
};


static void PyLayer_dtor(PyLayer *self)
{
  self->layer=NULL;
  self->ob_type->tp_free((PyObject*)self);
}


static int PyLayer_init(PyLayer *self, PyObject *args, PyObject *kwds)
{
  if (self->layer)
  {
    PyErr_SetString(PyExc_TypeError, "Layer is already initialized");
    return -1;
  }

  // process args
  VoxelGridLayerPtr newLayer;

  static char* fromPar[]={"layer", NULL};
  static char* defPar[]={"size", "offset", "name", NULL};

  PyLayer *fromLayer=NULL;
  int sizeX=0, sizeY=0, sizeZ=0, ofsX=0, ofsY=0, ofsZ=0;
  const char *name=NULL;

  if (PyArg_ParseTupleAndKeywords(args, kwds, "O!", fromPar, &sproxelPyLayerType, &fromLayer))
  {
    if (fromLayer->layer)
      newLayer=new VoxelGridLayer(*fromLayer->layer);
    else
      newLayer=new VoxelGridLayer();
  }
  else if (PyErr_Clear(), PyArg_ParseTupleAndKeywords(args, kwds, "|(iii)(iii)s", defPar,
    &sizeX, &sizeY, &sizeZ, &ofsX, &ofsY, &ofsZ, &name))
  {
    newLayer=new VoxelGridLayer();

    if (sizeX>0 && sizeY>0 && sizeZ>0)
      newLayer->resize(Imath::Box3i(Imath::V3i(0), Imath::V3i(sizeX-1, sizeY-1, sizeZ-1)));

    newLayer->setOffset(Imath::V3i(ofsX, ofsY, ofsZ));
    if (name) newLayer->setName(name);
  }
  else
    return -1;

  self->layer=newLayer;

  return 0;
}


#define CHECK_PYLAYER \
  if (!self->layer) { PyErr_SetString(PyExc_TypeError, "NULL Layer"); return NULL; }

#define CHECK_PYLAYER_S \
  if (!self->layer) { PyErr_SetString(PyExc_TypeError, "NULL Layer"); return -1; }


static PyObject* PyLayer_getOffset(PyLayer *self, void*)
{
  CHECK_PYLAYER
  const Imath::V3i &o=self->layer->offset();
  return Py_BuildValue("(iii)", o.x, o.y, o.z);
}


static int PyLayer_setOffset(PyLayer *self, PyObject *value, void*)
{
  CHECK_PYLAYER_S
  Imath::V3i o;
  if (!PyArg_ParseTuple(value, "iii", &o.x, &o.y, &o.z)) return -1;
  self->layer->setOffset(o);
  return 0;
}


static PyObject* PyLayer_getVisible(PyLayer *self, void*)
{
  CHECK_PYLAYER
  return PyBool_FromLong(self->layer->isVisible());
}


static int PyLayer_setVisible(PyLayer *self, PyObject *value, void*)
{
  CHECK_PYLAYER_S
  self->layer->setVisible(PyObject_IsTrue(value));
  return 0;
}


static PyObject* PyLayer_getName(PyLayer *self, void*)
{
  CHECK_PYLAYER
  return qstr_to_py(self->layer->name());
}


static int PyLayer_setName(PyLayer *self, PyObject *value, void*)
{
  CHECK_PYLAYER_S

  QString str;
  if (!py_to_qstr(value, str)) return -1;

  self->layer->setName(str);
  return 0;
}


static PyObject* PyLayer_getSize(PyLayer *self, void*)
{
  CHECK_PYLAYER
  const Imath::V3i s=self->layer->size();
  return Py_BuildValue("(iii)", s.x, s.y, s.z);
}


static PyObject* PyLayer_getBounds(PyLayer *self, void*)
{
  CHECK_PYLAYER
  const Imath::Box3i b=self->layer->bounds();
  return Py_BuildValue("((iii)(iii))", b.min.x, b.min.y, b.min.z, b.max.x, b.max.y, b.max.z);
}


static PyObject* PyLayer_getDataType(PyLayer *self, void*)
{
  CHECK_PYLAYER
  const char *s;
  switch (self->layer->dataType())
  {
    case VoxelGridLayer::TYPE_IND: s="IND"; break;
    case VoxelGridLayer::TYPE_RGB: s="RGB"; break;
    default: s="UNK";
  }
  return PyString_FromString(s);
}


static PyGetSetDef pyLayer_getsets[]=
{
  {"offset", (getter)PyLayer_getOffset, (setter)PyLayer_setOffset, "Grid offset", NULL},
  {"visible", (getter)PyLayer_getVisible, (setter)PyLayer_setVisible, "Visibility", NULL},
  {"name", (getter)PyLayer_getName, (setter)PyLayer_setName, "Layer name", NULL},
  //== palette
  {"size", (getter)PyLayer_getSize, NULL, "Grid size", NULL},
  {"bounds", (getter)PyLayer_getBounds, NULL, "Grid bounds", NULL},
  {"dataType", (getter)PyLayer_getDataType, NULL, "Grid data type", NULL},
  {NULL, NULL, NULL, NULL, NULL}
};


//ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//


static PyObject* PyLayer_reset(PyLayer *self)
{
  CHECK_PYLAYER
  self->layer->clear();
  Py_RETURN_NONE;
}


static PyObject* PyLayer_resize(PyLayer *self, PyObject *args)
{
  CHECK_PYLAYER
  Imath::Box3i b;
  if (!PyArg_ParseTuple(args, "((iii)(iii))",
    &b.min.x, &b.min.y, &b.min.z, &b.max.x, &b.max.y, &b.max.z)) return NULL;
  if (!b.isEmpty()) self->layer->resize(b);
  Py_RETURN_NONE;
}


static PyObject* PyLayer_getInd(PyLayer *self, PyObject *args)
{
  CHECK_PYLAYER
  Imath::V3i p;
  if (!PyArg_ParseTuple(args, "(iii)", &p.x, &p.y, &p.z))
  {
    PyErr_Clear();
    if (!PyArg_ParseTuple(args, "iii", &p.x, &p.y, &p.z)) return NULL;
  }
  return PyInt_FromLong(self->layer->getInd(p));
}


static PyObject* PyLayer_getColor(PyLayer *self, PyObject *args)
{
  CHECK_PYLAYER
  Imath::V3i p;
  if (!PyArg_ParseTuple(args, "(iii)", &p.x, &p.y, &p.z))
  {
    PyErr_Clear();
    if (!PyArg_ParseTuple(args, "iii", &p.x, &p.y, &p.z)) return NULL;
  }
  SproxelColor c=self->layer->getColor(p);
  return Py_BuildValue("(ffff)", c.r, c.g, c.b, c.a);
}


static PyObject* PyLayer_set(PyLayer *self, PyObject *args, PyObject *kwds)
{
  CHECK_PYLAYER

  static char *vecPar[]={"at", "color", "index", NULL};
  static char *xyzPar[]={"x", "y", "z", "color", "index", NULL};

  Imath::V3i p;
  SproxelColor c(0, 0, 0, 0);
  int i=-1;

  PyObject *cobj=NULL;

  if (!PyArg_ParseTupleAndKeywords(args, kwds, "(iii)|Oi", vecPar,
    &p.x, &p.y, &p.z, &cobj, &i))
  {
    PyErr_Clear();
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "iii|Oi", xyzPar,
      &p.x, &p.y, &p.z, &cobj, &i)) return NULL;
  }

  if (cobj) if (!py_to_color(cobj, c)) return NULL;

  self->layer->set(p, c, i);
  Py_RETURN_NONE;
}


static PyObject* PyLayer_toPNG(PyLayer *self)
{
  CHECK_PYLAYER
  QImage image=self->layer->makeQImage();
  QBuffer buf;
  buf.open(QIODevice::WriteOnly);
  if (!image.save(&buf, "PNG")) Py_RETURN_NONE;

  PyObject *str=PyString_FromStringAndSize(buf.buffer().data(), buf.buffer().size());
  if (!str) return PyErr_NoMemory();
  return str;
}


static PyMethodDef pyLayer_methods[]=
{
  { "reset", (PyCFunction)PyLayer_reset, METH_NOARGS, "Reset layer to the default blank state." },
  { "resize", (PyCFunction)PyLayer_resize, METH_VARARGS, "Resize layer to the specified bounds." },
  { "getIndex", (PyCFunction)PyLayer_getInd, METH_VARARGS, "Get index value of the specified voxel. Returns -1 for RGB grid." },
  { "getColor", (PyCFunction)PyLayer_getColor, METH_VARARGS, "Get color value of the specified voxel." },
  { "set", (PyCFunction)PyLayer_set, METH_VARARGS|METH_KEYWORDS,
      "Set color and/or index value of the specified voxel. Will expand grid if necessary." },
  { "toPNG", (PyCFunction)PyLayer_toPNG, METH_NOARGS, "Save layer as PNG with text fields and return PNG data as string." },
  { NULL, NULL, 0, NULL }
};


static PyObject* PyLayer_repr(PyLayer *self)
{
  CHECK_PYLAYER
  return PyString_FromFormat("Layer(\"%s\")", self->layer->name().toUtf8().data());
}


static int PyLayer_cmp(PyLayer *self, PyObject *o)
{
  if (o==Py_None && !self->layer) return 0;
  if (!PyObject_TypeCheck(o, &sproxelPyLayerType)) return -1;
  PyLayer *other=(PyLayer*)o;
  if (self->layer==other->layer) return 0;
  if (self->layer.constData()<other->layer.constData()) return -1;
  return 1;
}


PyTypeObject sproxelPyLayerType=
{
  PyObject_HEAD_INIT(NULL)
  0,                         /*ob_size*/
  "sproxel.Layer",           /*tp_name*/
  sizeof(PyLayer),           /*tp_basicsize*/
  0,                         /*tp_itemsize*/
  (destructor)PyLayer_dtor,  /*tp_dealloc*/
  0,                         /*tp_print*/
  0,                         /*tp_getattr*/
  0,                         /*tp_setattr*/
  (cmpfunc)PyLayer_cmp,      /*tp_compare*/
  (reprfunc)PyLayer_repr,    /*tp_repr*/
  0,                         /*tp_as_number*/
  0,                         /*tp_as_sequence*/
  0,                         /*tp_as_mapping*/
  0,                         /*tp_hash */
  0,                         /*tp_call*/
  0,                         /*tp_str*/
  0,                         /*tp_getattro*/
  0,                         /*tp_setattro*/
  0,                         /*tp_as_buffer*/
  Py_TPFLAGS_DEFAULT,        /*tp_flags*/
  "Sproxel layer",           /* tp_doc */
  0,                           /* tp_traverse */
  0,                           /* tp_clear */
  0,                           /* tp_richcompare */
  0,                           /* tp_weaklistoffset */
  0,                           /* tp_iter */
  0,                           /* tp_iternext */
  pyLayer_methods,           /* tp_methods */
  0,                         /* tp_members */
  pyLayer_getsets,           /* tp_getset */
  0,                         /* tp_base */
  0,                         /* tp_dict */
  0,                         /* tp_descr_get */
  0,                         /* tp_descr_set */
  0,                         /* tp_dictoffset */
  (initproc)PyLayer_init,    /* tp_init */
  0,                         /* tp_alloc */
  0,                         /* tp_new */
};


//ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//


static PyObject* layer_to_py(VoxelGridLayerPtr layer)
{
  if (!layer) Py_RETURN_NONE;
  PyLayer *pyl=PyObject_New(PyLayer, &sproxelPyLayerType);
  if (!pyl) return PyErr_NoMemory();
  *((void**)&pyl->layer)=NULL; // reset memory
  pyl->layer=layer;
  return (PyObject*)pyl;
}


//  Sprite  ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//


extern PyTypeObject sproxelPySpriteType;


struct PySprite
{
  PyObject_HEAD
  VoxelGridGroupPtr spr;
};


static void PySprite_dtor(PySprite *self)
{
  self->spr=NULL;
  self->ob_type->tp_free((PyObject*)self);
}


static int PySprite_init(PySprite *self, PyObject *args, PyObject *kwds)
{
  if (self->spr)
  {
    PyErr_SetString(PyExc_TypeError, "Sprite is already initialized");
    return -1;
  }

  // process args
  VoxelGridGroupPtr newSpr;

  static char *par[]={"from", NULL};
  PyObject *o=NULL;
  if (!PyArg_ParseTupleAndKeywords(args, kwds, "|O", par, &o)) return -1;

  if (o)
  {
    if (PyObject_TypeCheck(o, &sproxelPySpriteType))
    {
      PySprite *pys=(PySprite*)o;
      if (pys->spr)
        newSpr=new VoxelGridGroup(*pys->spr);
      else
        newSpr=new VoxelGridGroup();
    }
    else if (PyObject_TypeCheck(o, &sproxelPyLayerType))
    {
      PyLayer *pyl=(PyLayer*)o;
      newSpr=new VoxelGridGroup(pyl->layer.data());
    }
    else
    {
      PyErr_SetString(PyExc_TypeError, "Expected Layer or Sprite as an argument");
      return -1;
    }
  }
  else
    newSpr=new VoxelGridGroup();

  self->spr=newSpr;

  return 0;
}


#define CHECK_PYSPR \
  if (!self->spr) { PyErr_SetString(PyExc_TypeError, "NULL Sprite"); return NULL; }

#define CHECK_PYSPR_S \
  if (!self->spr) { PyErr_SetString(PyExc_TypeError, "NULL Sprite"); return -1; }


static PyObject* PySprite_getCurLayerIndex(PySprite *self, void*)
{
  CHECK_PYSPR
  return PyInt_FromLong(self->spr->curLayerIndex());
}


static int PySprite_setCurLayerIndex(PySprite *self, PyObject *value, void*)
{
  CHECK_PYSPR_S
  long i=PyInt_AsLong(value);
  if (PyErr_Occurred()) return -1;
  self->spr->setCurLayer(i);
  return 0;
}


static PyObject* PySprite_getCurLayer(PySprite *self, void*)
{
  CHECK_PYSPR
  return layer_to_py(self->spr->curLayer());
}


static PyObject* PySprite_getBounds(PySprite *self, void*)
{
  CHECK_PYSPR
  const Imath::Box3i b=self->spr->bounds();
  return Py_BuildValue("((iii)(iii))", b.min.x, b.min.y, b.min.z, b.max.x, b.max.y, b.max.z);
}


static PyObject* PySprite_getLayers(PySprite *self, void*)
{
  CHECK_PYSPR
  PyObject *list=PyList_New(self->spr->numLayers());
  if (!list) return PyErr_NoMemory();

  for (int i=0; i<self->spr->numLayers(); ++i)
    PyList_SetItem(list, i, layer_to_py(self->spr->layer(i)));

  return list;
}


static PyObject* PySprite_getNumLayers(PySprite *self, void*)
{
  CHECK_PYSPR
  return PyInt_FromLong(self->spr->numLayers());
}


static PyObject* PySprite_getName(PySprite *self, void*)
{
  CHECK_PYSPR
  return qstr_to_py(self->spr->name());
}


static int PySprite_setName(PySprite *self, PyObject *value, void*)
{
  CHECK_PYSPR_S

  QString str;
  if (!py_to_qstr(value, str)) return -1;

  self->spr->setName(str);
  return 0;
}


static PyGetSetDef pySprite_getsets[]=
{
  //== TODO: transform
  {"curLayerIndex", (getter)PySprite_getCurLayerIndex, (setter)PySprite_setCurLayerIndex, "Sprite's current layer index", NULL},
  {"curLayer", (getter)PySprite_getCurLayer, NULL, "Sprite's current layer", NULL},
  {"layers", (getter)PySprite_getLayers, NULL, "List of all sprite's layers", NULL},
  {"bounds", (getter)PySprite_getBounds, NULL, "Common bounds of all sprite layers", NULL},
  {"numLayers", (getter)PySprite_getNumLayers, NULL, "Number of layers in the sprite", NULL},
  {"name", (getter)PySprite_getName, (setter)PySprite_setName, "Sprite name", NULL},
  {NULL, NULL, NULL, NULL, NULL}
};


//ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//


static PyObject* PySprite_reset(PySprite *self)
{
  CHECK_PYSPR
  self->spr->clear();
  Py_RETURN_NONE;
}


static int py_to_layer_index(PyObject *o, VoxelGridGroup &spr)
{
  if (!o) return -1;

  if (PyInt_Check(o)) return PyInt_AsLong(o);
  else if (PyLong_Check(o)) return PyLong_AsLong(o);
  else
  {
    QString name;
    if (!py_to_qstr(o, name)) return -1;

    for (int i=0; i<spr.numLayers(); ++i)
      if (spr.layer(i)->name()==name) return i;

    return -1;
  }
}


static PyObject* PySprite_layer(PySprite *self, PyObject *arg)
{
  CHECK_PYSPR
  int i=py_to_layer_index(arg, *self->spr);
  if (PyErr_Occurred()) return NULL;
  return layer_to_py(self->spr->layer(i));
}


static PyObject* PySprite_insertLayerAbove(PySprite *self, PyObject *args)
{
  CHECK_PYSPR
  PyObject *iobj=NULL;
  PyLayer *pyl=NULL;
  if (!PyArg_ParseTuple(args, "O|O!", &iobj, &sproxelPyLayerType, &pyl)) return NULL;

  int i=py_to_layer_index(iobj, *self->spr);
  if (PyErr_Occurred()) return NULL;

  return layer_to_py(self->spr->insertLayerAbove(i, pyl?pyl->layer:VoxelGridLayerPtr()));
}


static PyObject* PySprite_removeLayer(PySprite *self, PyObject *arg)
{
  CHECK_PYSPR
  int i=py_to_layer_index(arg, *self->spr);
  if (PyErr_Occurred()) return NULL;
  return layer_to_py(self->spr->removeLayer(i));
}


static PyObject* PySprite_getIndex(PySprite *self, PyObject *args)
{
  CHECK_PYSPR
  Imath::V3i p;
  if (!PyArg_ParseTuple(args, "(iii)", &p.x, &p.y, &p.z))
  {
    PyErr_Clear();
    if (!PyArg_ParseTuple(args, "iii", &p.x, &p.y, &p.z)) return NULL;
  }
  return PyInt_FromLong(self->spr->getInd(p));
}


static PyObject* PySprite_getColor(PySprite *self, PyObject *args)
{
  CHECK_PYSPR
  Imath::V3i p;
  if (!PyArg_ParseTuple(args, "(iii)", &p.x, &p.y, &p.z))
  {
    PyErr_Clear();
    if (!PyArg_ParseTuple(args, "iii", &p.x, &p.y, &p.z)) return NULL;
  }
  SproxelColor c=self->spr->get(p);
  return Py_BuildValue("(ffff)", c.r, c.g, c.b, c.a);
}


static PyObject* PySprite_set(PySprite *self, PyObject *args, PyObject *kwds)
{
  CHECK_PYSPR

  static char *vecPar[]={"at", "color", "index", NULL};
  static char *xyzPar[]={"x", "y", "z", "color", "index", NULL};

  Imath::V3i p;
  SproxelColor c(0, 0, 0, 0);
  int i=-1;

  PyObject *cobj=NULL;

  if (!PyArg_ParseTupleAndKeywords(args, kwds, "(iii)|Oi", vecPar,
    &p.x, &p.y, &p.z, &cobj, &i))
  {
    PyErr_Clear();
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "iii|Oi", xyzPar,
      &p.x, &p.y, &p.z, &cobj, &i)) return NULL;
  }

  if (cobj) if (!py_to_color(cobj, c)) return NULL;

  self->spr->set(p, c, i);
  Py_RETURN_NONE;
}


static PyObject* PySprite_traceRay(PySprite *self, PyObject *args)
{
  CHECK_PYSPR
  Imath::Line3d r;
  if (!PyArg_ParseTuple(args, "((ddd)(ddd))",
    &r.pos.x, &r.pos.y, &r.pos.z, &r.dir.x, &r.dir.y, &r.dir.z))
  {
    PyErr_Clear();
    if (!PyArg_ParseTuple(args, "(ddd)(ddd)",
      &r.pos.x, &r.pos.y, &r.pos.z, &r.dir.x, &r.dir.y, &r.dir.z))
    {
      PyErr_Clear();
      if (!PyArg_ParseTuple(args, "dddddd",
        &r.pos.x, &r.pos.y, &r.pos.z, &r.dir.x, &r.dir.y, &r.dir.z)) return NULL;
    }
  }

  std::vector<Imath::V3i> list=self->spr->rayIntersection(r);

  PyObject *tuple=PyTuple_New(list.size());
  if (!tuple) return PyErr_NoMemory();

  for (size_t i=0; i<list.size(); ++i)
  {
    const Imath::V3i &v=list[i];
    PyTuple_SET_ITEM(tuple, i, Py_BuildValue("(iii)", v.x, v.y, v.z));
  }

  return tuple;
}


static PyMethodDef pySprite_methods[]=
{
  { "reset", (PyCFunction)PySprite_reset, METH_NOARGS, "Reset sprite to the default empty state." },
  { "layer", (PyCFunction)PySprite_layer, METH_O, "Get layer by index or name." },
  { "insertLayerAbove", (PyCFunction)PySprite_insertLayerAbove, METH_VARARGS, "Add new layer to the sprite." },
  { "removeLayer", (PyCFunction)PySprite_removeLayer, METH_O, "Remove layer from the sprite and return it." },
  { "getIndex", (PyCFunction)PySprite_getIndex, METH_VARARGS, "Get index value of the specified voxel." },
  { "getColor", (PyCFunction)PySprite_getColor, METH_VARARGS, "Get color value of the specified voxel." },
  { "set", (PyCFunction)PySprite_set, METH_VARARGS|METH_KEYWORDS,
    "Set color and/or index value of the specified voxel in the current layer." },
  { "traceRay", (PyCFunction)PySprite_traceRay, METH_VARARGS, "Trace ray and return tuple of affected grid cells." },
  { NULL, NULL, 0, NULL }
};


static PyObject* PySprite_repr(PySprite *self)
{
  CHECK_PYSPR
  Imath::V3i s=self->spr->bounds().size()+Imath::V3i(1);
  return PyString_FromFormat("Sprite(\"%s\" %dx%dx%d)", self->spr->name().toUtf8().data(),
    s.x, s.y, s.z);
}


static int PySprite_cmp(PySprite *self, PyObject *o)
{
  if (o==Py_None && !self->spr) return 0;
  if (!PyObject_TypeCheck(o, &sproxelPySpriteType)) return -1;
  PySprite *other=(PySprite*)o;
  if (self->spr==other->spr) return 0;
  if (self->spr.constData()<other->spr.constData()) return -1;
  return 1;
}


PyTypeObject sproxelPySpriteType=
{
  PyObject_HEAD_INIT(NULL)
  0,                         /*ob_size*/
  "sproxel.Sprite",          /*tp_name*/
  sizeof(PySprite),          /*tp_basicsize*/
  0,                         /*tp_itemsize*/
  (destructor)PySprite_dtor, /*tp_dealloc*/
  0,                         /*tp_print*/
  0,                         /*tp_getattr*/
  0,                         /*tp_setattr*/
  (cmpfunc)PySprite_cmp,     /*tp_compare*/
  (reprfunc)PySprite_repr,   /*tp_repr*/
  0,                         /*tp_as_number*/
  0,                         /*tp_as_sequence*/
  0,                         /*tp_as_mapping*/
  0,                         /*tp_hash */
  0,                         /*tp_call*/
  0,                         /*tp_str*/
  0,                         /*tp_getattro*/
  0,                         /*tp_setattro*/
  0,                         /*tp_as_buffer*/
  Py_TPFLAGS_DEFAULT,        /*tp_flags*/
  "Sproxel sprite",          /* tp_doc */
  0,                           /* tp_traverse */
  0,                           /* tp_clear */
  0,                           /* tp_richcompare */
  0,                           /* tp_weaklistoffset */
  0,                           /* tp_iter */
  0,                           /* tp_iternext */
  pySprite_methods,          /* tp_methods */
  0,                         /* tp_members */
  pySprite_getsets,          /* tp_getset */
  0,                         /* tp_base */
  0,                         /* tp_dict */
  0,                         /* tp_descr_get */
  0,                         /* tp_descr_set */
  0,                         /* tp_dictoffset */
  (initproc)PySprite_init,   /* tp_init */
  0,                         /* tp_alloc */
  0,                         /* tp_new */
};


//ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//


static PyObject* sprite_to_py(VoxelGridGroupPtr sprite)
{
  if (!sprite) Py_RETURN_NONE;
  PySprite *pys=PyObject_New(PySprite, &sproxelPySpriteType);
  if (!pys) return PyErr_NoMemory();
  *((void**)&pys->spr)=NULL; // reset memory
  pys->spr=sprite;
  return (PyObject*)pys;
}


//  SproxelProject  ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//


extern PyTypeObject sproxelPyProjectType;


struct PyProject
{
  PyObject_HEAD
  SproxelProjectPtr proj;
};


static void PyProject_dtor(PyProject *self)
{
  self->proj=NULL;
  self->ob_type->tp_free((PyObject*)self);
}


static int PyProject_init(PyProject *self, PyObject *args, PyObject *kwds)
{
  if (self->proj)
  {
    PyErr_SetString(PyExc_TypeError, "Project is already initialized");
    return -1;
  }

  // process args
  SproxelProjectPtr newProj;

  static char *par[]={"from", NULL};
  PyProject *o=NULL;
  if (!PyArg_ParseTupleAndKeywords(args, kwds, "|O!", par, &sproxelPyProjectType, &o)) return -1;

  if (o && o->proj)
    newProj=new SproxelProject(*o->proj);
  else
    newProj=new SproxelProject();

  self->proj=newProj;

  return 0;
}


#define CHECK_PYPROJ \
  if (!self->proj) { PyErr_SetString(PyExc_TypeError, "NULL Project"); return NULL; }

#define CHECK_PYPROJ_S \
  if (!self->proj) { PyErr_SetString(PyExc_TypeError, "NULL Project"); return -1; }


static PyObject* PyProject_getSprites(PyProject *self, void*)
{
  CHECK_PYPROJ
  PyObject *list=PyList_New(self->proj->sprites.size());
  if (!list) return PyErr_NoMemory();

  for (int i=0; i<self->proj->sprites.size(); ++i)
    PyList_SetItem(list, i, sprite_to_py(self->proj->sprites[i]));

  return list;
}


static int PyProject_setSprites(PyProject *self, PyObject *value, void*)
{
  CHECK_PYPROJ_S

  if (!PySequence_Check(value))
  {
    PyErr_SetString(PyExc_TypeError, "Expected sequence of sprites");
    return -1;
  }

  size_t num=PySequence_Size(value);
  if (num==size_t(-1))
  {
    PyErr_SetString(PyExc_TypeError, "Expected sequence of sprites");
    return -1;
  }

  self->proj->sprites.clear();
  self->proj->sprites.reserve(num);

  for (size_t i=0; i<num; ++i)
  {
    PyObject *o=PySequence_GetItem(value, i);
    if (!o) continue;
    if (!PyObject_TypeCheck(o, &sproxelPySpriteType)) continue;
    self->proj->sprites.push_back(((PySprite*)o)->spr);
  }

  return 0;
}


static PyGetSetDef pyProject_getsets[]=
{
  {"sprites", (getter)PyProject_getSprites, (setter)PyProject_setSprites, "Sprites list", NULL},
  {NULL, NULL, NULL, NULL, NULL}
};


static PyMethodDef pyProject_methods[]=
{
  //{ "reset", (PyCFunction)PyProject_reset, METH_NOARGS, "Reset sprite to the default empty state." },
  { NULL, NULL, 0, NULL }
};


static int PyProject_cmp(PyProject *self, PyObject *o)
{
  if (o==Py_None && !self->proj) return 0;
  if (!PyObject_TypeCheck(o, &sproxelPyProjectType)) return -1;
  PyProject *other=(PyProject*)o;
  if (self->proj==other->proj) return 0;
  if (self->proj.constData()<other->proj.constData()) return -1;
  return 1;
}


PyTypeObject sproxelPyProjectType=
{
  PyObject_HEAD_INIT(NULL)
  0,                         /*ob_size*/
  "sproxel.Project",         /*tp_name*/
  sizeof(PyProject),         /*tp_basicsize*/
  0,                         /*tp_itemsize*/
  (destructor)PyProject_dtor,/*tp_dealloc*/
  0,                         /*tp_print*/
  0,                         /*tp_getattr*/
  0,                         /*tp_setattr*/
  (cmpfunc)PyProject_cmp,    /*tp_compare*/
  0,                         /*tp_repr*/
  0,                         /*tp_as_number*/
  0,                         /*tp_as_sequence*/
  0,                         /*tp_as_mapping*/
  0,                         /*tp_hash */
  0,                         /*tp_call*/
  0,                         /*tp_str*/
  0,                         /*tp_getattro*/
  0,                         /*tp_setattro*/
  0,                         /*tp_as_buffer*/
  Py_TPFLAGS_DEFAULT,        /*tp_flags*/
  "Sproxel project",         /* tp_doc */
  0,                           /* tp_traverse */
  0,                           /* tp_clear */
  0,                           /* tp_richcompare */
  0,                           /* tp_weaklistoffset */
  0,                           /* tp_iter */
  0,                           /* tp_iternext */
  pyProject_methods,         /* tp_methods */
  0,                         /* tp_members */
  pyProject_getsets,         /* tp_getset */
  0,                         /* tp_base */
  0,                         /* tp_dict */
  0,                         /* tp_descr_get */
  0,                         /* tp_descr_set */
  0,                         /* tp_dictoffset */
  (initproc)PyProject_init,  /* tp_init */
  0,                         /* tp_alloc */
  0,                         /* tp_new */
};


//ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//


static PyObject* project_to_py(SproxelProjectPtr proj)
{
  if (!proj) Py_RETURN_NONE;
  PyProject *pyp=PyObject_New(PyProject, &sproxelPyProjectType);
  if (!pyp) return PyErr_NoMemory();
  *((void**)&pyp->proj)=NULL; // reset memory
  pyp->proj=proj;
  return (PyObject*)pyp;
}


//ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//


static PyObject* PySproxel_getProject(PyObject *)
{
  return project_to_py(main_window->project());
}


static PyObject* PySproxel_layerFromPng(PyObject *, PyObject *arg)
{
  uchar *buf=NULL;
  Py_ssize_t len=0;

  if (PyString_AsStringAndSize(arg, (char**)&buf, &len)!=0) return NULL;

  QImage image;
  if (!image.loadFromData(buf, len, "PNG"))
  {
    PyErr_SetString(PyExc_RuntimeError, "error loading PNG image");
    return NULL;
  }

  VoxelGridLayerPtr layer(VoxelGridLayer::fromQImage(image));
  return layer_to_py(layer);
}


static PyMethodDef moduleMethods[]=
{
  { "get_project", (PyCFunction)PySproxel_getProject, METH_NOARGS, "Get current Sproxel project." },
  { "layer_from_png", (PyCFunction)PySproxel_layerFromPng, METH_O, "Create layer from PNG data." },
  { NULL, NULL, 0, NULL }
};


//ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//


bool save_project(QString filename, SproxelProjectPtr project)
{
  if (!py_save_project) return false;
  PyObject *fn=qstr_to_py(filename);
  PyObject *pr=project_to_py(project);
  if (!fn || !pr)
  {
    PyErr_Print();
    Py_XDECREF(fn);
    Py_XDECREF(pr);
    return false;
  }

  PyObject *res=PyObject_CallFunctionObjArgs(py_save_project, fn, pr, NULL);
  Py_XDECREF(fn);
  Py_XDECREF(pr);

  if (!res)
  {
    PyErr_Print();
    return false;
  }

  bool result=PyObject_IsTrue(res);
  Py_DECREF(res);

  return result;
}


SproxelProjectPtr load_project(QString filename)
{
  if (!py_load_project) return SproxelProjectPtr();

  PyObject *fn=qstr_to_py(filename);
  if (!fn) { PyErr_Print(); return SproxelProjectPtr(); }

  PyObject *res=PyObject_CallFunctionObjArgs(py_load_project, fn, NULL);
  Py_XDECREF(fn);

  if (!res || !PyObject_TypeCheck(res, &sproxelPyProjectType))
  {
    PyErr_Print();
    Py_XDECREF(res);
    return SproxelProjectPtr();
  }

  SproxelProjectPtr result=((PyProject*)res)->proj;
  Py_DECREF(res);
  return result;
}


//ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//


void init_sproxel_bindings()
{
  // init types
  sproxelPyLayerType.tp_new=PyType_GenericNew;
  if (PyType_Ready(&sproxelPyLayerType)<0) return;

  sproxelPySpriteType.tp_new=PyType_GenericNew;
  if (PyType_Ready(&sproxelPySpriteType)<0) return;

  sproxelPyProjectType.tp_new=PyType_GenericNew;
  if (PyType_Ready(&sproxelPyProjectType)<0) return;

  // create module
  PyObject *mod=Py_InitModule3("sproxel", moduleMethods, "Sproxel data types.");

  // add types
  Py_INCREF(&sproxelPyLayerType); PyModule_AddObject(mod, "Layer", (PyObject*)&sproxelPyLayerType);
  Py_INCREF(&sproxelPySpriteType); PyModule_AddObject(mod, "Sprite", (PyObject*)&sproxelPySpriteType);
  Py_INCREF(&sproxelPyProjectType); PyModule_AddObject(mod, "Project", (PyObject*)&sproxelPyProjectType);
}
