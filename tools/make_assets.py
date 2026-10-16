"""Generates the pixel-art sprites (PNG) and 8-bit sounds (WAV, mono 48kHz 16-bit) used by JAILBREAK.
Run from the repo root: python3 tools/make_assets.py
"""
import math, os, random, struct, wave
from PIL import Image

OUT = "data"
PAL = {
    '.': (0, 0, 0, 0),
    'k': (20, 20, 28, 255),
    'h': (90, 55, 30, 255),
    's': (240, 190, 150, 255),
    'o': (235, 125, 35, 255),
    'O': (170, 80, 20, 255),
    'c': (25, 35, 80, 255),
    'b': (55, 85, 185, 255),
    'B': (35, 45, 115, 255),
    'y': (245, 215, 60, 255),
    'Y': (180, 140, 20, 255),
    'w': (240, 240, 240, 255),
    'g': (140, 140, 150, 255),
    'G': (85, 85, 95, 255),
    'r': (220, 40, 50, 255),
    'R': (120, 20, 30, 255),
    'e': (70, 210, 100, 255),
    'E': (30, 110, 55, 255),
}

def sprite(rows):
    img = Image.new("RGBA", (8, 8))
    for y, row in enumerate(rows):
        for x, ch in enumerate(row):
            img.putpixel((x, y), PAL[ch])
    return img

# character frames: [down, up, left] x [frame0, frame1]; right is mirrored left
PLAYER = {
    'down': [
        ["..hhhh..", ".hhhhhh.", ".hskksh.", ".ssssss.", "oooooooo", "s.oOOo.s", ".oo..oo.", ".k...kk."],
        ["..hhhh..", ".hhhhhh.", ".hskksh.", ".ssssss.", "oooooooo", "s.oOOo.s", ".oo..oo.", ".kk...k."],
    ],
    'up': [
        ["..hhhh..", ".hhhhhh.", ".hhhhhh.", ".shhhhs.", "oooooooo", "s.oooo.s", ".oo..oo.", ".k...kk."],
        ["..hhhh..", ".hhhhhh.", ".hhhhhh.", ".shhhhs.", "oooooooo", "s.oooo.s", ".oo..oo.", ".kk...k."],
    ],
    'left': [
        ["..hhhh..", ".hhhhhh.", ".kshhhh.", ".sssss..", "..oooo..", "..soOo..", "..oo.o..", "..kk.k.."],
        ["..hhhh..", ".hhhhhh.", ".kshhhh.", ".sssss..", "..oooo..", "..soOo..", "..o.oo..", "..k.kk.."],
    ],
}

def recolor(frames, mapping):
    return {d: [[''.join(mapping.get(c, c) for c in row) for row in f] for f in fl] for d, fl in frames.items()}

GUARD = recolor(PLAYER, {'h': 'c', 'o': 'b', 'O': 'y'})

def sheet(frames):
    img = Image.new("RGBA", (16, 32))
    for row, d in enumerate(['down', 'up', 'left']):
        for col in range(2):
            img.paste(sprite(frames[d][col]), (col * 8, row * 8))
    for col in range(2):
        img.paste(sprite(frames['left'][col]).transpose(Image.FLIP_LEFT_RIGHT), (col * 8, 24))
    return img

def noise_tile(base, var, seed):
    rnd = random.Random(seed)
    img = Image.new("RGBA", (8, 8))
    for y in range(8):
        for x in range(8):
            d = rnd.randint(-var, var)
            img.putpixel((x, y), tuple(max(0, min(255, c + d)) for c in base) + (255,))
    return img

def brick(face, mortar, top):
    img = Image.new("RGBA", (8, 8), mortar + (255,))
    for y in range(8):
        for x in range(8):
            row = y // 4
            off = 0 if row == 0 else 4
            if y % 4 == 3 or (x + off) % 8 == 7:
                continue
            c = top if y == 0 else face
            img.putpixel((x, y), c + (255,))
    return img

TILES = [
    noise_tile((52, 56, 70), 4, 1),                         # 0 floor
    brick((110, 105, 120), (60, 58, 70), (150, 145, 160)),  # 1 wall front
    sprite(["GGGGGGGG", "GgGgGgGG", "GgGgGgGG", "GgGyyGGG", "GgGYyGGG", "GgGgGgGG", "GgGgGgGG", "kkkkkkkk"]),  # 2 locked door
    sprite(["G......G", "g......g", "g......g", "g......g", "g......g", "g......g", "g......g", "G......G"]),  # 3 open door (over floor)
    sprite(["EEEEEEEE", "EeeeeeeE", "EekkkkeE", "EekeekeE", "EekeekeE", "EekkkkeE", "EeeeeeeE", "EEEEEEEE"]),  # 4 exit hatch
    noise_tile((48, 52, 66), 9, 2),                         # 5 floor variant
    noise_tile((92, 86, 110), 3, 3),                        # 6 wall top
    Image.new("RGBA", (8, 8)),                              # 7 unused
    sprite(["........", ".yyy....", "y...y...", "y...yyyy", ".yyy..yY", "......Y.", "........", "........"]),  # 8 key
    sprite(["........", "........", "........", "...gg...", "..gwgG..", "..gGG...", "........", "........"]),  # 9 pebble
    sprite(["........", ".rr.rr..", "rwrrrrr.", "rrrrrrr.", ".rrrrr..", "..rrr...", "...r....", "........"]),  # 10 heart
    sprite(["........", ".RR.RR..", "R..R..R.", "R.....R.", ".R...R..", "..R.R...", "...R....", "........"]),  # 11 empty heart
    sprite([".wwwwww.", "wwwrrwww", "wwwrrwww", "wwwrrwww", "wwwwwwww", "wwwrrwww", ".wwwwww.", "...ww..."]),  # 12 ! bubble
    sprite([".wwwwww.", "wwrrrrww", "wwwwwrww", "wwwrrrww", "wwwrwwww", "wwwwwwww", ".wwrwww.", "...ww..."]),  # 13 ? bubble
    sprite(["........", "........", "........", "........", "........", "..g..g..", ".gGg.g..", "........"]),  # 14 dust
    Image.new("RGBA", (8, 8)),                              # 15 unused
]

def tileset():
    img = Image.new("RGBA", (64, 16))
    for i, t in enumerate(TILES):
        img.paste(t, ((i % 8) * 8, (i // 8) * 8))
    return img

# ---------- audio ----------
RATE = 48000

def write_wav(name, samples):
    with wave.open(os.path.join(OUT, "sounds", name), "wb") as w:
        w.setnchannels(1); w.setsampwidth(2); w.setframerate(RATE)
        w.writeframes(b''.join(struct.pack('<h', int(max(-1, min(1, s)) * 32000)) for s in samples))

def square(f, t): return 1.0 if (t * f) % 1.0 < 0.5 else -1.0
def tri(f, t): p = (t * f) % 1.0; return 4 * abs(p - 0.5) - 1
def saw(f, t): return 2 * ((t * f) % 1.0) - 1

def tone(freqs, dur, wave_fn=square, vol=0.5, decay=True):
    """freqs: a frequency or a (start, end) sweep"""
    out = []
    n = int(dur * RATE)
    f0, f1 = (freqs, freqs) if not isinstance(freqs, tuple) else freqs
    phase = 0.0
    for i in range(n):
        f = f0 + (f1 - f0) * i / n
        phase += f / RATE
        env = (1 - i / n) if decay else min(1, (n - i) / 400)
        out.append(wave_fn(1, phase) * vol * env)
    return out

def noise(dur, vol=0.5):
    n = int(dur * RATE)
    return [random.uniform(-1, 1) * vol * (1 - i / n) for i in range(n)]

def mix(*tracks):
    n = max(len(t) for t in tracks)
    return [sum(t[i] for t in tracks if i < len(t)) for i in range(n)]

def note(n): return 440.0 * 2 ** ((n - 69) / 12)

def music():
    bpm, beats = 132, 32
    step = 60 / bpm / 2  # eighth notes
    bass = [45, 45, 52, 45, 48, 45, 52, 47] * 4 + [43, 43, 50, 43, 46, 43, 50, 45] * 4
    lead = [69, 0, 72, 0, 71, 69, 0, 64, 69, 0, 72, 74, 76, 0, 74, 72,
            67, 0, 70, 0, 69, 67, 0, 62, 67, 0, 70, 72, 71, 0, 69, 0] * 2
    out = []
    for i in range(beats * 2):
        b = tone(note(bass[i % len(bass)]), step, tri, 0.35, decay=False)
        l = tone(note(lead[i % len(lead)]), step, square, 0.12) if lead[i % len(lead)] else []
        h = noise(0.03, 0.15) if i % 2 == 1 else []
        seg = mix(b, l, h)
        seg += [0.0] * (int(step * RATE) - len(seg))
        out += seg
    return out

def main():
    os.makedirs(os.path.join(OUT, "sounds"), exist_ok=True)
    sheet(PLAYER).save(os.path.join(OUT, "player.png"))
    sheet(GUARD).save(os.path.join(OUT, "guard.png"))
    tileset().save(os.path.join(OUT, "tileset.png"))

    random.seed(7)
    write_wav("select.wav", tone(880, 0.05, square, 0.3))
    write_wav("confirm.wav", tone(660, 0.06, square, 0.3) + tone(990, 0.1, square, 0.3))
    write_wav("key.wav", sum((tone(note(n), 0.05, square, 0.3) for n in (84, 88, 91, 96)), []))
    write_wav("door.wav", mix(noise(0.15, 0.4), tone((180, 60), 0.25, saw, 0.4)))
    write_wav("alert.wav", tone(1200, 0.07, square, 0.35) + tone(1600, 0.15, square, 0.35))
    write_wav("caught.wav", tone((500, 60), 0.6, saw, 0.45))
    write_wav("throw.wav", noise(0.05, 0.3))
    write_wav("land.wav", tone((900, 300), 0.06, square, 0.3) + noise(0.04, 0.2))
    write_wav("exit.wav", sum((tone(note(n), 0.08, square, 0.3) for n in (72, 76, 79, 84, 79, 84)), []))
    write_wav("win.wav", sum((tone(note(n), d, square, 0.3) for n, d in ((72, .12), (72, .12), (72, .12), (77, .4), (81, .2), (84, .6))), []))
    write_wav("music.wav", music())
    print("assets written to", OUT)

if __name__ == "__main__":
    main()
