#import sproxelConsole
#sproxelConsole.console_write("test!")

print "in test.py!!!"
import sys
from PySide.QtCore import *
from PySide.QtGui import *

print "trying glue..."
from PySide.SproxelGlue import *

print "in test.py!", 1, 2, 3
print "sys.path: ", sys.path

print "main window: ", repr(main_window)

main_window.statusBar().showMessage("Python was here!")

import sproxel
l=sproxel.Layer((10, 20, 30), name='My Layer')
l.name="Changed name"
l.offset=(1, 2, 3)
print 'layer:', repr(l), l.name, l.offset, l.size, l.bounds, l.dataType
l.set(5, 5, 5, 0x112233);
print 'color:', l.getColor(5, 5, 5)
#l.reset()
#print 'layer:', repr(l), l.name, l.offset, l.size, l.bounds, l.dataType

print ''
s=sproxel.Sprite(l)
print 'sprite:', repr(s)
s.insertLayerAbove(0)
print 'curLayerIndex:', s.curLayerIndex
print 'curLayer:', s.curLayer, s.curLayer.name, s.curLayer.bounds
print 'layer(0):', s.layer(0), s.layer(0).name
print 'layer("Changed name"):', s.layer('Changed name')

# Create a Label and show it
#label = QLabel("Hello World")
#label.show()
