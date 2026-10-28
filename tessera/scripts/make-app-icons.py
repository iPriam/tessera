"""Draws the system applications' icons, light and dark, as iOS 18 pairs them.

Each is full-bleed square art: the platform clips it to the icon's continuous
corner. Drawn at 4x and reduced, so edges are smooth.

    python3 -I scripts/make-app-icons.py <tessera-projects>   (needs Pillow)
"""
import math
import os
import sys

from PIL import Image, ImageDraw, ImageFilter

OUT = 512
S = 4
N = OUT * S


def canvas():
    return Image.new("RGBA", (N, N), (0, 0, 0, 0))


def gradient(top, bottom):
    img = Image.new("RGBA", (N, N))
    d = ImageDraw.Draw(img)
    for y in range(N):
        t = y / (N - 1)
        c = tuple(round(top[i] + (bottom[i] - top[i]) * t) for i in range(3)) + (255,)
        d.line([(0, y), (N, y)], fill=c)
    return img


def u(v):
    return round(v * N)


def finish(img):
    return img.resize((OUT, OUT), Image.LANCZOS)


def shadow(layer, blur=0.012, fall=0.008, alpha=0.35):
    mask = layer.split()[3].point(lambda a: round(a * alpha))
    sh = Image.new("RGBA", layer.size, (0, 0, 0, 0))
    sh.putalpha(mask)
    sh = sh.filter(ImageFilter.GaussianBlur(u(blur)))
    out = Image.new("RGBA", layer.size, (0, 0, 0, 0))
    out.alpha_composite(sh, (0, u(fall)))
    out.alpha_composite(layer)
    return out


# ── Notes: a sheet with a yellow band and ruled lines ─────────────────────────
def notes(dark):
    img = gradient((44, 44, 46), (28, 28, 30)) if dark else gradient((255, 255, 255), (242, 242, 247))
    d = ImageDraw.Draw(img)
    band_top = (255, 214, 10) if not dark else (255, 204, 0)
    band_bottom = (255, 179, 0) if not dark else (230, 160, 0)
    for y in range(u(0.30)):
        t = y / u(0.30)
        c = tuple(round(band_top[i] + (band_bottom[i] - band_top[i]) * t) for i in range(3))
        d.line([(0, y), (N, y)], fill=c)
    line = (86, 86, 92) if dark else (198, 198, 204)
    for k in range(5):
        y = u(0.44 + k * 0.115)
        d.rounded_rectangle([u(0.10), y, u(0.90), y + u(0.012)], radius=u(0.006), fill=line)
    dots = (60, 60, 64) if dark else (232, 232, 236)
    for k in range(9):
        x = u(0.14 + k * 0.09)
        d.ellipse([x - u(0.012), u(0.335) - u(0.012), x + u(0.012), u(0.335) + u(0.012)], fill=dots)
    return finish(img)


# ── Clock: a dial with hands at 10:09 and a red second hand ───────────────────
def clock(dark):
    img = Image.new("RGBA", (N, N), (0, 0, 0, 255) if not dark else (12, 12, 14, 255))
    d = ImageDraw.Draw(img)
    c = N / 2
    r = u(0.42)
    face = (255, 255, 255) if not dark else (36, 36, 40)
    ink = (0, 0, 0) if not dark else (240, 240, 245)
    d.ellipse([c - r, c - r, c + r, c + r], fill=face)
    for k in range(60):
        a = k / 60 * 2 * math.pi
        major = k % 5 == 0
        outer = r * 0.93
        inner = r * (0.80 if major else 0.88)
        w = u(0.012 if major else 0.005)
        d.line(
            [(c + math.sin(a) * inner, c - math.cos(a) * inner),
             (c + math.sin(a) * outer, c - math.cos(a) * outer)],
            fill=ink if major else (ink if dark else (120, 120, 120)), width=w,
        )

    def hand(angle, length, width, colour, tail=0.0):
        a = angle * 2 * math.pi
        x0, y0 = c - math.sin(a) * tail * r, c + math.cos(a) * tail * r
        x1, y1 = c + math.sin(a) * length * r, c - math.cos(a) * length * r
        d.line([(x0, y0), (x1, y1)], fill=colour, width=width)
        d.ellipse([x1 - width / 2, y1 - width / 2, x1 + width / 2, y1 + width / 2], fill=colour)

    hand((10 + 9 / 60) / 12, 0.50, u(0.032), ink)
    hand(9 / 60, 0.76, u(0.024), ink)
    red = (255, 149, 0) if dark else (255, 59, 48)
    hand(32 / 60, 0.84, u(0.010), red, tail=0.18)
    d.ellipse([c - u(0.028), c - u(0.028), c + u(0.028), c + u(0.028)], fill=red)
    d.ellipse([c - u(0.010), c - u(0.010), c + u(0.010), c + u(0.010)], fill=face)
    return finish(img)


# ── Settings: a gear on brushed grey ──────────────────────────────────────────
def gear(colour, inner_colour, hole_colour):
    layer = canvas()
    d = ImageDraw.Draw(layer)
    c = N / 2
    teeth = 12
    outer, root = u(0.40), u(0.33)
    points = []
    steps = teeth * 16
    for k in range(steps):
        a = k / steps * 2 * math.pi
        phase = (k % 16) / 16
        rr = outer if 0.18 < phase < 0.62 else root
        points.append((c + math.cos(a) * rr, c + math.sin(a) * rr))
    d.polygon(points, fill=colour)
    ring = u(0.25)
    d.ellipse([c - ring, c - ring, c + ring, c + ring], fill=inner_colour)
    spokes = u(0.215)
    for k in range(6):
        a = k / 6 * 2 * math.pi
        d.line([(c, c), (c + math.cos(a) * spokes, c + math.sin(a) * spokes)], fill=colour, width=u(0.05))
    d.ellipse([c - spokes, c - spokes, c + spokes, c + spokes], outline=colour, width=u(0.035))
    hub = u(0.075)
    d.ellipse([c - hub, c - hub, c + hub, c + hub], fill=colour)
    hole = u(0.035)
    d.ellipse([c - hole, c - hole, c + hole, c + hole], fill=hole_colour)
    return layer


def settings(dark):
    if dark:
        img = gradient((40, 40, 44), (14, 14, 16))
        g = gear((152, 152, 160, 255), (40, 40, 44, 255), (20, 20, 22, 255))
    else:
        img = gradient((210, 211, 216), (142, 143, 150))
        g = gear((96, 97, 104, 255), (176, 177, 183, 255), (210, 211, 216, 255))
    img.alpha_composite(shadow(g))
    return finish(img)


# ── Finder: the two-tone face ─────────────────────────────────────────────────
def finder(dark):
    left_top, left_bottom = ((19, 92, 214), (8, 60, 160)) if dark else ((32, 134, 255), (18, 96, 228))
    right_top, right_bottom = ((60, 120, 190), (40, 86, 150)) if dark else ((190, 226, 255), (140, 196, 248))
    img = gradient(left_top, left_bottom)
    right = gradient(right_top, right_bottom)
    mask = Image.new("L", (N, N), 0)
    md = ImageDraw.Draw(mask)
    # The split runs down the middle with the nose jutting left.
    md.polygon([(u(0.53), 0), (N, 0), (N, N), (u(0.58), N), (u(0.56), u(0.62)),
                (u(0.47), u(0.62)), (u(0.47), u(0.56)), (u(0.53), u(0.40))], fill=255)
    img.paste(right, (0, 0), mask)
    d = ImageDraw.Draw(img)
    ink = (10, 30, 70) if not dark else (225, 235, 255)
    w = u(0.04)
    d.rounded_rectangle([u(0.30), u(0.26), u(0.30) + w, u(0.44)], radius=w // 2, fill=ink)
    d.rounded_rectangle([u(0.68), u(0.26), u(0.68) + w, u(0.44)], radius=w // 2, fill=ink)
    d.arc([u(0.20), u(0.42), u(0.80), u(0.80)], start=25, end=155, fill=ink, width=w)
    return finish(img)


# ── Disk Utility: a drive platter with an arm ─────────────────────────────────
def disk_utility(dark):
    img = gradient((48, 50, 56), (20, 22, 26)) if dark else gradient((236, 238, 242), (190, 194, 202))
    c = N / 2
    disc = canvas()
    d = ImageDraw.Draw(disc)
    r = u(0.38)
    rim = (130, 134, 144, 255) if dark else (150, 156, 168, 255)
    d.ellipse([c - r, c - r, c + r, c + r], fill=rim)
    rr = r - u(0.02)
    for k in range(rr, 0, -max(1, u(0.002))):
        t = k / rr
        v = round((70 + 60 * t) if dark else (190 + 50 * t))
        d.ellipse([c - k, c - k, c + k, c + k], fill=(v, v, min(255, v + 8), 255))
    for k in range(6):
        g = rr * (0.45 + k * 0.09)
        d.ellipse([c - g, c - g, c + g, c + g], outline=(255, 255, 255, 40 if dark else 70), width=u(0.003))
    hub = u(0.09)
    d.ellipse([c - hub, c - hub, c + hub, c + hub], fill=(60, 62, 70, 255) if dark else (120, 126, 138, 255))
    d.ellipse([c - u(0.03), c - u(0.03), c + u(0.03), c + u(0.03)], fill=(30, 30, 34, 255))
    img.alpha_composite(shadow(disc))
    arm = canvas()
    a = ImageDraw.Draw(arm)
    blue = (64, 156, 255, 255) if dark else (0, 122, 255, 255)
    pivot = (u(0.80), u(0.20))
    tip = (u(0.47), u(0.52))
    a.line([pivot, tip], fill=blue, width=u(0.045))
    a.ellipse([pivot[0] - u(0.06), pivot[1] - u(0.06), pivot[0] + u(0.06), pivot[1] + u(0.06)], fill=blue)
    a.ellipse([pivot[0] - u(0.022), pivot[1] - u(0.022), pivot[0] + u(0.022), pivot[1] + u(0.022)],
              fill=(255, 255, 255, 255))
    a.rounded_rectangle([tip[0] - u(0.04), tip[1] - u(0.03), tip[0] + u(0.04), tip[1] + u(0.03)],
                        radius=u(0.012), fill=blue)
    img.alpha_composite(shadow(arm, alpha=0.5))
    return finish(img)


# ── Welcome: a sunrise of colour, the greeting's wave ─────────────────────────
def welcome(dark):
    img = gradient((20, 24, 60), (6, 6, 18)) if dark else gradient((90, 200, 250), (88, 86, 214))
    d = ImageDraw.Draw(img)
    c = N / 2
    colours = [(255, 59, 48), (255, 149, 0), (255, 204, 0), (52, 199, 89), (0, 122, 255), (175, 82, 222)]
    for k, colour in enumerate(colours):
        r = u(0.40 - k * 0.055)
        d.arc([c - r, u(0.70) - r, c + r, u(0.70) + r], start=180, end=360, fill=colour, width=u(0.05))
    d.ellipse([c - u(0.07), u(0.70) - u(0.07), c + u(0.07), u(0.70) + u(0.07)],
              fill=(255, 255, 255) if not dark else (240, 240, 250))
    return finish(img)


# ── About: an information mark ────────────────────────────────────────────────
def about(dark):
    img = gradient((30, 34, 44), (10, 12, 16)) if dark else gradient((142, 142, 147), (72, 72, 78))
    d = ImageDraw.Draw(img)
    c = N / 2
    r = u(0.32)
    ring = (230, 230, 235) if not dark else (152, 152, 160)
    d.ellipse([c - r, c - r, c + r, c + r], outline=ring, width=u(0.04))
    d.ellipse([c - u(0.04), u(0.31) - u(0.04), c + u(0.04), u(0.31) + u(0.04)], fill=ring)
    d.rounded_rectangle([c - u(0.032), u(0.41), c + u(0.032), u(0.70)], radius=u(0.016), fill=ring)
    return finish(img)


ICONS = {
    "notes": notes,
    "clock": clock,
    "settings": settings,
    "finder": finder,
    "disk-utility": disk_utility,
    "welcome": welcome,
    "about": about,
}


def main(root):
    for package, draw in ICONS.items():
        folder = os.path.join(root, package, "Resources")
        os.makedirs(folder, exist_ok=True)
        draw(False).save(os.path.join(folder, "AppIcon.png"), optimize=True)
        draw(True).save(os.path.join(folder, "AppIconDark.png"), optimize=True)
        print(package)


if __name__ == "__main__":
    main(sys.argv[1])
