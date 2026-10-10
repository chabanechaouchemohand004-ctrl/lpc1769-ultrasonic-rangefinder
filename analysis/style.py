"""Shared matplotlib style: sober, serif, two colours plus greys."""
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

plt.rcParams.update({
    "font.family": "serif", "font.serif": ["Liberation Serif", "Times New Roman", "DejaVu Serif"],
    "mathtext.fontset": "stix", "svg.fonttype": "path",
    "font.size": 10, "legend.fontsize": 8.5, "xtick.labelsize": 9, "ytick.labelsize": 9,
    "axes.linewidth": 0.7, "xtick.direction": "in", "ytick.direction": "in",
    "legend.frameon": False, "axes.grid": True, "grid.color": "#e3e3e3", "grid.linewidth": 0.5,
    "lines.linewidth": 1.1, "axes.titlelocation": "left", "axes.titlesize": 10,
})
NAVY, RED, GREY, LIGHT, INK = "#1f3a5f", "#9a3b2e", "#6e6e6e", "#c8c8c8", "#222222"
