# contact sheet: python3 src/sheet.py out.jpg cols thumbW frame1.png frame2.png ...
import sys
from PIL import Image, ImageDraw
out, cols, tw = sys.argv[1], int(sys.argv[2]), int(sys.argv[3])
files = sys.argv[4:]
ims = [Image.open(f).convert('RGB') for f in files]
th = int(tw * ims[0].height / ims[0].width)
rows = (len(ims) + cols - 1) // cols
sheet = Image.new('RGB', (cols * tw, rows * th), (0, 0, 0))
for i, (im, f) in enumerate(zip(ims, files)):
    im = im.resize((tw, th), Image.LANCZOS)
    d = ImageDraw.Draw(im)
    label = f.split('_')[-1].split('.')[0]
    try: label = '%.1fs' % (int(label) / 30.0)
    except: pass
    d.rectangle([0, 0, 44, 12], fill=(0, 0, 0)); d.text((2, 0), label, fill=(255, 255, 0))
    sheet.paste(im, ((i % cols) * tw, (i // cols) * th))
sheet.save(out, quality=88)
