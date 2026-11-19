# audio.py — the soundtrack, synthesised from the renderer's event timeline with numpy/scipy only
# (no samples, no recordings): the evening outside, the alley's room tone and jukebox, the ball, every pin
# impact from the physics simulation (stretched and pitched down through the bullet-time passage),
# and the pinsetter machinery.
# usage: python3 src/audio.py work/timeline.json work/audio.wav
import sys, json, wave
import numpy as np
from scipy import signal

SR = 48000
tl = json.load(open(sys.argv[1]))
OUT = sys.argv[2]
LEN = tl['film_len']
N = int(LEN * SR)
t = np.arange(N) / SR
rng = np.random.default_rng(1964)

# ---------------------------------------------------------------- timeline curves at audio rate
rate = np.interp(t, np.arange(len(tl['rate'])) / 100.0, tl['rate'])
cam = np.array(tl['cam'])                       # 50 Hz: x y z speed bx by bz ballVisible
ct = np.arange(len(cam)) / 50.0
camx, camy, camz = (np.interp(t, ct, cam[:, i]) for i in range(3))
speed = np.interp(t, ct, cam[:, 3])
bx, by, bz = (np.interp(t, ct, cam[:, i]) for i in (4, 5, 6))
bvis = np.interp(t, ct, cam[:, 7])
T_REL, T_IMP, T_CUSH = tl['t_rel'], tl['t_impact'], tl['t_cushion']
M = tl['machine']
HX = tl['hero_x']
world = np.cumsum(rate) / SR                    # world seconds elapsed at each sample


def smooth(x, sec):
    n = max(int(sec * SR), 1)
    k = np.hanning(2 * n + 1); k /= k.sum()
    return np.convolve(np.pad(x, n, mode='edge'), k, mode='valid')


def sstep(a, b, x):
    u = np.clip((x - a) / (b - a), 0, 1)
    return u * u * (3 - 2 * u)


def bp(x, lo, hi, order=2):
    sos = signal.butter(order, [lo, hi], btype='band', fs=SR, output='sos')
    return signal.sosfilt(sos, x)


def lp(x, f, order=2):
    return signal.sosfilt(signal.butter(order, f, btype='low', fs=SR, output='sos'), x)


def hp(x, f, order=2):
    return signal.sosfilt(signal.butter(order, f, btype='high', fs=SR, output='sos'), x)


def varlp(x, cutoff):
    """one-pole low-pass with a time-varying cutoff (Hz), processed in short blocks"""
    y = np.empty_like(x); s = 0.0
    B = 256
    for i in range(0, len(x), B):
        a = 1 - np.exp(-2 * np.pi * float(cutoff[i]) / SR)
        seg = x[i:i + B]
        # y[n] = y[n-1] + a (x[n] - y[n-1])  == lfilter([a], [1, a-1])
        out, zf = signal.lfilter([a], [1, a - 1], seg, zi=[s * (1 - a)])
        y[i:i + B] = out; s = out[-1]
    return y


inside = sstep(-26.2, -24.6, camz)                       # 0 outside, 1 in the building
inMachine = sstep(17.3, 18.6, camz) * sstep(2.3, 1.9, camy)
slow = 1 - np.clip(rate, 0, 1)                           # 1 in deep bullet time

# ---------------------------------------------------------------- 1. (no drone)
# The camera is silent: the motors and prop wash of the first version were removed on request, so what is
# heard is the place itself.

# ---------------------------------------------------------------- 2. ambience outside / inside
pink = signal.lfilter([0.05], [1, -0.95], rng.standard_normal(N))
wind = lp(pink, 400) * (1.0 + 0.5 * np.sin(2 * np.pi * 0.11 * t + 1)) * 0.5
# evening birds in the palms: short downward chirps in loose phrases, a couple of voices
crick = np.zeros(N)
brng = np.random.default_rng(77)
for voice, (f_hi, f_lo, gain) in enumerate(((5200, 3300, 0.016), (4100, 2900, 0.012), (6100, 4400, 0.009))):
    tb = 0.4 + voice * 0.7
    while tb < 6.5:
        nch = int(brng.integers(2, 6))
        for k in range(nch):
            d = float(brng.uniform(0.045, 0.085)); n = int(d * SR); i0 = int((tb + k * brng.uniform(0.11, 0.16)) * SR)
            if i0 + n >= N: break
            u = np.arange(n) / n
            f = f_hi + (f_lo - f_hi) * u ** 0.7 + 180 * np.sin(2 * np.pi * 3 * u)
            crick[i0:i0 + n] += np.sin(2 * np.pi * np.cumsum(f) / SR) * np.sin(np.pi * u) ** 2 * gain * brng.uniform(0.6, 1.0)
        tb += float(brng.uniform(0.9, 2.2))
neonGain = np.clip(1.0 - (np.abs(camz + 25.3) / 22.0), 0, 1) ** 2
neon = sum(np.sin(2 * np.pi * 120 * k * t + k) / k for k in (1, 2, 3, 5, 7)) * 0.012
neon += bp(rng.standard_normal(N), 3000, 7000) * 0.004 * (np.mod(t * 120, 1) < 0.3)
traffic = lp(signal.lfilter([0.02], [1, -0.98], rng.standard_normal(N)), 160) * 2.0
outside = (wind * 0.2 + crick * 1.3 + traffic * 0.18) * (1 - inside) + neon * neonGain * (1 - 0.6 * inside)

roomtone = lp(pink, 220) * 0.09 + sum(np.sin(2 * np.pi * 60 * k * t + k * 0.7) / (k * k) for k in (1, 2, 3, 4)) * 0.006
flu = (np.sin(2 * np.pi * 120 * t) * 0.004 + bp(rng.standard_normal(N), 5000, 9000) * 0.0015)
interior = (roomtone + flu) * inside

# ---------------------------------------------------------------- 3. the jukebox: a little surf instrumental
def pluck(freq, dur, bright=0.5, decay=0.996):
    """Karplus-Strong string, vectorised one period at a time"""
    P = max(int(SR / freq), 2)
    n = int(dur * SR)
    buf = rng.uniform(-1, 1, P)
    buf = signal.lfilter([bright], [1, bright - 1], buf)       # softer pick for darker notes
    buf -= buf.mean()
    out = np.empty(n + P)
    out[:P] = buf
    prev = buf
    last = buf[-1]
    for i in range(P, n + P, P):
        sh = np.concatenate(([last], prev[:-1]))
        cur = decay * 0.5 * (prev + sh)
        m = min(P, n + P - i)
        out[i:i + m] = cur[:m]
        last = prev[-1]; prev = cur
    return out[:n]


BPM = 132.0
beat = 60.0 / BPM
MLEN = LEN + 6
music = np.zeros(int(MLEN * SR))


def put(buf, tpos, x, g=1.0):
    i = int(tpos * SR)
    if i >= len(buf) or i < 0: return
    m = min(len(x), len(buf) - i)
    buf[i:i + m] += x[:m] * g


NOTE = {'C': 0, 'D': 2, 'E': 4, 'F': 5, 'G': 7, 'A': 9, 'B': 11}
def hz(name, octave): return 440.0 * 2 ** ((NOTE[name] + 12 * (octave - 4) - 9) / 12.0)
prog = [('A', 'minor'), ('F', 'major'), ('C', 'major'), ('G', 'major')]      # Am F C G, one bar each
lead_lines = [                                                               # (beat offset, note, octave, beats)
    [(0, 'A', 4, 1), (1, 'C', 5, 0.5), (1.5, 'E', 5, 1.5), (3, 'D', 5, 0.5), (3.5, 'C', 5, 0.5)],
    [(0, 'A', 4, 1.5), (1.5, 'C', 5, 0.5), (2, 'F', 5, 1), (3, 'E', 5, 1)],
    [(0, 'G', 4, 1), (1, 'C', 5, 0.5), (1.5, 'E', 5, 1.5), (3, 'G', 5, 1)],
    [(0, 'D', 5, 1.5), (1.5, 'B', 4, 0.5), (2, 'G', 4, 1), (3, 'B', 4, 0.5), (3.5, 'D', 5, 0.5)],
]
nbars = int(MLEN / (4 * beat)) + 1
for bar in range(nbars):
    root, quality = prog[bar % 4]
    t0 = bar * 4 * beat
    third = 3 if quality == 'minor' else 4
    r = hz(root, 2)
    # bass: root - fifth walking
    for b, mult in ((0, 1.0), (1, 1.0), (2, 1.5), (3, 1.0 if bar % 2 else 2.0)):
        put(music, t0 + b * beat, lp(pluck(r * mult, 0.42, 0.25, 0.992), 500) * np.exp(-np.arange(int(0.42 * SR)) / (0.25 * SR)), 0.9)
    # rhythm guitar chops on the off-beats, with tremolo
    for b in (0.5, 1.5, 2.5, 3.5):
        ch = sum(pluck(hz(root, 3) * 2 ** (s / 12.0), 0.22, 0.7, 0.99) for s in (0, third, 7, 12))
        ch *= np.exp(-np.arange(len(ch)) / (0.07 * SR))
        put(music, t0 + b * beat, ch, 0.16)
    # twangy lead with vibrato arm dips
    if bar >= 2:
        for off, nm, octv, ln in lead_lines[bar % 4]:
            d = ln * beat + 0.25
            x = pluck(hz(nm, octv), d, 0.9, 0.9975)
            trem = 1 + 0.25 * np.sin(2 * np.pi * 6.2 * np.arange(len(x)) / SR)
            put(music, t0 + off * beat, x * trem, 0.30)
    # drums: kick, snare on 2 and 4, ride
    for b in range(4):
        n = int(0.16 * SR); tt = np.arange(n) / SR
        if b in (0, 2) or (b == 3 and bar % 4 == 3):
            put(music, t0 + b * beat, np.sin(2 * np.pi * (48 * tt + 60 * (1 - np.exp(-tt * 28)) / 28)) * np.exp(-tt * 22), 0.8)
        if b in (1, 3):
            sn = bp(rng.standard_normal(n), 900, 6500) * np.exp(-tt * 26) + 0.5 * np.sin(2 * np.pi * 190 * tt) * np.exp(-tt * 40)
            put(music, t0 + b * beat, sn, 0.33)
        for sub in (0, 0.5):
            n2 = int(0.05 * SR)
            put(music, t0 + (b + sub * (1.25 if sub else 0)) * beat, hp(rng.standard_normal(n2), 6500) * np.exp(-np.arange(n2) / (0.012 * SR)), 0.06 if sub else 0.09)
# spring-reverb-ish smear and the jukebox's small-speaker colour
ir_n = int(0.5 * SR)
spring = rng.standard_normal(ir_n) * np.exp(-np.arange(ir_n) / (0.12 * SR)); spring[0] = 0
music = music + 0.10 * signal.fftconvolve(music, bp(spring, 700, 3500), mode='full')[:len(music)]
music = np.tanh(bp(music, 110, 5200) * 1.6) * 0.5
# tape-slowdown through bullet time: read the tune with a warped clock
mclock = np.cumsum(rate ** 0.45) / SR
music_w = np.interp(mclock, np.arange(len(music)) / SR, music)
jx, jz = -3.3, -12.3
dj = np.sqrt((camx - jx) ** 2 + (camz - jz) ** 2 + (camy - 1.0) ** 2)
mgain = 1.0 / (1.0 + dj / 2.2) ** 1.25
mcut = 7000.0 / (1.0 + dj / 7.0)
mgain = mgain * (0.12 + 0.88 * inside) * (1 - 0.55 * inMachine)
mcut = mcut * (0.06 + 0.94 * inside) * (1 - 0.6 * inMachine) * (0.35 + 0.65 * rate ** 0.5)
jukebox = varlp(music_w, np.maximum(mcut, 160)) * mgain * 2.1

# ---------------------------------------------------------------- 4. the ball
wn = int((world[-1] + 2) * SR)
roll_src = bp(rng.standard_normal(wn), 45, 420) + 0.5 * bp(rng.standard_normal(wn), 700, 2400) * 0.25
roll_src *= 1 + 0.4 * np.sin(2 * np.pi * 12.5 * np.arange(wn) / SR)        # once-per-revolution thump
roll = np.interp(world, np.arange(wn) / SR, roll_src)                      # slowed (and so lowered) with the picture
t_rel_i, t_imp_i, t_cu_i = int(T_REL * SR), int(T_IMP * SR), int(T_CUSH * SR)
dball = np.sqrt((camx - bx) ** 2 + (camy - by) ** 2 + (camz - bz) ** 2)
rollEnv = sstep(T_REL - 0.42, T_REL - 0.30, t) * (1 - sstep(T_CUSH - 0.05, T_CUSH + 0.02, t)) * bvis
rollEnv *= 1 - 0.75 * sstep(T_IMP, T_IMP + 1.5, t) * (bz > 19.1)           # quieter once it drops onto the pit carpet
ballsnd = roll * rollEnv * (1.1 / (0.6 + dball)) * (0.4 + 0.6 * rate ** 0.3) * 1.0
# the ball landing on the approach behind us
n = int(0.5 * SR); tt = np.arange(n) / SR
thud = np.sin(2 * np.pi * (55 * tt + 40 * (1 - np.exp(-tt * 30)) / 30)) * np.exp(-tt * 14) + bp(rng.standard_normal(n), 200, 1500) * np.exp(-tt * 45) * 0.5
events = np.zeros(N + SR * 6)
put(events, T_REL - 0.44, thud, 0.55)

# ---------------------------------------------------------------- 5. impacts from the simulation
def stretch(x, s):
    if s <= 1.02: return x
    n2 = int(len(x) * s)
    return np.interp(np.arange(n2) / s, np.arange(len(x)), x)


def grain(kind, strength):
    n = int(0.22 * SR); tt = np.arange(n) / SR
    if kind == 0:      # ball on pin: heavy knock with a bright crack
        x = np.sin(2 * np.pi * 150 * tt) * np.exp(-tt * 26) * 1.2
        x += sum(np.sin(2 * np.pi * f * tt + p) * np.exp(-tt * d) for f, d, p in ((880, 55, 0), (1710, 70, 1), (2950, 95, 2))) * 0.5
        x += bp(rng.standard_normal(n), 1500, 8000) * np.exp(-tt * 160) * 0.8
    elif kind == 1:    # pin on pin: hollow maple clack
        f0 = rng.uniform(950, 1350)
        x = sum(np.sin(2 * np.pi * f0 * m * tt + m) * np.exp(-tt * (60 + 25 * m)) / m for m in (1, 2.27, 3.6))
        x += bp(rng.standard_normal(n), 2000, 9000) * np.exp(-tt * 220) * 0.6
    elif kind == 2:    # pin on the deck, kickback or pit
        f0 = rng.uniform(420, 700)
        x = np.sin(2 * np.pi * f0 * tt) * np.exp(-tt * 48) + 0.6 * np.sin(2 * np.pi * 115 * tt) * np.exp(-tt * 30)
        x += bp(rng.standard_normal(n), 800, 5000) * np.exp(-tt * 140) * 0.5
    else:              # ball into the pit cushion
        x = np.sin(2 * np.pi * (52 * tt + 30 * (1 - np.exp(-tt * 20)) / 20)) * np.exp(-tt * 9) * 1.6
        x += lp(rng.standard_normal(n), 500) * np.exp(-tt * 30) * 0.8
    return x * min(strength, 7.0) / 7.0


evlist = []
for tv, kind, strength, ex, ey, ez, r in tl['events']:
    if tv < 0 or tv > LEN: continue
    ci = min(int(tv * SR), N - 1)
    d = np.sqrt((camx[ci] - ex) ** 2 + (camy[ci] - ey) ** 2 + (camz[ci] - ez) ** 2)
    s = (1.0 / max(r, 0.02)) ** 0.5                       # long and low in bullet time
    g = grain(int(kind), strength)
    x = stretch(g, min(s, 7.0))
    gain = (1.0 / (0.5 + d)) * (0.5 if kind == 1 or kind == 2 else (0.9 if kind == 0 else 2.6)) * (1.0 + 0.5 * min(s, 4) / 4)
    put(events, tv, x, gain)
    evlist.append((tv, kind, strength))
# the hit itself gets a sub-bass bloom, and time bending has its own sound
n = int(3.0 * SR); tt = np.arange(n) / SR
boom = np.sin(2 * np.pi * (34 * tt + 26 * (1 - np.exp(-tt * 3)) / 3)) * np.exp(-tt * 1.6) * sstep(0, 0.01, tt)
put(events, T_IMP, boom, 0.55)
t_slow0 = next(i for i in range(len(tl['rate'])) if tl['rate'][i] < 0.95) / 100.0
t_slow1 = max(i for i in range(len(tl['rate'])) if tl['rate'][i] < 0.5) / 100.0
n = int(0.9 * SR); tt = np.arange(n) / SR
down = np.sin(2 * np.pi * np.cumsum(700 * np.exp(-tt * 3.2) + 60) / SR) * np.exp(-tt * 2.5) * sstep(0, 0.05, tt)
put(events, t_slow0 - 0.05, down + bp(rng.standard_normal(n), 300, 3000) * np.exp(-tt * 4) * 0.3, 0.16)
n = int(1.4 * SR); tt = np.arange(n) / SR
up = bp(rng.standard_normal(n), 200, 5000) * (tt / 1.4) ** 3 * 0.5 + np.sin(2 * np.pi * np.cumsum(60 + 500 * (tt / 1.4) ** 3) / SR) * (tt / 1.4) ** 2 * 0.4
put(events, t_slow1 - 1.0, up * sstep(1.4, 1.3, tt), 0.22)
# a held, slowly beating tone under the frozen moment
hold = (np.sin(2 * np.pi * 55 * t) + 0.6 * np.sin(2 * np.pi * 82.6 * t + 1) + 0.3 * np.sin(2 * np.pi * 110.4 * t)) * 0.035
hold *= smooth(slow ** 2, 0.15) * (1 + 0.3 * np.sin(2 * np.pi * 0.7 * t))

# ---------------------------------------------------------------- 6. the pinsetter
mach = np.zeros(N + SR * 6)
def clunk(tv, g=1.0, f=95):
    n = int(0.35 * SR); tt = np.arange(n) / SR
    x = np.sin(2 * np.pi * f * tt) * np.exp(-tt * 18) + bp(rng.standard_normal(n), 300, 3500) * np.exp(-tt * 60) * 0.7 + np.sin(2 * np.pi * 730 * tt) * np.exp(-tt * 35) * 0.25
    put(mach, tv, x, g)
def motor(t0, t1, f=58, g=1.0):
    n = int((t1 - t0 + 0.3) * SR); tt = np.arange(n) / SR
    env = sstep(0, 0.15, tt) * sstep(t1 - t0 + 0.3, t1 - t0 + 0.05, tt)
    fr = f * (0.6 + 0.4 * sstep(0, 0.25, tt))
    ph = 2 * np.pi * np.cumsum(fr) / SR
    x = sum(np.sin(k * ph + k) / k for k in range(1, 9)) * 0.35 + bp(rng.standard_normal(n), 400, 2500) * 0.25 * (1 + 0.5 * np.sin(2 * np.pi * 17 * tt))
    put(mach, t0, x * env, g * 0.5)
clunk(M['sweep_down'] + 0.25, 0.9, 80); motor(M['sweep_down'], M['sweep_down'] + 0.3, 70, 0.6)
motor(M['sweep_run'], M['sweep_run'] + 1.0, 62, 0.8); clunk(M['sweep_run'] + 1.0, 0.7, 110)
motor(M['sweep_back'], M['sweep_back'] + 0.9, 62, 0.6); clunk(M['sweep_back'] + 0.9, 0.6)
motor(M['table_down'], M['table_down'] + 1.0, 48, 0.9); clunk(M['table_set'], 1.0, 70)
for k in range(10):                                           # ten pins touching down
    put(mach, M['table_set'] - 0.06 + 0.012 * k + rng.uniform(0, 0.02), grain(2, 2.0), 0.35)
motor(M['table_up'], M['table_up'] + 0.9, 52, 0.8); clunk(M['table_up'] + 0.9, 0.8, 90)
motor(M['sweep_up'], M['sweep_up'] + 0.4, 70, 0.6); clunk(M['sweep_up'] + 0.4, 0.8, 100)
# pins dropping from the elevator onto the belt and into the turret, about one a second
tp = M['sweep_run'] + 0.4
while tp < LEN:
    put(mach, tp, grain(1, 2.5), 0.30)
    put(mach, tp + 0.55, grain(2, 2.0), 0.22)
    tp += 1.12
machGain = (0.10 + 0.90 * smooth(inMachine, 0.2)) * sstep(T_IMP + 2.0, T_IMP + 4.5, t)
# running gear: motor hum, belt slap, chain, the wheel's rumble — only present near the machine
hum = sum(np.sin(2 * np.pi * 120 * k * t + 0.3 * k) / k ** 1.3 for k in range(1, 8)) * 0.05
belt = bp(rng.standard_normal(N), 600, 2600) * (0.5 + 0.5 * np.sign(np.sin(2 * np.pi * 7.5 * t))) * 0.03
chain = bp(rng.standard_normal(N), 2500, 7000) * (np.mod(t * 31, 1) < 0.25) * 0.02
rumble = lp(rng.standard_normal(N), 90) * 0.2
running = (hum + belt + chain + rumble) * smooth(inMachine, 0.25) * (0.25 + 0.75 * rate)
machine = mach[:N] * machGain + running

# ---------------------------------------------------------------- 7. room, stereo, mix
def reverb(x, rt, wet):
    n = int(rt * 1.2 * SR)
    ir = rng.standard_normal(n) * np.exp(-np.arange(n) / (rt / 6.9 * SR))
    ir = lp(ir, 5000); ir[:int(0.012 * SR)] = 0
    ir /= np.sqrt(np.sum(ir ** 2))
    return x + wet * signal.fftconvolve(x, ir, mode='full')[:len(x)]

ev = events[:N]
hall = reverb(ev * 0.9 + ballsnd + jukebox * 0.6 + machine * 0.6, 1.5, 0.22)
dry = jukebox * 0.4 + machine * 0.4 + ev * 0.25
mid = hall + dry + interior + outside + hold
# decorrelated sides: ambience spreads a little, impacts pan with their position
def spread(x, ms):
    d = int(ms * SR / 1000)
    return np.concatenate((np.zeros(d), x[:-d]))
side = 0.35 * (spread(outside + interior, 11) - (outside + interior)) + 0.3 * (spread(hall, 17) - hall) * 0.5
L = mid + side
R = mid - side
env = sstep(0.0, 0.8, t) * sstep(LEN, LEN - 1.0, t)
st = np.stack([L, R], axis=1) * env[:, None]
st -= st.mean(axis=0)
st = signal.sosfilt(signal.butter(2, 32, btype='high', fs=SR, output='sos'), st, axis=0)
# gentle bus compression and a ceiling
peak = np.max(np.abs(st))
st = st / peak * 3.0
st = np.tanh(st) * 0.89
w = wave.open(OUT, 'wb'); w.setnchannels(2); w.setsampwidth(2); w.setframerate(SR)
w.writeframes((st * 32767).astype(np.int16).tobytes()); w.close()

# ---------------------------------------------------------------- report
def db(x): return 20 * np.log10(max(x, 1e-9))
mono = st.mean(axis=1)
print('audio: %.2f s, peak %.1f dBFS, rms %.1f dBFS, clipped samples %d, L/R correlation %.2f' % (
    len(st) / SR, db(np.max(np.abs(st))), db(np.sqrt(np.mean(mono ** 2))), int(np.sum(np.abs(st) >= 0.999)), np.corrcoef(st[:, 0], st[:, 1])[0, 1]))
def rise(tv, pre=0.25, post=0.25):
    i = int(tv * SR)
    a = np.sqrt(np.mean(mono[max(i - int(pre * SR), 0):i] ** 2)); b = np.sqrt(np.mean(mono[i:i + int(post * SR)] ** 2))
    return db(b) - db(a)
print('onset rise: ball lands %.1f dB, impact %.1f dB, cushion %.1f dB, table set %.1f dB' % (rise(T_REL - 0.44), rise(T_IMP), rise(T_CUSH), rise(M['table_set'])))
for name, a, b in (('outside', 0.5, 4.0), ('lobby', 6, 14), ('chase', T_REL, T_IMP - 0.3), ('bullet time', T_IMP, t_slow1), ('machine', t_slow1, 33), ('return', 34.5, 41)):
    seg = mono[int(a * SR):int(b * SR)]
    print('  %-12s %5.1f-%5.1f s  rms %.1f dBFS' % (name, a, b, db(np.sqrt(np.mean(seg ** 2)))))
print('events placed: %d (ball-pin %d, pin-pin %d, pin-surface %d, cushion %d)' % (len(evlist), sum(1 for e in evlist if e[1] == 0), sum(1 for e in evlist if e[1] == 1), sum(1 for e in evlist if e[1] == 2), sum(1 for e in evlist if e[1] == 3)))
