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
__version__ = '1.1.3'

TKHTML_RELEASE = "3.0 (TkinterWeb standard)" # Backwards-compatibility. Not useful or even necessarily true anymore. Will be removed.

TKHTML_ROOT_DIR = os.path.join(os.path.abspath(os.path.dirname(__file__)), "tkhtml")

# Begin universal sdist
if PLATFORM.system == "Linux":
    if "arm" in PLATFORM.machine: # 32 bit arm Linux - Raspberry Pi and others
        TKHTML_ROOT_DIR = os.path.join(TKHTML_ROOT_DIR, "linux_armv71")
    elif "aarch64" in PLATFORM.machine: # 64 bit arm Linux - Raspberry Pi and others
        TKHTML_ROOT_DIR = os.path.join(TKHTML_ROOT_DIR, "manylinux2014_aarch64")
    elif sys.maxsize > 2**32: # 64 bit Linux
        TKHTML_ROOT_DIR = os.path.join(TKHTML_ROOT_DIR, "manylinux1_x86_64")
    else: # 32 bit Linux
        TKHTML_ROOT_DIR = os.path.join(TKHTML_ROOT_DIR, "manylinux1_i686")
elif PLATFORM.system == "Darwin":
    if "arm" in PLATFORM.machine: # M1 Mac
        TKHTML_ROOT_DIR = os.path.join(TKHTML_ROOT_DIR, "macosx_11_0_arm64")
    else:  # other Macs
        TKHTML_ROOT_DIR = os.path.join(TKHTML_ROOT_DIR, "macosx_10_6_x86_64")
else:
    if sys.maxsize > 2**32: # 64 bit Windows
        TKHTML_ROOT_DIR = os.path.join(TKHTML_ROOT_DIR, "win_amd64")
    else: # 32 bit Windows
        TKHTML_ROOT_DIR = os.path.join(TKHTML_ROOT_DIR, "win32")
# End universal sdist

TKHTML_BINARIES =  [file for file in os.listdir(TKHTML_ROOT_DIR) if "libTkhtml" in file]

# NOTE:
# File names should be in the format libTkhtml[major].[minor].[so/dll/dylib]
# Experimental file names should be in the format libTkhtml[major].[minor]exp.[so/dll/dylib]

tkhtml_file = None

help_message = f"To add a new Tkhtml version, drop your binary into the {TKHTML_ROOT_DIR} folder, named using the following conventions:\n\
- For a standard release: libTkhtml[major_version.minor_version].[dll/dylib.so] (eg. libTkhtml3.0.dll)\n\
- For an experimental release: libTkhtml[major_version.minor_version]exp.[dll/dylib.so] (eg. libTkhtml3.1exp.dll)"

def get_tkhtml_folder():
    # Backwards-compatibility. Will be removed.
    return TKHTML_ROOT_DIR

def get_loaded_tkhtml_version():
    global tkhtml_file
    if tkhtml_file:
        version = os.path.basename(tkhtml_file).replace("libTkhtml", "")
        return version[:version.rfind(".")]
    else:
        return None
    
def get_tkhtml_file(version=None, index=-1, experimental=False):
    "Get the location of the platform's Tkhtml binary"
    if isinstance(version, float):
        version = str(version)
    if version:
        for file in TKHTML_BINARIES:
            if version in file:
                # Note: experimental can be "auto"
                if "exp" in file:
                    if not experimental:
                        raise OSError(f"Tkhtml version {version} is an experimental release but experimental mode is disabled. {help_message}")
                    experimental = True
                else:
                    if experimental == True:
                        raise OSError(f"Tkhtml version {version} is not an experimental release but experimental mode is enabled. {help_message}")
                    experimental = False
                return os.path.join(TKHTML_ROOT_DIR, file), version, experimental
        raise OSError(f"Tkhtml version {version} either does not exist or is unsupported on your system. {help_message}")
    else:
        # Get highest numbered avaliable file if a version is not provided
        if experimental == True:
            files = [k for k in TKHTML_BINARIES if 'exp' in k]
            if not files:
                raise OSError(f"No experimental Tkhtml versions could be found on your system. {help_message}")
        elif not experimental:
            files = [k for k in TKHTML_BINARIES if 'exp' not in k]
        else:
            files = TKHTML_BINARIES
        file = sorted(files)[index]
        if "exp" in file:
            experimental = True
        else:
            experimental = False
        version = file.replace("libTkhtml", "").replace("exp", "")
        version = version[:version.rfind(".")]
        return os.path.join(TKHTML_ROOT_DIR, file), version, experimental


def load_tkhtml_file(master, file, force=False):
    "Load Tkhtml into the current Tcl/Tk instance"
    global tkhtml_file
    if (not tkhtml_file) or force:
        if TKHTML_ROOT_DIR not in os.environ["PATH"].split(os.pathsep):
            os.environ["PATH"] = os.pathsep.join([
                TKHTML_ROOT_DIR,
                os.environ["PATH"]
            ])
        master.tk.call("load", file)
        tkhtml_file = file


def load_tkhtml(master, force=False, use_prebuilt=False):
    "Load Tkhtml into the current Tcl/Tk instance"
    if use_prebuilt:
        # Backwards-compatibility. Will likely be removed.
        file, version, experimental = get_tkhtml_file(None, 0)
        load_tkhtml_file(master, file, force)
        return version
    
    global tkhtml_file
    if (not tkhtml_file) or force:
        master.tk.call("package", "require", "Tkhtml")
        tkhtml_file = True
        return master.tk.call("package", "present", "Tkhtml")