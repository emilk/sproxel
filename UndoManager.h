#ifndef __UNDO_MANAGER_H__
#define __UNDO_MANAGER_H__

#include "Global.h"

#include <QString>
#include <QWidget>
#include <QUndoStack>
#include <QUndoCommand>


// A wrapper class for Sproxel.  
// Allows for an easy subset of undo/redo operations.
class UndoManager : public QWidget
{
    Q_OBJECT
    
public:
    UndoManager();
    virtual ~UndoManager() {}

    void changeEntireVoxelGrid(SproxelGrid& origGrid, 
                               const SproxelGrid& newGrid);

    void setVoxelColor(SproxelGrid& origGrid, 
                       const Imath::V3i& index, 
                       const Imath::Color4f& color);

    void beginMacro(const QString& macroName);
    void endMacro();
    void clear();

    void undo();
    void redo();

    void setClean();
    bool isClean() const;

signals:
    void cleanChanged(bool);

private:
    QUndoStack m_undoStack;

};


// Sproxel registers two QT undo'able commands
// ChangeEntireVoxelGrid (which cannot be Macro'ed)
class CmdChangeEntireVoxelGrid : public QUndoCommand
{
public:
    CmdChangeEntireVoxelGrid(SproxelGrid* gvg, const SproxelGrid& newGrid) :
        m_pGvg(gvg)
    {
        m_newGrid = newGrid;
        m_oldGrid = *gvg;
        setText("Change grid");
    }

    virtual void redo()
    {
        *m_pGvg = m_newGrid;
    }

    virtual void undo()
    {
        *m_pGvg = m_oldGrid;
    }

private:
    SproxelGrid* m_pGvg;
    SproxelGrid  m_newGrid;
    SproxelGrid  m_oldGrid;
};


// SetVoxelColor (which can be Macro'ed)
class CmdSetVoxelColor : public QUndoCommand
{
public:
    CmdSetVoxelColor(SproxelGrid* gvg, const Imath::V3i& index, const Imath::Color4f color) :
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
    SproxelGrid* m_pGvg;
    std::vector<Imath::V3i> m_index;
    std::vector<Imath::Color4f> m_newColor;
    std::vector<Imath::Color4f> m_oldColor;
};

#endif
