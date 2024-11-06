import random
import os

from pathlib import Path

types_list = {0,1,2,3,4,5,6,7,8,9,10};
types_number = 11;
image_number = 100;

filenpath = [];
data_path = "./data"

def make_data_dir(types):
    if not os.path.isdir(data_path):
      os.mkdir("data")

    for i in range(0,types):
      type_path = data_path + "/" + str(i)
      if not os.path.isdir(type_path):
        os.mkdir(type_path)

def write_to_file(types, count,filename):
    with open(filename, 'w') as file:
        file.write("x:image,y:label\n")
        for i in range(0,types):
            type_path = Path(data_path + "/" + str(i))

            for img in type_path.iterdir():
              if img.is_file() and img.suffix.lower() in ".png":
                filenpath.append(f"{type_path}\\{img.name},{i}\n")

        random.shuffle(filenpath)

        for path in filenpath:
            file.write(path)

output_filename = "index.csv"
make_data_dir(types_number)
write_to_file(types_number, image_number, output_filename)

print(f"{output_filename} done.")

