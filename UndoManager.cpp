#include "UndoManager.h"

UndoManager::UndoManager()
{
    QObject::connect(&m_undoStack, SIGNAL(cleanChanged(bool)),
                     this, SIGNAL(cleanChanged(bool)));
}


void UndoManager::changeEntireVoxelGrid(VoxelGridGroupPtr origGrid,
                                        const VoxelGridGroupPtr newGrid)
{
    m_undoStack.push(new CmdChangeEntireVoxelGrid(origGrid, newGrid));
}


void UndoManager::setVoxelColor(VoxelGridGroupPtr sprite,
                                const Imath::V3i& pos,
                                const Imath::Color4f& color,
                                int index)
{
    // Validity check
    if (!sprite) return;

    VoxelGridLayerPtr layer=sprite->curLayer();
    if (!layer) return;

    m_undoStack.push(new CmdSetVoxelColor(sprite, layer, pos, color, index));
}


void UndoManager::beginMacro(const QString& macroName)
{
    m_undoStack.beginMacro(macroName);
}


void UndoManager::endMacro()
{
    m_undoStack.endMacro();
}


void UndoManager::clear()
{
    m_undoStack.clear();
}


void UndoManager::setClean()
{
    m_undoStack.setClean();
}


bool UndoManager::isClean() const
{
    return m_undoStack.isClean();
}


void UndoManager::undo()
{
    m_undoStack.undo();
}


void UndoManager::redo()
{
    m_undoStack.redo();
}
