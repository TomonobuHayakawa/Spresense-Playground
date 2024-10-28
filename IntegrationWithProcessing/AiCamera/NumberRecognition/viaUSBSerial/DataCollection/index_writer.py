import random

types_number = 11;
image_number = 100;

filenpath = [];

def write_to_file(base_string, types, count,filename):
    with open(filename, 'w') as file:
        file.write("x:image,y:label\n")
        for i in range(0,types):
            for j in range(0,count):
                filenpath.append(f".\{i}\\{str(j).zfill(3)}.png,{i}\n")

        random.shuffle(filenpath)

        for path in filenpath:
            file.write(path)

output_filename = "index.csv"
write_to_file("item", types_number, image_number, output_filename)

print(f"{output_filename} done.")

