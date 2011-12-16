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

    void changeEntireVoxelGrid(VoxelGridGroupPtr origGrid,
                               const VoxelGridGroupPtr newGrid);

    void setVoxelColor(VoxelGridGroupPtr origGrid,
                       const Imath::V3i& at,
                       const Imath::Color4f& color,
                       int index);

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
    CmdSetVoxelColor(VoxelGridGroupPtr gvg, const Imath::V3i& index, const Imath::Color4f color, int ind) :
        m_pGvg(gvg),
        m_layerId(gvg->curLayerIndex()),
        m_index(),
        m_newColor(),
        m_oldColor()
    {
        m_index.push_back(index);
        m_newColor.push_back(color);
        m_oldColor.push_back(gvg->get(index));
        m_newIndex.push_back(ind);
        m_oldIndex.push_back(gvg->getInd(index));
        setText("Set voxel");
    }

    virtual void redo()
    {
        VoxelGridLayerPtr layer=m_pGvg->layer(m_layerId);
        if (!layer) return;

        for (size_t i = 0; i < m_index.size(); i++)
            layer->set(m_index[i], m_newColor[i], m_newIndex[i]);
    }

    virtual void undo()
    {
        VoxelGridLayerPtr layer=m_pGvg->layer(m_layerId);
        if (!layer) return;

        for (size_t i = 0; i < m_index.size(); i++)
            layer->set(m_index[i], m_oldColor[i], m_oldIndex[i]);
    }

protected:
    virtual bool mergeMeWith(QUndoCommand* other)
    {
        if (other->id() != id())
            return false;

        const CmdSetVoxelColor* otherSet = static_cast<const CmdSetVoxelColor*>(other);
        if (otherSet->m_pGvg!=m_pGvg || otherSet->m_layerId!=m_layerId) return false;

        // TODO: shouldn't it push _all_ elements of otherSet?
        m_index.push_back(otherSet->m_index[0]);
        m_newColor.push_back(otherSet->m_newColor[0]);
        m_newIndex.push_back(otherSet->m_newIndex[0]);
        m_oldColor.push_back(otherSet->m_oldColor[0]);
        m_oldIndex.push_back(otherSet->m_oldIndex[0]);
        return true;
    }

private:
    VoxelGridGroupPtr m_pGvg;
    int m_layerId;
    std::vector<Imath::V3i> m_index;
    std::vector<Imath::Color4f> m_newColor;
    std::vector<Imath::Color4f> m_oldColor;
    std::vector<int> m_newIndex;
    std::vector<int> m_oldIndex;
};

#endif
