from fnmatch import filter as fnfilter
import os

IPTH = os.path.dirname(os.path.realpath(__file__))
IPTH = os.path.join(IPTH, "src")

for i in ("*.c", "*.h"):
    for j in fnfilter(os.listdir(IPTH), i):  # Read the content of the file
        
        fp = os.path.join(IPTH, j)
        print(fp)
        
        with open(fp, "r") as file: content = file.read()

        # Replace tabs
        mod_content = content.replace("\t", " "*4)

        # Write the modified content back to the file
        with open(fp, "w") as file: file.write(mod_content)


input("\nPress ENTER to end")
