import os
from pathlib import Path

SEARCH_STRING = "something"
CASE_SENSITIVE = False

BASE_PATH = os.path.dirname(__file__)
SRC_PATH =  os.path.join(BASE_PATH, 'src')

dir_content = sorted(Path(SRC_PATH).iterdir())

found_list = []
for path in dir_content: 
    if not path.is_dir():
        if (CASE_SENSITIVE and SEARCH_STRING in path.read_text()) or (not CASE_SENSITIVE and SEARCH_STRING.lower() in path.read_text().lower()):
            found_list.append(path)

if found_list:
    print(f"Found string '{SEARCH_STRING}' in {len(found_list)} file{"s" if len(found_list) else ""}:")
    for path in found_list:
        print(f"  {path}")
else:
    print(f"No occurences for '{SEARCH_STRING}' found")