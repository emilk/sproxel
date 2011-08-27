#include "UndoManager.h"

UndoManager::UndoManager()
{
    QObject::connect(&m_undoStack, SIGNAL(cleanChanged(bool)),
                     this, SIGNAL(cleanChanged(bool)));
}


void UndoManager::changeEntireVoxelGrid(SproxelGrid& origGrid,
                                        const SproxelGrid& newGrid)
{
    m_undoStack.push(new CmdChangeEntireVoxelGrid(&origGrid, newGrid));
}


void UndoManager::setVoxelColor(SproxelGrid& origGrid,
                                const Imath::V3i& index,
                                const Imath::Color4f& color)
{
    // Validity check
    /*
    const Imath::V3i& cd = origGrid.cellDimensions();
    if (index.x < 0     || index.y < 0     || index.z < 0 ||
        index.x >= cd.x || index.y >= cd.y || index.z >= cd.z)
        return;
    */

    m_undoStack.push(new CmdSetVoxelColor(&origGrid, index, color));
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
