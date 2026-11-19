# spectrogram of the soundtrack as a PNG (for checking the mix without ears)
import sys, wave
import numpy as np
from scipy import signal
from PIL import Image
w = wave.open(sys.argv[1]); sr = w.getframerate()
x = np.frombuffer(w.readframes(w.getnframes()), dtype=np.int16).reshape(-1, 2).mean(axis=1) / 32768.0
f, t, S = signal.spectrogram(x, sr, nperseg=2048, noverlap=1536)
S = 10 * np.log10(S + 1e-12)
# log-frequency rows from 30 Hz to 12 kHz
rows = 360
fr = np.geomspace(30, 12000, rows)
idx = np.clip(np.searchsorted(f, fr), 0, len(f) - 1)
img = S[idx][::-1]
img = np.clip((img + 110) / 70, 0, 1)
cols = 1680
ci = np.linspace(0, img.shape[1] - 1, cols).astype(int)
img = img[:, ci]
rgb = np.stack([img ** 0.7, img ** 1.4, 0.25 + 0.5 * img ** 3], axis=2)
Image.fromarray((rgb * 255).astype(np.uint8)).save(sys.argv[2])
