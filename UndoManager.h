#ifndef __UNDO_MANAGER_H__
#define __UNDO_MANAGER_H__

#include "Global.h"

#include <QString>
#include <QWidget>
#include <QVector>
#include <QUndoStack>
#include <QUndoCommand>


// A wrapper class for Sproxel.
// Allows for an easy subset of undo/redo operations.
class UndoManager : public QWidget
{
    Q_OBJECT

public:

    enum { ID_SETVOXEL=1, };

    UndoManager();
    virtual ~UndoManager() {}

    void changeEntireVoxelGrid(VoxelGridGroupPtr origGrid,
                               const VoxelGridGroupPtr newGrid);

    void setVoxelColor(VoxelGridGroupPtr origGrid,
                       const Imath::V3i& at,
                       const Imath::Color4f& color,
                       int index);

    void setPaletteColor(ColorPalettePtr pal, int index, const SproxelColor &color);

    void beginMacro(const QString& macroName);
    void endMacro();
    void clear();

    void undo();
    void redo();

    void setClean();
    bool isClean() const;

    QAction* createUndoAction(QObject *parent, const QString &prefix)
      { return m_undoStack.createUndoAction(parent, prefix); }

    QAction* createRedoAction(QObject *parent, const QString &prefix)
      { return m_undoStack.createRedoAction(parent, prefix); }

signals:
    void cleanChanged(bool);

private:
    QUndoStack m_undoStack;

};


// Change single palette color
class CmdSetPaletteColor : public QUndoCommand
{
public:

  CmdSetPaletteColor(ColorPalettePtr pal, int index, const SproxelColor &color)
    : m_palette(pal), m_index(index), m_newColor(color)
  {
    m_oldColor=pal->color(index);
    setText("Change palette color");
  }

  virtual void redo()
  {
    m_palette->setColor(m_index, m_newColor);
  }

  virtual void undo()
  {
    m_palette->setColor(m_index, m_oldColor);
  }

private:
  ColorPalettePtr m_palette;
  int m_index;
  SproxelColor m_oldColor, m_newColor;
};


// ChangeEntireVoxelGrid (which cannot be Macro'ed)
class CmdChangeEntireVoxelGrid : public QUndoCommand
{
public:
    CmdChangeEntireVoxelGrid(VoxelGridGroupPtr gvg, const VoxelGridGroupPtr newGrid) :
        m_pGvg(gvg)
    {
        m_newGrid = new VoxelGridGroup(*newGrid);
        m_oldGrid = new VoxelGridGroup(*gvg);
        setText("Change grid");
    }

    virtual void redo()
    {
        *m_pGvg = *m_newGrid;
    }

    virtual void undo()
    {
        *m_pGvg = *m_oldGrid;
    }

private:
    VoxelGridGroupPtr m_pGvg;
    VoxelGridGroupPtr m_newGrid;
    VoxelGridGroupPtr m_oldGrid;
};


// SetVoxelColor (which can be Macro'ed)
class CmdSetVoxelColor : public QUndoCommand
{
public:

  CmdSetVoxelColor(VoxelGridGroupPtr spr, VoxelGridLayerPtr layer,
    const Imath::V3i& pos, const Imath::Color4f &color, int index)
    : m_sprite(spr), m_layer(layer)
  {
    m_changes.push_back(Change(pos, m_layer->getColor(pos), m_layer->getInd(pos), color, index));
    setText("Set voxel");
  }

  virtual void redo()
  {
    if (!m_layer) return;

    for (int i=0; i<m_changes.size(); ++i)
    {
      const Change &c=m_changes[i];
      m_layer->set(c.pos, c.newColor, c.newIndex);
    }
  }

  virtual void undo()
  {
    for (int i=m_changes.size()-1; i>=0; --i)
    {
      const Change &c=m_changes[i];
      m_layer->set(c.pos, c.oldColor, c.oldIndex);
    }
  }

  virtual int id() const { return UndoManager::ID_SETVOXEL; }

protected:

  struct Change
  {
    Imath::V3i pos;
    SproxelColor oldColor, newColor;
    int oldIndex, newIndex;

    Change() {}
    Change(const Imath::V3i& p, const Imath::Color4f &oc, int oi, const Imath::Color4f &nc, int ni)
      : pos(p), oldColor(oc), oldIndex(oi), newColor(nc), newIndex(ni) {}
  };

  virtual bool mergeWith(const QUndoCommand* other)
  {
    if (other->id() != id()) return false;

    const CmdSetVoxelColor* otherSet = static_cast<const CmdSetVoxelColor*>(other);
    if (otherSet->m_layer!=m_layer || otherSet->m_sprite!=m_sprite) return false;

    m_changes+=otherSet->m_changes;
    return true;
  }

private:
  VoxelGridGroupPtr m_sprite;
  VoxelGridLayerPtr m_layer;
  QVector<Change> m_changes;
};

#endif
