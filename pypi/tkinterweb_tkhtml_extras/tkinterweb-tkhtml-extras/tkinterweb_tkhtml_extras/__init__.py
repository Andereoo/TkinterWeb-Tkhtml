"""
TkinterWeb-Tkhtml-Extras v1.0
This package provides pre-built binaries of a modified version of the Tkhtml3 widget from https://github.com/Andereoo/TkinterWeb-Tkhtml, 
which enables the display of styled HTML and CSS code in Tkinter applications.

Copyright (c) 2025 Andrew Clarke
"""

import os

__title__ = 'TkinterWeb-Tkhtml-Extras'
__author__ = "Andrew Clarke"
__copyright__ = "Copyright (c) 2025 Andrew Clarke"
__license__ = "MIT"
__version__ = '1.0.0'


# --- Begin universal sdist ---------------------------------------------------
import sys
import platform

PLATFORM = platform.uname()
# --- End universal sdist -----------------------------------------------------

TKHTML_EXTRAS_ROOT_DIR = os.path.join(os.path.abspath(os.path.dirname(__file__)), "tkhtml")

# --- Begin universal sdist -----------------------------------------------------
if PLATFORM.system == "Linux":
    if "arm" in PLATFORM.machine: # 32 bit arm Linux - Raspberry Pi and others
        TKHTML_EXTRAS_ROOT_DIR = None
    elif "aarch64" in PLATFORM.machine: # 64 bit arm Linux - Raspberry Pi and others
        TKHTML_EXTRAS_ROOT_DIR = None
    elif sys.maxsize > 2**32: # 64 bit Linux
        TKHTML_EXTRAS_ROOT_DIR = os.path.join(TKHTML_EXTRAS_ROOT_DIR, "manylinux1_x86_64")
    else: # 32 bit Linux
        TKHTML_EXTRAS_ROOT_DIR = None
elif PLATFORM.system == "Darwin":
    TKHTML_EXTRAS_ROOT_DIR = None
else:
    if sys.maxsize > 2**32: # 64 bit Windows
        TKHTML_EXTRAS_ROOT_DIR = os.path.join(TKHTML_EXTRAS_ROOT_DIR, "win_amd64")
    else: # 32 bit Windows
        TKHTML_EXTRAS_ROOT_DIR = None
# --- End universal sdist -----------------------------------------------------