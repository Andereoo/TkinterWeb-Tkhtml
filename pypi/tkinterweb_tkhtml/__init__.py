"""
TkinterWeb-Tkhtml v1.1
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
__version__ = '1.1.0'

TKHTML_RELEASE = "3.0 (TkinterWeb standard)" # Backwards-compatibility. Not useful or even necessarily true anymore. Will be removed.

TKHTML_ROOT_DIR = os.path.abspath(os.path.dirname(__file__))

# Begin universal sdist
if PLATFORM.system == "Linux":
    if "arm" in PLATFORM.machine: # 32 bit arm Linux - Raspberry Pi and others
        TKHTML_ROOT_DIR = os.path.join(TKHTML_ROOT_DIR, "tkhtml", "linux_armv71")
    elif "aarch64" in PLATFORM.machine: # 64 bit arm Linux - Raspberry Pi and others
        TKHTML_ROOT_DIR = os.path.join(TKHTML_ROOT_DIR, "tkhtml", "manylinux2014_aarch64")
    elif sys.maxsize > 2**32: # 64 bit Linux
        TKHTML_ROOT_DIR = os.path.join(TKHTML_ROOT_DIR, "tkhtml", "manylinux1_x86_64")
    else: # 32 bit Linux
        TKHTML_ROOT_DIR = os.path.join(TKHTML_ROOT_DIR, "tkhtml", "manylinux1_i686")
elif PLATFORM.system == "Darwin":
    if "arm" in PLATFORM.machine: # M1 Mac
        TKHTML_ROOT_DIR = os.path.join(TKHTML_ROOT_DIR, "tkhtml", "macosx_11_0_arm64")
    else:  # other Macs
        TKHTML_ROOT_DIR = os.path.join(TKHTML_ROOT_DIR, "tkhtml", "macosx_10_6_x86_64")
else:
    if sys.maxsize > 2**32: # 64 bit Windows
        TKHTML_ROOT_DIR = os.path.join(TKHTML_ROOT_DIR, "tkhtml", "win_amd64")
    else: # 32 bit Windows
        TKHTML_ROOT_DIR = os.path.join(TKHTML_ROOT_DIR, "tkhtml", "win32")
# End universal sdist

TKHTML_BINARIES =  [file for file in os.listdir(TKHTML_ROOT_DIR) if "libTkhtml" in file]


tkhtml_loaded = False


def get_tkhtml_folder():
    # Backwards-compatibility. Will be removed.
    return TKHTML_ROOT_DIR

def get_tkhtml_file(version=None, index=-1):
    "Get the location of the platform's Tkhtml binary"
    if not version:
        # Get highest numbered avaliable file if a version is not provided
        file = sorted(TKHTML_BINARIES)[index]
        version = file.replace("libTkhtml", "")
        version = version[:version.rfind(".")]
        return os.path.join(TKHTML_ROOT_DIR, file), version
    else:
        for file in TKHTML_BINARIES:
            if version in file:
                return os.path.join(TKHTML_ROOT_DIR, file), version
        raise OSError(f"Tkhtml version {version} either does not exist or is unsupported on your system")


def load_tkhtml_file(master, file, force=False):
    "Load Tkhtml into the current Tcl/Tk instance"
    global tkhtml_loaded
    if (not tkhtml_loaded) or force:
        if TKHTML_ROOT_DIR not in os.environ["PATH"].split(os.pathsep):
            os.environ["PATH"] = os.pathsep.join([
                TKHTML_ROOT_DIR,
                os.environ["PATH"]
            ])
        master.tk.call("load", file)
        tkhtml_loaded = True


def load_tkhtml(master, force=False, use_prebuilt=False):
    "Load Tkhtml into the current Tcl/Tk instance"
    if use_prebuilt:
        # Backwards-compatibility. Will likely be removed.
        file, version = get_tkhtml_file(None, 0)
        load_tkhtml_file(master, file, force)
        return version
    
    global tkhtml_loaded
    if (not tkhtml_loaded) or force:
        master.tk.call("package", "require", "Tkhtml")
        tkhtml_loaded = True
        return master.tk.call("package", "present", "Tkhtml")