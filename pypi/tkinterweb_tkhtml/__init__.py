"""
TkinterWeb-Tkhtml v1.0
This package provides pre-built binaries of a modified version of the Tkhtml3 widget from http://tkhtml.tcl.tk/tkhtml.html, 
which enables the display of styled HTML and CSS code in Tkinter applications.

This package can be used to import the Tkhtml widget into Tkinter projects
but is mainly intended to be used through TkinterWeb, which provides a full Python interface. 
See https://github.com/Andereoo/TkinterWeb.

Copyright (c) 2025 Andereoo
"""

import os

# Begin universal sdist
import sys
import platform

PLATFORM = platform.uname()
# End universal sdist


__title__ = 'TkinterWeb-Tkhtml'
__author__ = "Andereoo"
__copyright__ = "Copyright (c) 2025 Andereoo"
__license__ = "MIT"
__version__ = '1.0'


TKHTML_RELEASE = "3.0 (TkinterWeb standard)" # For debugging; eventually this project might also bundle experimental binaries
TKHTML_ROOT_DIR = os.path.join(os.path.abspath(os.path.dirname(__file__)), "tkhtml")

tkhtml_loaded = False


def get_tkhtml_folder():
    "Get the location of the platform's Tkhtml binary"
    # Begin universal sdist
    if PLATFORM.system == "Linux":
       if "arm" in PLATFORM.machine: # 32 bit arm Linux - Raspberry Pi and others
           return os.path.join(TKHTML_ROOT_DIR, "linux_armv71")
       elif "aarch64" in PLATFORM.machine: # 64 bit arm Linux - Raspberry Pi and others
           return os.path.join(TKHTML_ROOT_DIR, "manylinux2014_aarch64")
       elif sys.maxsize > 2**32: # 64 bit Linux
           return os.path.join(TKHTML_ROOT_DIR, "manylinux1_x86_64")
       else: # 32 bit Linux
           return os.path.join(TKHTML_ROOT_DIR, "manylinux1_i686")
    elif PLATFORM.system == "Darwin":
       if "arm" in PLATFORM.machine: # M1 Mac
           return os.path.join(TKHTML_ROOT_DIR, "macosx_11_0_arm64")
       else:  # other Macs
           return os.path.join(TKHTML_ROOT_DIR, "macosx_10_6_x86_64")
    else:
       if sys.maxsize > 2**32: # 64 bit Windows
           return os.path.join(TKHTML_ROOT_DIR, "win_amd64")
       else: # 32 bit Windows
           return os.path.join(TKHTML_ROOT_DIR, "win32")
    # End universal sdist
    return TKHTML_ROOT_DIR


def load_tkhtml(master, location=None, force=False):
    "Load Tkhtml into the current Tcl/Tk instance"
    global tkhtml_loaded
    if (not tkhtml_loaded) or force:
        if location:
            master.tk.eval("set auto_path [linsert $auto_path 0 {" + location + "}]")
        master.tk.eval("package require Tkhtml")
        tkhtml_loaded = True