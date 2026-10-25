"""Writes three placeholder felling sounds (44.1 kHz mono 16-bit WAV) into Saved/AudioSrc, synthesised in plain Python:
- SW_AxeChop.wav: a short wooden thunk (0.35 s): low decaying tones + a filtered noise burst.
- SW_TreeCrack.wav: the trunk giving way (1.6 s): crackles getting denser and louder, under a low creak.
- SW_TreeFall.wav: the tree hitting the ground (1.8 s): a heavy low thud + rustling leaves dying away.
- SW_PickaxeHit.wav: steel on rock (0.4 s): bright inharmonic ring + a dull knock.
- SW_RockBreak.wav: a rock splitting (1.3 s): a sharp crack, then crumbling pieces and a low rumble.
- SW_StoneCarve.wav: a chisel strike (0.18 s): a short high tick with grit.
Placeholders until real recordings replace them (same asset names under /Game/Audio/Felling).
Run outside the editor: python Scripts/make_felling_sounds.py"""
import math, os, random, struct, wave
RATE = 44100
OUT = os.path.join(os.path.dirname(__file__), '..', 'Saved', 'AudioSrc')
random.seed(3)

def lowpass(x, a):
    y, prev = [], 0.0
    for v in x:
        prev += a * (v - prev); y.append(prev)
    return y

def write(name, x, gain=0.9):
    peak = max(1e-6, max(abs(v) for v in x))
    with wave.open(os.path.join(OUT, name), 'wb') as w:
        w.setnchannels(1); w.setsampwidth(2); w.setframerate(RATE)
        w.writeframes(b''.join(struct.pack('<h', int(max(-1, min(1, v / peak * gain)) * 32767)) for v in x))

def chop():
    n = int(0.35 * RATE)
    noise = lowpass([random.uniform(-1, 1) for _ in range(n)], 0.25)
    out = []
    for i in range(n):
        t = i / RATE
        body = 0.8 * math.sin(2 * math.pi * 140 * t) * math.exp(-t * 28) + 0.5 * math.sin(2 * math.pi * 310 * t) * math.exp(-t * 45)
        click = noise[i] * math.exp(-t * 60) * 1.4
        out.append(body + click)
    return out

def crack():
    n = int(1.6 * RATE)
    out = [0.0] * n
    creak_noise = lowpass([random.uniform(-1, 1) for _ in range(n)], 0.02)
    for i in range(n):
        t = i / RATE
        f = 95 + 30 * math.sin(2 * math.pi * 1.7 * t) + 40 * t
        out[i] += 0.35 * math.sin(2 * math.pi * f * t + 3 * creak_noise[i]) * min(1, t * 3) * (1 - t / 1.6) ** 0.5
    t0 = 0.0
    while t0 < 1.5:
        density = 0.12 * (1 - t0 / 1.6) ** 2 + 0.008
        t0 += random.expovariate(1 / density)
        start = int(t0 * RATE); amp = 0.3 + 0.9 * (t0 / 1.6) ** 1.5
        length = int(random.uniform(0.004, 0.02) * RATE)
        burst = lowpass([random.uniform(-1, 1) for _ in range(length)], random.uniform(0.3, 0.8))
        for k in range(length):
            if start + k < n:
                out[start + k] += amp * burst[k] * math.exp(-k / (length / 3))
    return out

def fall():
    n = int(1.8 * RATE)
    rustle = lowpass([random.uniform(-1, 1) for _ in range(n)], 0.35)
    rumble = lowpass([random.uniform(-1, 1) for _ in range(n)], 0.01)
    out = []
    for i in range(n):
        t = i / RATE
        thud = (1.0 * math.sin(2 * math.pi * 52 * t) + 0.6 * math.sin(2 * math.pi * 78 * t)) * math.exp(-t * 6) + 2.5 * rumble[i] * math.exp(-t * 4)
        leaves = 0.35 * rustle[i] * math.exp(-t * 1.8) * min(1, t * 20)
        out.append(thud + leaves)
    return out

def pickaxe():
    n = int(0.4 * RATE)
    noise = lowpass([random.uniform(-1, 1) for _ in range(n)], 0.6)
    out = []
    for i in range(n):
        t = i / RATE
        ring = sum(a * math.sin(2 * math.pi * f * t) for f, a in ((2310, 0.5), (3470, 0.35), (5120, 0.2))) * math.exp(-t * 18)
        knock = 0.7 * math.sin(2 * math.pi * 180 * t) * math.exp(-t * 40) + noise[i] * math.exp(-t * 90)
        out.append(ring + knock)
    return out

def rock_break():
    n = int(1.3 * RATE)
    out = [0.0] * n
    crack_noise = lowpass([random.uniform(-1, 1) for _ in range(n)], 0.7)
    rumble = lowpass([random.uniform(-1, 1) for _ in range(n)], 0.015)
    for i in range(n):
        t = i / RATE
        out[i] = 1.6 * crack_noise[i] * math.exp(-t * 35) + 2.0 * rumble[i] * math.exp(-t * 3) + 0.6 * math.sin(2 * math.pi * 70 * t) * math.exp(-t * 8)
    t0 = 0.08
    while t0 < 1.1:
        t0 += random.expovariate(1 / 0.045)
        start = int(t0 * RATE); length = int(random.uniform(0.01, 0.04) * RATE); amp = 0.5 * math.exp(-t0 * 2)
        f = random.uniform(900, 2600)
        for k in range(length):
            if start + k < n:
                out[start + k] += amp * math.sin(2 * math.pi * f * k / RATE) * math.exp(-k / (length / 4))
    return out

def carve():
    n = int(0.18 * RATE)
    noise = lowpass([random.uniform(-1, 1) for _ in range(n)], 0.8)
    return [(0.8 * math.sin(2 * math.pi * 4200 * i / RATE) + 0.6 * noise[i]) * math.exp(-i / RATE * 60) + 0.3 * math.sin(2 * math.pi * 900 * i / RATE) * math.exp(-i / RATE * 45) for i in range(n)]

os.makedirs(OUT, exist_ok=True)
write('SW_AxeChop.wav', chop()); write('SW_TreeCrack.wav', crack(), 0.8); write('SW_TreeFall.wav', fall())
write('SW_PickaxeHit.wav', pickaxe(), 0.8); write('SW_RockBreak.wav', rock_break()); write('SW_StoneCarve.wav', carve(), 0.7)
print(sorted(os.listdir(OUT)))
