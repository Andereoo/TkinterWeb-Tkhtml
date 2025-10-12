### Script to automatically compile Tkhtml
### Sample usage: python3 compile.py configure -w /usr/local/tcl9/bin/tclsh9.0
### Copyright (c) 2025 Andereoo

import tkinter
import os, glob, subprocess, re, sys, argparse, shutil, platform
from pathlib import Path

### May be "ask", "configure", "test", "build"
### Only applies if a command line argument is not supplied
MODE = "ask"

TEST_STRING = """<body><div>
    <p style='padding: 15px; 
        display: inline-block; 
        white-space: nowrap; 
        margin:0; 
        border: 1px solid grey; 
        border-color: lightgrey grey grey lightgrey; 
        border-radius: 5px; 
        background-color: #34ebb7;'>
    If you see this, woohoo!</p>
</div></body>"""

### You probably won't need to change these
BASE_PATH = os.path.dirname(os.path.abspath(__file__))
BUILD_PATH = os.path.join(BASE_PATH, 'build')
CONFIGURE_PATH = os.path.join(BASE_PATH, 'configure')
SRC_PATH = os.path.join(BASE_PATH, 'src')
CSSPROP_PATH = os.path.join(BASE_PATH, 'src', 'cssprop.tcl')
MAKE_PATH = os.path.join(BASE_PATH, 'build', 'Makefile')
GET_PATHS_PATH = os.path.join(BASE_PATH, 'get_paths.tcl')

SYSTEM = platform.system()

tkhtml_version = None
tkhtml_file = None

root = tkinter.Tcl()
paths = root.exprstring('$auto_path').split()
version = str(tkinter.TkVersion)
paths = list(set(paths))
paths.sort(key=len)

tclConfig_paths = []
tkConfig_paths = []


parser = argparse.ArgumentParser(description="Compile Tkhtml")
parser.add_argument("mode", type=str, nargs="?", default=MODE, choices=["ask", "configure", "test", "build"], help="the default mode")
parser.add_argument('-n', '--noprompt', action='store_true', help='disable prompting for user input')
parser.add_argument('-t', '--notest', action='store_true', help='disable opening a window to test the binary (the binary will still be quietly tested)')
parser.add_argument('-q', '--quiet', action='store_true', help='hide gcc & configure script output')
parser.add_argument('-s', '--silent', action='store_true', help='hide all output')
parser.add_argument('-i', '--install', action='store_true', help='install to the tkinterweb-tkhtml package')
parser.add_argument('-c', '--disable-cairo', action='store_true', help='disable Cairo graphics support')
parser.add_argument('-w', '--with-tclsh', help='the path to the tclsh interpreter to use when looking for Tcl/Tk releases')
args = parser.parse_args()

mode = args.mode
noprompt = args.noprompt
notest = args.notest
quiet = args.quiet
silent = args.silent
install = args.install
disable_cairo = args.disable_cairo
with_tclsh = args.with_tclsh

# Not very 'Pythonic', but works like a charm
if silent:
    if not quiet: quiet = True
    def print(*args, **kwargs):
        pass
if noprompt:
    def input(string):
        #print(string)
        return ""


def print_error(*args):
    print(args)

def run_command(cmd, cmd_input=None, capture_output=False):
    if quiet: capture_output = True
    return subprocess.run(cmd, input=cmd_input, capture_output=capture_output, universal_newlines=True, check=True)

def return_command(cmd):
    return subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True)

def test():
    global tkhtml_version, tkhtml_file
    print("\nTesting result...")
    if with_tclsh:
        script = f"""
        set auto_path [linsert $auto_path 0 {BUILD_PATH}]
        puts [package require Tkhtml]
        puts [info loaded]
        html .h
        pack .h
        .h parse "{TEST_STRING}"
        """
        if notest:
            script += "\ndestroy ."
        out = run_command([with_tclsh], script, True).stdout.strip().splitlines()
        tkhtml_version = out[0].strip()

        for i in " ".join(out[1:]).replace("{", "").split("}"):
            o = i.split()
            if o[1] == "Tkhtml":
                tkhtml_file = o[0]
                break

    else:
        root = tkinter.Tk()
        root.tk.eval("set auto_path [linsert $auto_path 0 {"+BUILD_PATH+"}]")
        tkhtml_version = root.tk.eval("package require Tkhtml")
        widget = tkinter.Widget(root, "html")
        widget.tk.call(widget._w, "parse", TEST_STRING)
        for i in root.tk.call("info", "loaded"):
            if i[1] == "Tkhtml":
                tkhtml_file = i[0]
                break
        
        if notest:
            root.destroy()
        else:
            widget.pack(expand=True, fill="both")
            root.mainloop()
    
    print("Success!")
        

def make():
    if os.name == "nt":
        run_command(["make", "binaries"])
    else:
        run_command(["make"])

print("Welcome to TkinterWeb's TkHtml3.1 compile script. Note that for this to succeed you will need tcl-dev, tk-dev, cairo, gcc, and make installed on your system.")

if mode == "ask":
    mode = input("""Please enter an option:
   C: update CSS support, run the configure script to generate a makefile, and build and test your binary
   T: test the binary
   Any other key: build and test your binary and only run the configure script if the build directory does not exist
> """).upper()

    if mode == "C":
        mode = "configure"
    elif mode == "T":
        mode = "test"
    else:
        mode = "build"

if os.path.exists(BUILD_PATH) and mode == "build":
    os.chdir(BUILD_PATH)
    def compile_tkhtml():
        try:
            print("\nCompiling...")
            make()
        except subprocess.CalledProcessError:
            print("\nFatal error encountered.")
            if noprompt: exit()
            override = input("Press N to abort or any other key to try again: ")
            if override.upper() == "N":
                sys.exit()
            else:
                compile_tkhtml()
    compile_tkhtml()

    print("\nTesting result...")

elif (not os.path.exists(BUILD_PATH) and mode == "build") or mode == "configure":
    print("\nCreating build directory...")
    if os.path.exists(BUILD_PATH):
        if len(os.listdir(BUILD_PATH)) == 0:
            print('Directory "build" already exists and is empty. Skipping.')
        else:
            print('Directory "build" already exists. Erase contents?')
            override = input(f"Press N to skip or any other key to empty {BUILD_PATH}: ")
            if override.upper() != "N":
                files = glob.glob(BUILD_PATH+os.sep+'*')
                for file in files:
                    os.remove(file)
    else:
        Path(BUILD_PATH).mkdir(parents=True, exist_ok=True)
        print("Done!")

    print("\nSearching for Tcl/Tk configuration files...")

    def search():
        global tclConfig_paths, tkConfig_paths, valid_tclConfig_paths, valid_tkConfig_paths

        for path in paths: 
            if os.path.exists(path):
                may_continue = True
                if may_continue:
                    tclConfig_paths += glob.glob(path+os.sep+'**/*tclConfig.sh', recursive=True)
                    tkConfig_paths += glob.glob(path+os.sep+'**/*tkConfig.sh', recursive=True)

        print(f"Found {len(tclConfig_paths)} Tcl configuration file{'' if len(tclConfig_paths) == 1 else 's'} and {len(tkConfig_paths)} Tk configuration file{'' if len(tkConfig_paths) == 1 else 's'}")

        def check_config_files(config_paths, config_type, header_file):
            valid_paths = {}
            for file in config_paths:
                with open(file, "r") as handle:
                    content = handle.read()
                    version = re.findall(config_type+r"_VERSION='(.*?)'", content, flags=re.MULTILINE)
                    include_spec = re.findall(config_type+r"_INCLUDE_SPEC='(.*?)'", content, flags=re.MULTILINE)
                    if version and include_spec:
                        if not include_spec[0]:
                            continue
                        include_spec = include_spec[0].replace("-I", "")
                        if not os.path.isdir(include_spec) and os.name == "nt": # msys2
                            try:
                                old_include_spec = include_spec
                                include_spec = subprocess.run(['cygpath', '-w', include_spec], stdout=subprocess.PIPE, check=True).stdout.decode(sys.stdout.encoding).replace("\n", "")
                                print(f"Mapping {old_include_spec} to {include_spec}")
                            except subprocess.CalledProcessError:
                                print(f"Warning: the directory {include_spec} listed in {file} does not exist")
                        include_file = glob.glob(include_spec+os.sep+'**/'+header_file, recursive=True)
                        if include_file:
                            valid_paths[file] = [version[0], os.path.dirname(include_file[0])]
            return valid_paths

        print("\nReading files...")

        valid_tclConfig_paths = check_config_files(tclConfig_paths, "TCL", "tcl.h")
        valid_tkConfig_paths = check_config_files(tkConfig_paths, "TK", "tk.h")

        print(f"Found {len(valid_tclConfig_paths)} valid Tcl configuration file{'' if len(valid_tclConfig_paths) == 1 else 's'} and {len(valid_tkConfig_paths)} valid Tk configuration file{'' if len(valid_tkConfig_paths) == 1 else 's'}")

    if with_tclsh:
        valid_tclConfig_paths = []
        valid_tkConfig_paths = []
    else:
        search()
    

    def choose_path(config_paths): # If multiple tcl/tkConfig files exist, try to pick one that corresponds to the right version
        chosen_path = list(config_paths)[0]
        if len(config_paths) > 0:
            for path in list(config_paths):
                if version in config_paths[path][0]:
                    chosen_path = path
                    break
        return chosen_path
    
    if len(valid_tclConfig_paths) == 0 or len(valid_tkConfig_paths) == 0:
        if with_tclsh:
            print(f"Using {with_tclsh}.")
            process = return_command(f"{with_tclsh} {GET_PATHS_PATH}")
        else:
            print("\nError: no valid Tcl/Tk configuration files were found. Trying tclsh...")
            process = return_command(f"tclsh {GET_PATHS_PATH}")

        out, err = process.communicate()

        if err:
            raise RuntimeError

        if with_tclsh:
            paths = out.decode().split()
        else:
            paths = paths + out.decode().split()
        paths = list(set(paths))
        paths.sort(key=len)

        tclConfig_paths = []
        tkConfig_paths = []

        search()

    abort = False
    if len(valid_tclConfig_paths) == 0 and len(valid_tkConfig_paths) == 0:
        override = input("Error: no valid Tcl/Tk configuration files found. Press N to override or any other key to abort: ")
        abort = True
    elif len(valid_tclConfig_paths) == 0:
        override = input("Error: no valid Tcl configuration files found. Press N to override or any other key to abort: ")
        abort = True
    elif len(valid_tkConfig_paths) == 0:
        override = input("Error: no valid Tk configuration files found. Press N to override or any other key to abort: ")
        abort = True
    else:
        print("\nChoosing a file...")
        tclConfig_path = choose_path(valid_tclConfig_paths)
        tkConfig_path = choose_path(valid_tkConfig_paths)
        print(f"Using {tclConfig_path} and {tkConfig_path}")

        override = input("Press N to override or any other key to continue: ")

    def manual_choose_path(options):
        if options:
            text = ""
            for index, path in enumerate(options):
                if index == len(options)-2:
                    filler = " or"
                elif index == len(options)-1:
                    filler = ""
                else:
                    filler = ","
                text += f" {index} for {path}{filler}"
            option = input(f"Choose{text}: ")
            try:
                chosen_path = list(options)[int(option)]
                print(f"Using {chosen_path}")
                return chosen_path
            except (ValueError, IndexError):
                print("Invalid selection")
                return manual_choose_path(options)
        else:
            option = input("Please enter the file path: ")
            if os.path.exists(option):
                return option
            else:
                print("File does not exist")
                return manual_choose_path(options)
            
    if override.upper() == "N":
        print("Select a Tcl configuration file to use. ", end="")
        tclConfig_path = manual_choose_path(valid_tclConfig_paths)
        print("Select a Tk configuration file to use. ", end="")
        tkConfig_path = manual_choose_path(valid_tkConfig_paths)
    elif abort:
        sys.exit()

    tclConfig_folder = os.path.dirname(tclConfig_path)
    try:
        tcl_path = valid_tclConfig_paths[tclConfig_path][1]
    except (TypeError, IndexError):
        tcl_path = tclConfig_path
    tkConfig_folder = os.path.dirname(tkConfig_path)
    try:
        tk_path = valid_tkConfig_paths[tkConfig_path][1]
    except (TypeError, IndexError):
        tk_path = tkConfig_path

    if any(" " in path for path in [tclConfig_folder, tkConfig_folder, tcl_path, tk_path]):
        print("Warning: the following Tcl/Tk sources are in a directory that contains spaces:")
        for path in [tclConfig_folder, tkConfig_folder, tcl_path, tk_path]:
            if " " in path:
                print(f"  - {path}")
        print("You will likely encounter errors when trying to compile.")
        print("On Windows, this can happen when MSYS is installed to C:/Program Files/msys/ instead of C:/msys/")
        override = input("Press N to abort or any other key to continue: ")
        if override.upper() == "N":
            sys.exit()

    print("\nUpdating CSS property support...")
    with open(CSSPROP_PATH, "r") as h:
        root.eval(f"cd {{{SRC_PATH}}}\n{h.read()}")

    # print("\nUpdating configure script...")
    # override = input(f"Press U to update or any other key to skip: ")
    # if override.upper() == "U":
    #     os.chdir(BASE_PATH)
    #     rm -r autom4te.cache && autoconf
    #     run_command(["rm", "-r", "autom4te.cache"])
    #     run_command(["autoconf", "-o", CONFIGURE_PATH, CONFIGUREIN_PATH])

    run_command(['chmod', '+rwx', CONFIGURE_PATH])
    os.chdir(BUILD_PATH)

    def compile_tkhtml():
        flags = f"--with-tcl={tclConfig_folder} --with-tk={tkConfig_folder} --with-tclinclude={tcl_path} --with-tkinclude={tk_path}"
        if SYSTEM == "Windows":
            flags += " --with-system=windows"
            if sys.maxsize > 2**32:
                flags += " --with-shlib-ld='gcc -static-libgcc -pipe -shared'"
        elif SYSTEM == "Darwin":
            flags += " --with-system=darwin"

        if disable_cairo:
            flags += " --disable-cairo"

        print(f"Running configure script with the flags {flags}")
        override = input("Press N to add more flags or any other key to continue: ")

        if override.upper() == "N":
            print()
            run_command(["bash", "../configure", '--help'])
            flags += " " + input("Please enter desired flags seperated by a space: ") #I.e. CC="gcc" --pipe --shared CC="gcc -static-libgcc"  SHLIB_LD = gcc -static-libgcc -pipe -shared
            print(f"Running configure script with the flags {flags}")

        try:
            run_command(["bash", "../configure"] + flags.split())
            print("\nCompiling...")
            make()
        except subprocess.CalledProcessError:
            print("Fatal error encountered. Try changing the configure script flags.\n")
            if noprompt: exit()
            compile_tkhtml()

    print("\nCreating Makefile...")
    compile_tkhtml()

test()

if install:
    import tkinterweb_tkhtml
    
    print("\nInstalling...")

    binary = f"libTkhtml{tkhtml_version}"
    if tkinter.TclVersion >= 9:
        binary += "-TclTk9"
    if SYSTEM == "Windows": binary += ".dll"
    elif SYSTEM == "Darwin": binary += ".dylib"
    else: binary += ".so"

    source = os.path.join(BUILD_PATH, tkhtml_file)
    destination = os.path.join(tkinterweb_tkhtml.TKHTML_ROOT_DIR, binary)

    print(f"Copying {binary} to {tkinterweb_tkhtml.TKHTML_ROOT_DIR}")

    if os.path.exists(destination):
        print("Warning: destination file already exists")
        override = input("Press N to abort or any other key to continue: ")
        if override.upper() == "N":
            print("No action done")
        else:
            shutil.copy2(source, destination) 
            print("Successfully overwritten")
    else:
        shutil.copy2(source, destination)
        print("Successfully copied")

    with_tclsh = None
    test()
