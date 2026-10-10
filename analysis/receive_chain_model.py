"""
receive_chain_model.py - Python model of the 40 kHz receive chain.

Component values are taken from the OrCAD schematic (docs/schematic-reception.png):
  series RLC   L1 = 100 uH, C1 = 160 nF, R2 = 50 ohm  (f0 = 39.8 kHz, Q = 0.5)
  stage 1      TL082, 1 + 9k/1k  = x10
  stage 2      TL082, 1 + 29k/1k = x30
  detector     D1 (1N4148) + C2 = 10 nF // R8 = 2 kohm  (tau = 20 us)
  comparator   LM311, threshold Vref = 6 V (1k/1k divider on 12 V), open collector,
               pull-up 3.3 kohm to 3.3 V
The original OrCAD simulation captures were not kept: this script regenerates the
figure. It is a behavioural model, NOT a measurement.

Assumptions (not from the schematic):
  - echo amplitude 5 mV peak at the RLC input, 0.3 mV rms white noise;
  - ideal buffer, ideal op-amps referenced to Vref, outputs clipped to 0-12 V,
    gain-bandwidth 4 MHz (first-order roll-off per stage);
  - diode: ideal, 0.6 V forward drop; comparator: ideal, no delay;
  - ideal 8-cycle burst (no transducer response); c = 343 m/s.

Run from the repo root:  python3 analysis/receive_chain_model.py   (writes docs/simulation-chaine.png)
"""
import numpy as np
from scipy import signal

# ---- parameters -----------------------------------------------------------
F0, CYCLES = 40e3, 8
L1, C1, R2 = 100e-6, 160e-9, 50.0
G1, G2 = 1 + 9e3 / 1e3, 1 + 29e3 / 1e3                # x10, x30
R8, C2, VF = 2e3, 10e-9, 0.6
VCC, VREF, V33 = 12.0, 6.0, 3.3
GBW = 4e6
A_ECHO, N_RMS = 5e-3, 0.3e-3
DIST_M, C_SOUND = 1.0, 343.0
DT, T_END = 0.1e-6, 8e-3

t = np.arange(0, T_END, DT)
t_echo = 2 * DIST_M / C_SOUND                          # 5.831 ms
decay = np.exp(-DT / (R8 * C2))
num, den = [R2 * C1, 0], [L1 * C1, R2 * C1, 1]         # series RLC, output across R2


def stage(v_ac_in, gain):
    """Op-amp stage referenced to Vref, first-order GBW limit, rails 0-12 V."""
    b, a = signal.bilinear([1], [gain / (2 * np.pi * GBW), 1], 1 / DT)
    return np.clip(VREF + signal.lfilter(b, a, gain * v_ac_in), 0, VCC)


def run(a_echo):
    rng = np.random.default_rng(1)
    gate = ((t >= t_echo) & (t < t_echo + CYCLES / F0)).astype(float)
    vin = a_echo * np.sin(2 * np.pi * F0 * (t - t_echo)) * gate
    vin += rng.normal(0, N_RMS, t.size)
    _, v_rlc, _ = signal.lsim((num, den), vin, t)
    v1 = stage(v_rlc, G1)
    v2 = stage(v1 - VREF, G2)
    vd = np.empty_like(v2)                              # diode + C2 // R8 to ground
    x = 0.0
    for i, v in enumerate(v2):
        x = v - VF if v - VF > x else x * decay
        vd[i] = x
    comp = np.where(vd > VREF, V33, 0.0)                # LM311, 3.3 V pull-up
    edges = np.flatnonzero((comp[1:] > 0) & (comp[:-1] == 0))
    edges = edges[t[edges] > t_echo - 100e-6]
    return v_rlc, v1, v2, vd, comp, t[edges[0] + 1], edges.size


v_rlc, v1, v2, vd, comp, t_det, n_pulses = run(A_ECHO)
delay_us = (t_det - t_echo) * 1e6
sweep = {a: (run(a)[5] - t_echo) * 1e6 for a in (2.5e-3, 3e-3, 4e-3, 5e-3, 10e-3)}
print(f"echo at {t_echo*1e3:.3f} ms (1 m), first rising edge at {t_det*1e3:.3f} ms, "
      f"delay {delay_us:.0f} us, {n_pulses} comparator pulses")
print("detection delay vs echo amplitude:",
      ", ".join(f"{a*1e3:g} mV -> {d:.0f} us" for a, d in sweep.items()))

# ---- figure ------------------------------------------------------------------------
import os, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from style import plt, NAVY, RED, GREY, INK

w = (t > t_echo - 40e-6) & (t < t_echo + 300e-6)
tw = (t[w] - t_echo) * 1e6
panels = [
    (v_rlc[w] * 1e3, "(a) After the RLC filter (mV)"),
    (v1[w], "(b) Stage 1, ×10 (V)"),
    (v2[w], "(c) Stage 2, ×30, total ×300 (V)"),
    (vd[w], "(d) Peak detector, τ = 20 µs (V)"),
    (comp[w], "(e) LM311 output (V)"),
]
fig, axes = plt.subplots(5, 1, figsize=(6.5, 8.0), sharex=True)
for a, (y, title) in zip(axes, panels):
    a.plot(tw, y, color=NAVY)
    a.set_title(title, fontsize=9.5, pad=3)
axes[3].axhline(VREF, color=GREY, ls="--", lw=0.9)
axes[3].text(298, VREF + 0.08, "threshold 6 V (as drawn)", ha="right", va="bottom", fontsize=8, color=GREY)
axes[4].axvline(delay_us, color=RED, ls=":", lw=1.0)
axes[4].text(215, V33 * 0.5, f"first rising edge,\n{delay_us:.0f} µs after the echo",
             fontsize=8, color=RED, va="center", ha="left")
axes[4].annotate("", xy=(delay_us, V33 * 0.5), xytext=(212, V33 * 0.5),
                 arrowprops=dict(arrowstyle="-", color=RED, lw=0.6, ls=":"))
axes[4].set_yticks([0, V33]); axes[4].set_yticklabels(["0", "3.3"])
axes[4].set_xlabel("time after the echo reaches the receiver (µs)")
fig.tight_layout(h_pad=0.6)
fig.savefig("docs/simulation-chaine.png", dpi=300)
