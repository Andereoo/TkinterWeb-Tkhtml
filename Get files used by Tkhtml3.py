from fnmatch import fnmatch
from shutil import move
import os

IPTH = os.path.dirname(os.path.realpath(__file__))
SRCDIR = os.path.join(IPTH, "src")
O = "OBSOLETE"
F = "main.mk"

print(IPTH)

with open(os.path.join(IPTH, F)) as file:
    data = [i.split() for i in file.read().replace("\\\n", " ").split("\n") if i and not i[0] == "#"]

    for i in data:
        if i[0] == "SRC":
            used = i[2:]
            break

used.extend(["htmltokens.c", "htmldefaultstyle.c"])

got = [i for i in os.listdir(SRCDIR) if fnmatch(i, "*.c")]
print("Files used by TkHTML-3:")
print(" ".join(got))

unused = set(got) - set(used)

if not os.path.exists(os.path.join(SRCDIR, O)): os.makedirs(os.path.join(SRCDIR, O))

print(f"\nFiles moved to {O}:")

for i in unused:
    move(os.path.join(SRCDIR, i), os.path.join(SRCDIR, O, i))
    print(i)

input("\nPress ENTER to end")
