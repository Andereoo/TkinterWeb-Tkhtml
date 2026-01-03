# TkinterWeb-Tkhtml
**This directory contains all source code files for the Tkhtml widget from [http://tkhtml.tcl.tk](https://web.archive.org/web/20250219233338/http://tkhtml.tcl.tk/), which renders HTML for Tcl/Tk 8.0 and later.** 

Some of the files have been modified for use with [TkinterWeb](https://github.com/Andereoo/TkinterWeb).

## Installation

To compile Tkhtml, download the repository and run the [compile.py](./compile.py) script. 

If using TkinterWeb, run ``python compile.py configure --install`` to compile and install Tkhtml.

Alternatively, see [COMPILE.txt](./COMPILE.txt) for more ways to compile this widget.

By default, this version of Tkhtml requires Cairo to be installed for border-radius support. Cairo is not part of Tcl/Tk. If you don't have it installed and don't want to install it, add the flag ``--disable-cairo`` to disable this feature.

## Directories

* [doc](./doc) - Other documentation about TkHtml.
* [src](./src) - All of the source code.
* [hv](./hv) - Source code for the Hv3 widget.
* [tools](./tools) - Source code to tools that are used to build the widget but which do not become part of the widget.

## Credits

 Thanks to the [TkHtml3 project](http://tkhtml.tcl.tk/) for source code, [Zamy846692](https://github.com/Zamy846692/TkinterWeb-Tkhtml) for numerous modifications, [BRL-CAD](https://github.com/BRL-CAD/brlcad) for 64-bit Windows modifications.
