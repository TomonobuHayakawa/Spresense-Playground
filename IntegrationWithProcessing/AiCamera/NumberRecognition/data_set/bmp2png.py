from PIL import Image
import glob

filenames = glob.glob("*.bmp")

for bmpname in filenames:
	image = Image.open(bmpname)
	pngname = bmpname[:-4] + ".png"
	image.save(pngname)

