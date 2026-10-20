"""
diag_expected_overlay.py — 用现有 CSV 画「A/C/D 片段 voiced 帧 + 期望角度」叠加图

不重跑、不改参数，只读 Board2_REC_006_0-247_f64_i8.csv 和 segments.txt。
坐标轴/颜色与原图一致（y 固定 [-90,90]、viridis 0–1、灰色虚线=interp1 档位）。
每个片段上方标说话人字母；在 +42/+32/+8° 画三条红线。
"""

import numpy as np
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

CSV = "Board2_REC_006_0-247_f64_i8.csv"
SEG_FILE = "segments.txt"
OUT = "diag_expected_overlay.png"
FS = 16000  # 已知（原图标题 fs=16000Hz），用于画 interp1 档位虚线

EXPECTED = {"A": 42.0, "C": 32.0, "D": 8.0}  # 红线角度
SPEAKERS = ["A", "C", "D"]


def noninterp_grid(fs, d=0.036, c=343.0, m=0.1):
    tau_max = d / c
    max_n = int(np.floor((1.0 + m) * tau_max * fs))
    out = []
    for nn in range(-max_n, max_n + 1):
        x = c * (nn / fs) / d
        if abs(x) <= 1.0:
            out.append(round(float(np.degrees(np.arcsin(x))), 2))
    return sorted(set(out))


def read_csv(path):
    data = np.genfromtxt(path, delimiter=",", names=True)
    return data


def read_segments(path):
    segs = {}
    with open(path, "r", encoding="utf-8") as fh:
        for line in fh:
            line = line.strip()
            if not line or line.startswith("#"):
                continue
            lab, s, e = line.split()
            segs.setdefault(lab, []).append((float(s), float(e)))
    return segs


def main():
    data = read_csv(CSV)
    t = data["time_s"]
    ang = data["angle_deg"]
    conf = data["confidence"]
    voiced = data["voiced"]
    segs = read_segments(SEG_FILE)

    fig, ax = plt.subplots(figsize=(12, 5))

    # interp1 档位虚线（与原图一致）
    for g in noninterp_grid(FS):
        ax.axhline(g, ls="--", color="gray", lw=0.6, alpha=0.5)

    # 只画 A/C/D 片段内的 voiced 帧
    sc = None
    for spk in SPEAKERS:
        for s, e in segs.get(spk, []):
            m = (voiced == 1) & (t >= s) & (t < e)
            if m.any():
                sc = ax.scatter(t[m], ang[m], c=conf[m], cmap="viridis",
                                vmin=0, vmax=1, s=10)
            # 片段上方标说话人字母
            ax.text((s + e) / 2.0, 83, spk, color="black", fontsize=11,
                    fontweight="bold", ha="center", va="center")

    if sc is not None:
        cb = fig.colorbar(sc, ax=ax)
        cb.set_label("confidence")

    # 期望角度红线
    for spk in SPEAKERS:
        y = EXPECTED[spk]
        ax.axhline(y, color="red", lw=1.2)
        ax.text(ax.get_xlim()[1] if False else t.max(), y,
                f"{spk} expected", color="red", fontsize=9,
                ha="right", va="bottom")

    ax.set_ylim(-90, 90)
    ax.set_xlim(0, t.max())
    ax.set_xlabel("time_s (whole-file time)")
    ax.set_ylabel("angle (deg)")
    ax.set_title("Board2_REC_006  A/C/D voiced frames vs expected angles  "
                 "(fs=16000Hz, interp=8, frame=64ms)")
    ax.grid(True, alpha=0.2)
    fig.tight_layout()
    fig.savefig(OUT, dpi=120)
    plt.close(fig)
    print(f"图已写出: {OUT}")


if __name__ == "__main__":
    main()
