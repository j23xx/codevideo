# qc.py — decode the finished film and look for bad frames: black, frozen, or single-frame outliers.
# usage: python3 src/qc.py film.mp4
import sys, subprocess, json
import numpy as np
path = sys.argv[1]
info = json.loads(subprocess.run(['ffprobe', '-v', 'error', '-show_streams', '-show_format', '-of', 'json', path], capture_output=True, text=True).stdout)
for s in info['streams']:
    if s['codec_type'] == 'video':
        print('video: %s %s %sx%s %s fps, %s frames, %.3f s, pix_fmt %s' % (s['codec_name'], s.get('profile'), s['width'], s['height'], s['r_frame_rate'], s.get('nb_frames'), float(s['duration']), s['pix_fmt']))
    else:
        print('audio: %s %s Hz %s ch, %.3f s' % (s['codec_name'], s['sample_rate'], s['channels'], float(s['duration'])))
print('file: %.1f MB, overall bitrate %.1f Mbps' % (float(info['format']['size']) / 1e6, float(info['format']['bit_rate']) / 1e6))
W, H = 320, 180
p = subprocess.Popen(['ffmpeg', '-v', 'error', '-i', path, '-vf', 'scale=%d:%d' % (W, H), '-f', 'rawvideo', '-pix_fmt', 'gray', '-'], stdout=subprocess.PIPE)
frames = []
while True:
    b = p.stdout.read(W * H)
    if len(b) < W * H: break
    frames.append(np.frombuffer(b, dtype=np.uint8).astype(np.float32))
n = len(frames)
F = np.stack(frames).reshape(n, H, W)
mean = F.mean(axis=(1, 2))
fps = 30.0
fade_in, fade_out = int(0.8 * fps), n - int(1.0 * fps)
black = [i for i in range(n) if mean[i] < 6 and fade_in < i < fade_out]
diff = np.abs(F[1:] - F[:-1]).mean(axis=(1, 2))
frozen = [i + 1 for i in range(n - 1) if diff[i] < 0.02 and fade_in < i < fade_out]
# "lost tile": a 20x20 block that jumps for exactly one frame while its neighbours in time agree
bs = 20
B = F.reshape(n, H // bs, bs, W // bs, bs).mean(axis=(2, 4))
tile = []
for i in range(1, n - 1):
    d_prev = np.abs(B[i] - B[i - 1]); d_next = np.abs(B[i] - B[i + 1]); d_around = np.abs(B[i + 1] - B[i - 1])
    bad = (d_prev > 40) & (d_next > 40) & (d_around < 8)
    if bad.any(): tile.append((i, int(bad.sum())))
# whole-frame flashes: mean level that departs from both neighbours
flash = [i for i in range(1, n - 1) if abs(mean[i] - mean[i - 1]) > 18 and abs(mean[i] - mean[i + 1]) > 18 and abs(mean[i + 1] - mean[i - 1]) < 6]
print('decoded frames: %d' % n)
print('black frames outside the fades: %d %s' % (len(black), black))
print('frozen frames: %d %s' % (len(frozen), frozen))
print('single-frame block outliers: %d %s' % (len(tile), tile))
print('single-frame brightness flashes: %d %s' % (len(flash), flash))
print('mean level by 3 s window:', ' '.join('%.0f' % mean[i:i + 90].mean() for i in range(0, n, 90)))
