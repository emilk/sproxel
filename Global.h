#ifndef __GLOBAL_H
#define __GLOBAL_H

#include <ImathColor.h>
#include "GameVoxelGrid.h"

enum SproxelAxis { X_AXIS, Y_AXIS, Z_AXIS };

enum SproxelTool { TOOL_SPLAT, 
                   TOOL_FLOOD, 
                   TOOL_RAY,
                   TOOL_DROPPER, 
                   TOOL_ERASER, 
                   TOOL_REPLACE, 
                   TOOL_SLAB };

typedef GameVoxelGrid<Imath::Color4f> SproxelGrid;

#endif
