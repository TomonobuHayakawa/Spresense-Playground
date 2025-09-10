from PIL import Image
import random
import os
import sys
from pathlib import Path

directory_name = sys.argv[1] if len(sys.argv) > 1 else "."

types_list = {0,1,2,3,4,5,6,7,8,9,10}
types_number = len(types_list)
data_path = "data"

def make_data_dir(types):
    os.makedirs(os.path.join(directory_name, data_path), exist_ok=True)
    for i in range(types):
        os.makedirs(os.path.join(directory_name, data_path, str(i)), exist_ok=True)

def write_to_file(types, filename):
    filenpath = []
    with open(filename, 'w') as file:
        file.write("x:image,y:label\n")
        for i in range(types):
            type_path = Path(os.path.join(directory_name, data_path, str(i)))
            for img_path in type_path.iterdir():
                if img_path.is_file() and img_path.suffix.lower() == ".bmp":
                    png_path = img_path.with_suffix(".png")
                    image = Image.open(img_path)
                    image.save(png_path)
                    image.close()
                    os.remove(img_path)
                    rel_path = os.path.relpath(png_path, directory_name)
                    filenpath.append(f"{rel_path},{i}\n")
                elif img_path.is_file() and img_path.suffix.lower() == ".png":
                    rel_path = os.path.relpath(img_path, directory_name)
                    filenpath.append(f"{rel_path},{i}\n")
        random.shuffle(filenpath)
        file.writelines(filenpath)

output_filename = os.path.join(directory_name, "index.csv")
make_data_dir(types_number)
write_to_file(types_number, output_filename)
print(f"{output_filename} done.")
