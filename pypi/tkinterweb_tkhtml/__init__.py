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
__version__ = '1.1'

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

os.environ["PATH"] = os.pathsep.join([
    os.path.dirname(TKHTML_ROOT_DIR),
    os.environ["PATH"]
])

TKHTML_BINARIES =  [file for file in os.listdir(TKHTML_ROOT_DIR) if "libTkhtml" in file]

tkhtml_loaded = False


def get_tkhtml_file(version=None):
    "Get the location of the platform's Tkhtml binary"
    if not version:
        # Get highest numbered avaliable file if a version is not provided
        file = sorted(TKHTML_BINARIES)[-1]
        version = file.replace("libTkhtml", "")
        version = version[:version.rfind(".")]
        return os.path.join(TKHTML_ROOT_DIR, file), version
    else:
        for file in TKHTML_BINARIES:
            if version in file:
                return os.path.join(TKHTML_ROOT_DIR, file), version
        raise OSError(f"Tkhtml version {version} either does not exist or is unsupported on your system")


def load_tkhtml(master, use_prebuilt=True, version=None, file=None, force=False):
    "Load Tkhtml into the current Tcl/Tk instance"
    global tkhtml_loaded, tkhtml_version
    if (not tkhtml_loaded) or force:
        if use_prebuilt:
            if not file:
                file, version = get_tkhtml_file(version)
            master.tk.call("load", file)
            tkhtml_loaded = True
            return version
        else:
            master.tk.call("package", "require", "Tkhtml")
            tkhtml_loaded = True
            return master.tk.call("package", "present", "Tkhtml")

        