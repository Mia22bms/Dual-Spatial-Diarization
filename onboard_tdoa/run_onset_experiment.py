"""
run_onset_experiment.py — onset 帧筛选实验（本任务一次性驱动脚本）

对 REC_016（4 段，segments_distance.txt）与 REC_018（3 段，segments_distance2.txt）
跑三种设置，全部 interp=8 + 抛物线拟合（--parabolic）：
  (a) 全部 voiced 帧，frame=64 ms，hop=32 ms                （基线，不启用 onset）
  (b) onset 帧，       frame=64 ms，hop=32 ms               （onset-db=6, lookback=3）
  (c) onset 帧，       frame=16 ms，hop=8 ms                 （onset-db=6, lookback=3）

每段每设置汇报：保留帧数、角度中位数、角度 25/75 百分位。
另对 REC_016 的 0.3 m、1 m 两段，各画一张“所有 voiced 帧归一化 GCC-PhaT 曲线的
平均（横轴换算成角度）”。

onset 规则与默认值见 single_board_doa.py：一帧保留 = voiced 且该帧功率 ≥
前 lookback 帧平均功率 × 10^(onset_db/10)。默认 onset_db=6 dB、lookback=3。
"""

import os
import numpy as np

import wav_io
from single_board_doa import (
    analyze, _frame_starts, _frame_energy, _noise_floor,
    MIC1_COL, MIC2_COL, DEF_VAD_DB, DEF_ONSET_DB, DEF_ONSET_LOOKBACK,
)
from gcc_phat import gcc_phat, C_SOUND, D_MIC

HERE = os.path.dirname(os.path.abspath(__file__))
REC_DIR = os.path.join(HERE, "recordings")
SEG_DIR = os.path.join(HERE, "segments")
DEF_OUTDIR = os.path.join(HERE, "results", "scratch")

FILES = [
    ("REC_016.WAV", "segments_distance.txt"),
    ("REC_018.WAV", "segments_distance2.txt"),
]

# 设置：(名称, frame_ms, hop_ms, onset_db)；onset_db=None 即基线全 voiced
SETTINGS = [
    ("(a) voiced f64",  64.0, 32.0, None),
    ("(b) onset  f64",  64.0, 32.0, DEF_ONSET_DB),
    ("(c) onset  f16",  16.0,  8.0, DEF_ONSET_DB),
]

ONSET_LOOKBACK = DEF_ONSET_LOOKBACK


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


def stat_for(r):
    """从 analyze 结果取 (保留帧数, 角度中位数, p25, p75)。"""
    rows = r["rows"]
    onset = r.get("onset")
    if onset is not None:
        mask = onset == 1
    else:
        mask = rows[:, 4] == 1 if rows.size else np.zeros(0, dtype=bool)
    n = int(mask.sum())
    if n == 0:
        return 0, None, None, None
    angs = rows[mask, 2]
    return (n, float(np.median(angs)),
            float(np.percentile(angs, 25)), float(np.percentile(angs, 75)))


def avg_norm_curve(path, preloaded, seg_start, seg_end,
                   frame_ms=64.0, hop_ms=32.0, interp=8):
    """所有 voiced 帧的归一化 GCC-PhaT 曲线（每帧除以其峰值）平均，
    横轴换算成角度。返回 (angles_deg, mean_curve, n_voiced)。"""
    samples, fs, _bits, nch = preloaded
    mic1 = samples[:, MIC1_COL].astype(np.float64)
    mic2 = samples[:, MIC2_COL].astype(np.float64)
    total = samples.shape[0]

    n = int(round(frame_ms / 1000.0 * fs))
    hop = int(round(hop_ms / 1000.0 * fs))
    nf, _ = _noise_floor(mic1, mic2, n, hop, total, fs)
    thr = nf * (10.0 ** (DEF_VAD_DB / 10.0))

    lo = int(round(seg_start * fs))
    hi = int(round(seg_end * fs))
    starts = _frame_starts(total, n, hop, lo, hi)

    acc = None
    lags = None
    nv = 0
    for s in starts:
        if _frame_energy(mic1, mic2, s, n) <= thr:
            continue
        res = gcc_phat(mic1[s:s + n], mic2[s:s + n], fs,
                       interp=interp, return_curve=True)
        cur = res["curve"]
        peak = cur.max()
        if peak <= 0:
            continue
        cur = cur / peak                     # 归一化：每帧峰值 = 1
        acc = cur if acc is None else acc + cur
        lags = res["lags"]
        nv += 1

    mean_curve = acc / nv
    # lag → 角度：只保留 arcsin 定义域内的点（|c·τ/d| ≤ 1，即物理 ±90°）
    x = C_SOUND * lags / D_MIC
    valid = np.abs(x) <= 1.0
    angles = np.degrees(np.arcsin(x[valid]))
    order = np.argsort(angles)
    return angles[order], mean_curve[valid][order], nv


def main(outdir=DEF_OUTDIR):
    os.makedirs(outdir, exist_ok=True)
    rows_by_file = {}
    for wav, segfile in FILES:
        wav_path = os.path.join(REC_DIR, wav)
        segs = read_segments(os.path.join(SEG_DIR, segfile))
        preloaded = wav_io.read_wav(wav_path)
        table = []
        for label, s, e in segs:
            for sname, fms, hms, odb in SETTINGS:
                r = analyze(wav_path, start=s, end=e, interp=8,
                            frame_ms=fms, hop_ms=hms,
                            preloaded=preloaded, write_outputs=False,
                            verbose=False, onset_db=odb,
                            onset_lookback=ONSET_LOOKBACK, onset_ref="mean",
                            parabolic=True)
                nf, med, p25, p75 = stat_for(r)
                table.append((label, f"{s:g}-{e:g}", sname, nf, med, p25, p75))
        rows_by_file[wav] = table

    # ---- 打印主表 ----
    hdr = f"{'段':<16}{'设置':<16}{'保留帧':>7}{'角度中位':>10}{'p25':>9}{'p75':>9}"
    lines = []
    for wav, _seg in FILES:
        lines.append(f"\n=== {wav} ===")
        lines.append(hdr)
        for label, span, sname, nf, med, p25, p75 in rows_by_file[wav]:
            lbl = f"{label} ({span})"
            if med is None:
                lines.append(f"{lbl:<16}{sname:<16}{nf:>7}{'—':>10}{'—':>9}{'—':>9}")
            else:
                lines.append(f"{lbl:<16}{sname:<16}{nf:>7}"
                             f"{med:>10.2f}{p25:>9.2f}{p75:>9.2f}")
    report = "\n".join(lines)
    print(report)

    # ---- REC_016 的 0.3m / 1m 平均归一化 GCC-PhaT 曲线图 ----
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt

    rec016 = os.path.join(REC_DIR, "REC_016.WAV")
    pre016 = wav_io.read_wav(rec016)
    plot_targets = [("0.3m", 9.0, 24.0), ("1m", 76.0, 91.0)]
    plot_paths = []
    for label, s, e in plot_targets:
        ang, curve, nv = avg_norm_curve(rec016, pre016, s, e)
        fig, ax = plt.subplots(figsize=(9, 5))
        ax.plot(ang, curve, lw=1.4, color="tab:blue")
        peak_ang = ang[int(np.argmax(curve))]
        ax.axvline(peak_ang, ls="--", color="tab:red", lw=1.0,
                   label=f"peak {peak_ang:.1f}°")
        ax.set_xlim(-90, 90)
        ax.set_xlabel("angle (deg)")
        ax.set_ylabel("avg normalized GCC-PhaT")
        ax.set_title(f"REC_016 {label} ({s:g}-{e:g}s)  "
                     f"avg of {nv} voiced frames  frame=64ms interp=8")
        ax.grid(True, alpha=0.3)
        ax.legend()
        fig.tight_layout()
        out = os.path.join(outdir, f"REC_016_{int(s)}-{int(e)}_avgcurve.png")
        fig.savefig(out, dpi=120)
        plt.close(fig)
        plot_paths.append((label, out, nv, peak_ang))
        print(f"\n[图] REC_016 {label}: {out}  "
              f"(n_voiced={nv}, 峰角 {peak_ang:.1f}°)")

    return rows_by_file, plot_paths


if __name__ == "__main__":
    import argparse
    ap = argparse.ArgumentParser(description="onset 帧筛选实验")
    ap.add_argument("--outdir", default=DEF_OUTDIR)
    main(outdir=ap.parse_args().outdir)
