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

# Create a Label and show it
#label = QLabel("Hello World")
#label.show()
