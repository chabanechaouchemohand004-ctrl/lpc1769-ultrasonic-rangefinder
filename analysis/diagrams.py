"""Block diagram of the measurement chain and timing diagram of one measurement.
Run from the repo root:  python3 analysis/diagrams.py   (writes docs/chaine.svg, docs/sequence.svg)"""
import os, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from style import plt, NAVY, RED, GREY, LIGHT, INK
from matplotlib.patches import Rectangle, FancyArrowPatch
import numpy as np


def box(ax, x0, x1, y0, y1, lines, title_color=INK, ls="-", lw=0.8, ec=INK, fs=8.5):
    ax.add_patch(Rectangle((x0, y0), x1 - x0, y1 - y0, fill=False, ec=ec, lw=lw, ls=ls))
    xm, ym = (x0 + x1) / 2, (y0 + y1) / 2
    n = len(lines)
    for i, (txt, kind) in enumerate(lines):
        y = ym + (n - 1) * 2.2 - i * 4.4
        ax.text(xm, y, txt, ha="center", va="center", fontsize=fs,
                color=NAVY if kind == "t" else INK, fontweight="normal")


def arrow(ax, p, q, color=INK, lw=0.8):
    ax.add_patch(FancyArrowPatch(p, q, arrowstyle="-|>", mutation_scale=8, lw=lw, color=color,
                                 shrinkA=0, shrinkB=0))


# ---------------------------------------------------------------- block diagram
fig, ax = plt.subplots(figsize=(11, 3.9))
ax.set_xlim(0, 142); ax.set_ylim(0, 50); ax.axis("off"); ax.grid(False)
YE, YR = 36, 12
box(ax, 1, 25, 4, 44, [("LPC1769", "t"), ("Cortex-M3, 100 MHz", "k"), ("TIMER1: burst (MR0),", "k"),
                       ("echo capture (CAP1.0)", "k")])
ax.text(63, 47.5, "Emission", fontsize=9.5, color=NAVY, ha="center")
ax.text(80, 24.5, "Reception", fontsize=9.5, color=NAVY, ha="center")
for (x0, x1, l1, l2) in [(33, 49, "IR2304", "gate driver"), (57, 73, "IRFZ24N", "MOSFET, 12 V"),
                         (81, 97, "400ST100", "TX transducer")]:
    box(ax, x0, x1, YE - 6, YE + 6, [(l1, "t"), (l2, "k")])
arrow(ax, (25, YE), (33, YE)); ax.text(29, YE + 1.6, "P2.13", ha="center", fontsize=7.5, color=GREY)
arrow(ax, (49, YE), (57, YE)); arrow(ax, (73, YE), (81, YE))
box(ax, 128, 141, 8, 40, [("Obstacle", "t"), ("5–250 cm", "k")], ec=GREY, ls="--")
arrow(ax, (97, YE), (128, YE)); ax.text(112, YE + 1.6, "8 × 40 kHz", ha="center", fontsize=7.5, color=GREY)
rx = [(113, 125, "400SR100", "RX transducer"), (98, 110, "MCP6002", "buffer"),
      (83, 95, "RLC filter", "f0 = 39.8 kHz"), (68, 80, "2 × TL082", "×10, ×30"),
      (53, 65, "Peak", "detector"), (38, 50, "LM311", "comparator")]
for (x0, x1, l1, l2) in rx:
    box(ax, x0, x1, YR - 6, YR + 6, [(l1, "t"), (l2, "k")], fs=8)
arrow(ax, (128, YR), (125, YR))
for a, b in zip(rx[:-1], rx[1:]):
    arrow(ax, (a[0], YR), (b[1], YR))
arrow(ax, (38, YR), (25, YR)); ax.text(31.5, YR + 1.6, "P1.18", ha="center", fontsize=7.5, color=GREY)
fig.tight_layout(pad=0.3)
fig.savefig("docs/chaine.svg"); plt.close(fig)

# ---------------------------------------------------------------- timing diagram
fig, ax = plt.subplots(figsize=(11, 3.7))
ax.set_xlim(0, 100); ax.set_ylim(-26, 30); ax.axis("off"); ax.grid(False)
X0, XB, XE, XT = 8, 22, 70, 94           # t0, end of blanking, echo (CR0), timeout (not to scale)
# P2.13 burst
yb = 20
ax.text(0, yb + 3, "P2.13", fontsize=9, color=NAVY, va="center")
xs, ys = [0, X0], [yb, yb]
for k in range(8):
    x = X0 + k * 1.1 * 1.0
    xs += [x, x, x + 0.55, x + 0.55]; ys += [yb, yb + 5, yb + 5, yb]
xs += [100]; ys += [yb]
ax.plot(xs, ys, color=NAVY, lw=1.0)
# P1.18 comparator output
yc = 6
ax.text(0, yc + 3, "P1.18", fontsize=9, color=NAVY, va="center")
ax.plot([0, 9, 9, 15, 15, XE, XE, XE + 6, XE + 6, 100], [yc, yc, yc + 5, yc + 5, yc, yc, yc + 5, yc + 5, yc, yc],
        color=NAVY, lw=1.0, drawstyle="steps-post")
ax.text(12, yc + 7, "direct coupling (ignored)", fontsize=8, color=GREY, ha="center")
ax.text(XE + 3, yc + 7, "echo", fontsize=8.5, color=RED, ha="center")
# capture window
yw = -6
ax.text(0, yw, "Capture", fontsize=9, color=NAVY, va="center")
ax.add_patch(Rectangle((X0, yw - 2), XB - X0, 4, fill=False, ec=GREY, lw=0.8, hatch="////"))
ax.add_patch(Rectangle((XB, yw - 2), XT - XB, 4, fill=False, ec=INK, lw=0.8))
ax.text((X0 + XB) / 2, yw - 5.2, "off", ha="center", fontsize=8, color=GREY)
ax.text((XB + XT) / 2, yw - 5.2, "on: rising edge of P1.18 stored in CR0", ha="center", fontsize=8, color=INK)
# markers
for x, lab, col in [(X0, "t0", INK), (XB, "t0 + 290 µs", INK), (XE, "CR0", RED), (XT, "t0 + 14.5 ms (MR1)", INK)]:
    ax.plot([x, x], [-9.5, 27], color=col, lw=0.6, ls=":")
    ax.text(x, 28.5, lab, ha="center", fontsize=8.5, color=col)
ax.add_patch(FancyArrowPatch((X0, -17), (XE, -17), arrowstyle="<|-|>", mutation_scale=8, lw=0.8, color=INK,
                             shrinkA=0, shrinkB=0))
ax.text((X0 + XE) / 2, -21, r"time of flight = CR0 − t0,   $d = c\,t\,/\,2$", ha="center", fontsize=9)
ax.text(100, -25.5, "time axis not to scale", ha="right", fontsize=7.5, color=GREY)
fig.tight_layout(pad=0.3)
fig.savefig("docs/sequence.svg"); plt.close(fig)
