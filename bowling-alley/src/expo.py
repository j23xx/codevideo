# expo.py — turn the metering pre-pass into a lagging auto-exposure curve (one multiplier per frame).
# usage: python3 src/expo.py work/meter.txt work/expo.txt nframes fps
import sys, math
src, dst, n, fps = sys.argv[1], sys.argv[2], int(sys.argv[3]), float(sys.argv[4])
pts = sorted((int(a), float(b)) for a, b in (l.split() for l in open(src) if l.strip()))
def meter(f):
    if f <= pts[0][0]: return pts[0][1]
    if f >= pts[-1][0]: return pts[-1][1]
    for (f0, m0), (f1, m1) in zip(pts, pts[1:]):
        if f0 <= f <= f1:
            t = (f - f0) / max(f1 - f0, 1)
            return math.exp(math.log(m0) * (1 - t) + math.log(m1) * t)
KEY, LO, HI = 0.155, 0.8, 7.0
ev = None
out = []
for f in range(n):
    target = math.log(min(max(KEY / max(meter(f), 1e-4), LO), HI))
    if ev is None: ev = target
    # the camera stops down quickly when it gets brighter and opens up more slowly in the dark
    tau = 0.30 if target < ev else 0.65
    ev += (target - ev) * (1 - math.exp(-1.0 / (fps * tau)))
    out.append(math.exp(ev))
open(dst, 'w').write('\n'.join('%.5f' % v for v in out) + '\n')
print('exposure: min %.2f max %.2f' % (min(out), max(out)))
