#
# Firmware Programmer
# Copyright <c> 2012 Sifteo, Inc.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#

#
# Used to temporarily program firmware on to the masters
# at the factory. Requires swiss and the master firmware
# file to be in the same directory. I've put these files in
# C:\Python27 directory and then make a lnk from there
# to the desktop on the test machine
#

import subprocess
from Tkinter import *

FILENAME = ""

class App:
    
    def __init__(self, master):
        
        # w = Canvas(master, width=200, height=50)
        # w.pack()
        # rectangle = w.create_rectangle(0, 0, 200,100, fill="grey")

        frame = Frame(master, height=100, width=100)
        frame.pack()
        
        self.program = Button(frame, text="Program", command=self.program, padx=50, pady=50, font=('Courier New', 50, 'bold'))
        self.program.pack()

    def program(self):
        try:
			subprocess.check_call(["swiss", "update", FILENAME])
        except:
            print "ERROR: Couldnt program using file: %s" % FILENAME

if __name__ == '__main__':
    
    if len(sys.argv) < 2:
        print >> sys.stderr, "usage: python program.py <filename>"
        sys.exit(1)
        
    
    FILENAME = sys.argv[1]    
    
    root = Tk()
    
    root.wm_title("Firmware Programmer")
    root.resizable(False,False)
	
    app = App(root)

    root.mainloop()




