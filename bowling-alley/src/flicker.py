# flicker.py — how much does the picture shimmer from one frame to the next?
# For each pair of consecutive frames: take the luma difference, remove its smooth part (real motion and exposure
# drift are smooth or lie on edges), and report the robust spread of what is left, in 8-bit levels.
# usage: python3 src/flicker.py <frames dir or video> first last [x0 y0 x1 y1]
import sys, subprocess, numpy as np
from scipy import ndimage
src, a, b = sys.argv[1], int(sys.argv[2]), int(sys.argv[3])
box = [int(v) for v in sys.argv[4:8]] if len(sys.argv) >= 8 else None
def load(n):
    if src.endswith('.mp4'):
        raw = subprocess.run(['ffmpeg', '-v', 'error', '-ss', '%.4f' % (n / 30.0), '-i', src, '-frames:v', '1', '-f', 'rawvideo', '-pix_fmt', 'gray', '-'], capture_output=True).stdout
        im = np.frombuffer(raw, np.uint8).reshape(1080, 1920).astype(np.float32)
    else:
        from PIL import Image
        im = np.asarray(Image.open('%s/f_%05d.png' % (src, n)).convert('L')).astype(np.float32)
    return im[box[1]:box[3], box[0]:box[2]] if box else im
prev, out, worst = load(a), [], []
for n in range(a + 1, b + 1):
    cur = load(n)
    d = cur - prev
    hp = d - ndimage.gaussian_filter(d, 2.5)
    out.append(1.4826 * np.median(np.abs(hp)))
    h, w = hp.shape
    blk = np.sqrt((hp[:h // 16 * 16, :w // 16 * 16] ** 2).reshape(h // 16, 16, w // 16, 16).mean(axis=(1, 3)))
    worst.append(np.percentile(blk, 95))
    prev = cur
print('%s frames %d-%d%s: shimmer %.2f levels typical, %.2f in the worst 5%% of 16x16 blocks (median of %d frame pairs)' % (src, a, b, ' box %s' % box if box else '', np.median(out), np.median(worst), len(out)))
