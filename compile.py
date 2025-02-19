import tkinter
import os, glob, subprocess, re, sys
from pathlib import Path

BASE_PATH = os.path.dirname(__file__)
BUILD_PATH =  os.path.join(BASE_PATH, 'build')
CONFIGURE_PATH =  os.path.join(BASE_PATH, 'configure')
SRC_PATH =  os.path.join(BASE_PATH, 'src')
CSSPROP_PATH =  os.path.join(BASE_PATH, 'src', 'cssprop.tcl')

print("Welcome to TkinterWeb's TkHtml3.0 compile script. Note that for this to succeed you will likely need tcl-dev, tk-dev, gcc, and make installed on your system.")

root = tkinter.Tcl()
paths = root.exprstring('$auto_path').split()#tcl_pkgPath
version = str(tkinter.TkVersion)
paths.sort(key=len)

tclConfig_paths = []
tkConfig_paths = []
used_paths = []

print("Searching for Tcl/Tk configuration files...")

for path in paths: 
    if os.path.exists(path):
        may_continue = True
        for i in used_paths:
            if i in path:
                may_continue = False
                break
        if may_continue:
            tclConfig_paths += glob.glob(path+os.sep+'**/*tclConfig.sh', recursive=True)
            tkConfig_paths += glob.glob(path+os.sep+'**/*tkConfig.sh', recursive=True)
            used_paths.append(path)

print(f"Found {len(tclConfig_paths)} Tcl configuration file{'' if len(tclConfig_paths) == 1 else 's'} and {len(tkConfig_paths)} Tk configuration file{'' if len(tkConfig_paths) == 1 else 's'}")

def check_config_files(config_paths, config_type, header_file):
    valid_paths = {}
    for file in config_paths:
        with open(file, "r") as handle:
            content = handle.read()
            version = re.findall(config_type+r"_VERSION='(.*?)'", content, flags=re.MULTILINE)
            include_spec = re.findall(config_type+r"_INCLUDE_SPEC='(.*?)'", content, flags=re.MULTILINE)
            used_paths.append(path)
            if version and include_spec:
                include_spec = include_spec[0].replace("-I", "")
                if not os.path.isdir(include_spec): #msys2
                    print(f"Warning: the directory {include_spec} listed in {file} does not exist")
                    try:
                        old_include_spec = include_spec
                        include_spec = subprocess.run(['cygpath', '-w', include_spec], stdout=subprocess.PIPE, check=True).stdout.decode(sys.stdout.encoding).replace("\n", "")
                        print(f"Mapping {old_include_spec} to {include_spec}")
                    except subprocess.CalledProcessError:
                        pass
                include_file = glob.glob(include_spec+os.sep+'**/'+header_file, recursive=True)
                if include_file:
                    valid_paths[file] = [version[0], os.path.dirname(include_file[0])]
    return valid_paths

def choose_path(config_paths): # If multiple tcl/tkConfig files exist, try to pick one that corresponds to the right version
    chosen_path = list(config_paths)[0]
    if len(config_paths) > 0:
        for path in list(config_paths):
            if version in config_paths[path][0]:
                chosen_path = path
                break
    return chosen_path

print("Reading files..")

valid_tclConfig_paths = check_config_files(tclConfig_paths, "TCL", "tcl.h")
valid_tkConfig_paths = check_config_files(tkConfig_paths, "TK", "tk.h")

print(f"Found {len(valid_tclConfig_paths)} valid Tcl configuration file{'' if len(valid_tclConfig_paths) == 1 else 's'} and {len(valid_tkConfig_paths)} valid Tk configuration file{'' if len(valid_tkConfig_paths) == 1 else 's'}")

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
    print("Choosing a file...")
    tclConfig_path = choose_path(valid_tclConfig_paths)
    tkConfig_path = choose_path(valid_tkConfig_paths)
    print(f"Using {tclConfig_path} and {tkConfig_path}")

    override = input("Press N to override or any other key to continue: ")

def manual_choose_path(options):
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
    
if override.upper() == "N":
    print("Select a Tcl configuration file to use. ", end="")
    tclConfig_path = manual_choose_path(valid_tclConfig_paths)
    print("Select a Tk configuration file to use. ", end="")
    tkConfig_path = manual_choose_path(valid_tkConfig_paths)
elif abort:
    sys.exit()

tclConfig_folder = os.path.dirname(tclConfig_path)
tcl_path = valid_tclConfig_paths[tclConfig_path][1]
tkConfig_folder = os.path.dirname(tkConfig_path)
tk_path = valid_tkConfig_paths[tkConfig_path][1]

###
def print_error(*args):
    print(args)

def run_command(cmd):
    return subprocess.run(cmd, stdout=sys.stdout, stderr=sys.stderr, check=True)

print("\nUpdating CSS property support...")
with open(CSSPROP_PATH, "r") as h:
    root.eval(f"cd {{{SRC_PATH}}}\n{h.read()}")

print("\nCreating build directory...")
if os.path.exists(BUILD_PATH):
    if len(os.listdir(BUILD_PATH)) == 0:
        print('Directory "build" already exists and is empty. Skipping.')
    else:
        print('Directory "build" already exists. Erase contents?')
        override = input(f"Press Y to empty {BUILD_PATH} or any other key to continue: ")
        if override.upper() == "Y":
            files = glob.glob(BUILD_PATH+os.sep+'*')
            for file in files:
                os.remove(file)
else:
    Path(BUILD_PATH).mkdir(parents=True, exist_ok=True)
    print("Done!")

os.chdir(BUILD_PATH)
run_command(['chmod', '+rwx', CONFIGURE_PATH])

def compile_tkhtml():
    print(f"Running configure script with the flags --with-tcl={tclConfig_folder} --with-tk={tkConfig_folder} --with-tclinclude={tcl_path} --with-tkinclude={tk_path}")
    override = input("Press N to add more flags or any other key to continue: ")

    other_flags = ""
    if override.upper() == "N":
        print()
        run_command(["bash", "../configure", '--help'])
        other_flags = " " + input("Please enter desired flags seperated by a space: ") #I.e. CC="gcc" --pipe --shared CC="gcc -static-libgcc"  SHLIB_LD = gcc -static-libgcc -pipe -shared
        print(f"Running configure script with the flags --with-tcl={tclConfig_folder} --with-tk={tkConfig_folder} --with-tclinclude={tcl_path} --with-tkinclude={tk_path}{other_flags}")
    
    try:
        run_command(["bash", "../configure", 
                            f'--with-tcl={tclConfig_folder}', 
                            f'--with-tk={tkConfig_folder}', 
                            f'--with-tclinclude={tcl_path}', 
                            f'--with-tkinclude={tk_path}'] + other_flags.split())
        print("\nCompiling...")
        run_command(["make"])
    except subprocess.CalledProcessError:
        print("Fatal error encountered. Try changing the configure script flags.\n")
        compile_tkhtml()

print("\nCreating Makefile...")
compile_tkhtml()

print("\nOpening test window...")
root = tkinter.Tk()
root.tk.eval("set auto_path [linsert $auto_path 0 {"+BUILD_PATH+"}]")
root.tk.eval("package require Tkhtml")
widget = tkinter.Widget(root, "html")
widget.tk.call(widget._w, "parse", "<p>If you see this, wohoo!!!</p>")
widget.pack(expand=True, fill="both")
root.mainloop()