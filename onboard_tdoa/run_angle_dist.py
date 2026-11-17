"""
run_angle_dist.py — 每帧角度分布输出（本任务一次性驱动脚本）

对每个 voiced 帧：取 GCC-PhaT 曲线在物理可能时延范围 ±d/c 内的部分，
负值置 0，归一化使总和为 1，每个时延点换算成对应角度 angle=arcsin(c·τ/d)。

(1) 每段：所有帧分布取平均，画角度–概率图（左轴，概率质量）。
(2) 同图叠加该段逐帧角度读数（interp8+抛物线）的归一化直方图（右轴，密度）。

每段汇报：平均分布峰值角度、覆盖 50% 概率质量的角度范围（中心 25–75 百分位）、
局部峰个数（高于最高峰 30% 才算）、PNG 路径、CSV 路径。
逐帧分布另存 CSV（每行一帧，每列一个角度）。

分析参数：frame=64 ms、hop=32 ms、interp=8、parabolic；其余（VAD、频带）未改。
"""

import os
import numpy as np

import wav_io
from single_board_doa import (
    _frame_starts, _frame_energy, _noise_floor,
    MIC1_COL, MIC2_COL, DEF_VAD_DB,
)
from gcc_phat import gcc_phat, C_SOUND, D_MIC

HERE = os.path.dirname(os.path.abspath(__file__))
REC_DIR = os.path.join(HERE, "recordings")
SEG_DIR = os.path.join(HERE, "segments")
DEF_OUTDIR = os.path.join(HERE, "results", "scratch")

FILES = [
    ("REC_015.wav", "segments_nearfield.txt"),
    ("REC_016.WAV", "segments_distance.txt"),
    ("REC_018.WAV", "segments_distance2.txt"),
]

FRAME_MS = 64.0
HOP_MS = 32.0
INTERP = 8
HIST_BINS = np.arange(-90.0, 90.0 + 1e-9, 2.0)   # 直方图分箱（2°）
PEAK_FRAC = 0.30                                  # 局部峰阈值：最高峰的 30%
TAU_MAX = D_MIC / C_SOUND                          # 物理时延上限 ±d/c


def read_segments(path):
    segs = []
    with open(path, "r", encoding="utf-8") as fh:
        for line in fh:
            line = line.strip()
            if not line or line.startswith("#"):
                continue
            p = line.split()
            segs.append((p[0], float(p[1]), float(p[2])))
    return segs


def seg_distributions(preloaded, seg_start, seg_end):
    """返回 (angles, dists, readings, n_skipped)。
    angles: 固定角度栅格（升序）；dists: 每帧归一化分布 (nframes, nangle)；
    readings: 每帧 interp8+抛物线角度读数。"""
    samples, fs, _bits, _nch = preloaded
    mic1 = samples[:, MIC1_COL].astype(np.float64)
    mic2 = samples[:, MIC2_COL].astype(np.float64)
    total = samples.shape[0]

    n = int(round(FRAME_MS / 1000.0 * fs))
    hop = int(round(HOP_MS / 1000.0 * fs))
    nf, _ = _noise_floor(mic1, mic2, n, hop, total, fs)
    thr = nf * (10.0 ** (DEF_VAD_DB / 10.0))

    lo = int(round(seg_start * fs))
    hi = int(round(seg_end * fs))
    starts = _frame_starts(total, n, hop, lo, hi)

    angles = None
    sel = None
    dists = []
    readings = []
    n_skip = 0
    for s in starts:
        if _frame_energy(mic1, mic2, s, n) <= thr:
            continue
        res = gcc_phat(mic1[s:s + n], mic2[s:s + n], fs,
                       interp=INTERP, parabolic=True, return_curve=True)
        lags = res["lags"]
        curve = res["curve"]
        if sel is None:
            sel = np.abs(lags) <= TAU_MAX                 # 物理范围 ±d/c
            x = C_SOUND * lags[sel] / D_MIC
            angles = np.degrees(np.arcsin(np.clip(x, -1.0, 1.0)))
        dist = np.clip(curve[sel], 0.0, None)             # 负值置 0
        ssum = dist.sum()
        if ssum <= 0:
            n_skip += 1
            continue
        dists.append(dist / ssum)                         # 归一化使总和为 1
        readings.append(res["angle_deg"])

    return angles, np.array(dists), np.array(readings), n_skip


def mass_range(angles, pmf, lo=0.25, hi=0.75):
    """中心 50% 概率质量的角度范围（PMF 的 25–75 百分位，线性内插）。"""
    c = np.cumsum(pmf)
    c = c / c[-1]
    return float(np.interp(lo, c, angles)), float(np.interp(hi, c, angles))


def count_local_peaks(avg, frac=PEAK_FRAC):
    """局部峰个数：严格高于相邻点且高度 > frac×最高峰（含端点）。"""
    thr = frac * avg.max()
    k = avg.size
    peaks = []
    for i in range(k):
        left_ok = (i == 0) or (avg[i] > avg[i - 1])
        right_ok = (i == k - 1) or (avg[i] > avg[i + 1])
        if left_ok and right_ok and avg[i] > thr:
            peaks.append(i)
    return peaks


def main(outdir=DEF_OUTDIR):
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt

    os.makedirs(outdir, exist_ok=True)
    out_lines = [f"{'文件':<12}{'段':<16}{'帧数':>6}{'峰值角':>9}"
                 f"{'50%质量范围':>18}{'局部峰':>7}"]
    for wav, segfile in FILES:
        wav_path = os.path.join(REC_DIR, wav)
        base = os.path.splitext(os.path.basename(wav))[0]
        preloaded = wav_io.read_wav(wav_path)
        for label, s, e in read_segments(os.path.join(SEG_DIR, segfile)):
            angles, dists, readings, n_skip = seg_distributions(preloaded, s, e)
            nfr = dists.shape[0]
            avg = dists.mean(axis=0)
            peak_angle = float(angles[int(np.argmax(avg))])
            a_lo, a_hi = mass_range(angles, avg)
            peaks = count_local_peaks(avg)

            # ---- CSV：每行一帧，每列一个角度 ----
            csv_path = os.path.join(outdir, f"{base}_{int(s)}-{int(e)}_angdist.csv")
            header = ",".join(f"{a:.2f}" for a in angles)
            with open(csv_path, "w", encoding="utf-8") as fh:
                fh.write(header + "\n")
                for row in dists:
                    fh.write(",".join(f"{v:.6f}" for v in row) + "\n")

            # ---- 图：平均分布（左轴）+ 读数直方图（右轴）----
            fig, axL = plt.subplots(figsize=(9, 5))
            axL.plot(angles, avg, "-o", ms=3, color="tab:blue",
                     label="avg distribution")
            axL.axvline(peak_angle, ls="--", color="tab:red", lw=1.0,
                        label=f"peak {peak_angle:.1f}°")
            axL.axvspan(a_lo, a_hi, color="tab:red", alpha=0.08,
                        label=f"50% mass [{a_lo:.1f},{a_hi:.1f}]")
            axL.set_xlim(-90, 90)
            axL.set_xlabel("angle (deg)")
            axL.set_ylabel("avg distribution (prob. mass / delay-point)",
                           color="tab:blue")
            axL.tick_params(axis="y", labelcolor="tab:blue")
            axL.grid(True, alpha=0.3)

            axR = axL.twinx()
            axR.hist(readings, bins=HIST_BINS, density=True,
                     color="tab:green", alpha=0.35,
                     label="frame-reading hist")
            axR.set_ylabel("frame-reading density (1/deg)", color="tab:green")
            axR.tick_params(axis="y", labelcolor="tab:green")

            lh, ll = axL.get_legend_handles_labels()
            rh, rl = axR.get_legend_handles_labels()
            axL.legend(lh + rh, ll + rl, loc="upper left", fontsize=8)
            axL.set_title(f"{base} {label} ({s:g}-{e:g}s)  {nfr} frames  "
                          f"frame=64ms interp=8")
            fig.tight_layout()
            png_path = os.path.join(outdir, f"{base}_{int(s)}-{int(e)}_angdist.png")
            fig.savefig(png_path, dpi=120)
            plt.close(fig)

            out_lines.append(
                f"{base:<12}{label+' ('+f'{s:g}-{e:g}'+')':<16}{nfr:>6}"
                f"{peak_angle:>8.1f}°"
                f"{'['+f'{a_lo:.1f},{a_hi:.1f}'+']':>18}{len(peaks):>7}")
            print(f"{base} {label} ({s:g}-{e:g}): 帧={nfr} 跳过={n_skip} "
                  f"峰={peak_angle:.1f}° 50%=[{a_lo:.1f},{a_hi:.1f}] "
                  f"局部峰={len(peaks)}@{[round(float(angles[i]),1) for i in peaks]}")
            print(f"   PNG {png_path}")
            print(f"   CSV {csv_path}")

    print("\n" + "\n".join(out_lines))


if __name__ == "__main__":
    import argparse
    ap = argparse.ArgumentParser(description="每帧角度分布输出")
    ap.add_argument("--outdir", default=DEF_OUTDIR)
    main(outdir=ap.parse_args().outdir)
