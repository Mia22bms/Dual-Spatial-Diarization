"""
single_board_doa.py — 单板双麦 GCC-PhaT 方向估计主程序

实现 TDOA_SPEC.md 的 §3（流程）、§4（energy VAD）、§9（命令行与输出）、
§10（CSV 规范）、§11（角度 vs 时间图）。SPEC §6 的默认值一律不改。

用法（§9）：
  python single_board_doa.py <wav路径> [--start 秒] [--end 秒]
         [--interp 8] [--frame-ms 64] [--hop-ms 32] [--vad-db 6]
         [--noise-start 秒 --noise-end 秒] [--outdir 输出文件夹]
"""

import os
import argparse
import numpy as np

import wav_io
from gcc_phat import gcc_phat, C_SOUND, D_MIC, M_MARGIN

# —— SPEC §6 默认值（不改）——
DEF_INTERP = 8
DEF_FRAME_MS = 64.0
DEF_HOP_MS = 32.0
DEF_VAD_DB = 6.0
NOISE_PERCENTILE = 10          # §4：整个文件帧能量的第 10 百分位

# SPEC §1.1：轨2=MIC1=列1，轨4=MIC2=列3
MIC1_COL = 1
MIC2_COL = 3


def _frame_starts(total, n, hop, lo, hi):
    """返回帧起点索引数组：起点 ≥ lo 且 起点+n ≤ hi。"""
    if hi - lo < n:
        return np.empty(0, dtype=int)
    last = hi - n
    return np.arange(lo, last + 1, hop, dtype=int)


def _frame_energy(mic1, mic2, start, n):
    """§4 帧能量：MIC1、MIC2 两路、加窗前、每采样点平方后的平均。"""
    a = mic1[start:start + n]
    b = mic2[start:start + n]
    return (np.dot(a, a) + np.dot(b, b)) / (2.0 * n)


def _noise_floor(mic1, mic2, n, hop, total, fs,
                 noise_start=None, noise_end=None):
    """§4 噪声底。默认 = 整个文件所有帧能量的第 10 百分位；
    若指定 --noise-start/--noise-end，则用该时间段内帧能量的中位数（优先）。"""
    if noise_start is not None and noise_end is not None:
        lo = max(0, int(round(noise_start * fs)))
        hi = min(total, int(round(noise_end * fs)))
        starts = _frame_starts(total, n, hop, lo, hi)
        if starts.size == 0:
            raise ValueError("指定的噪声时间段太短，放不下一帧")
        energies = np.array([_frame_energy(mic1, mic2, s, n) for s in starts])
        return float(np.median(energies)), "手动噪声段中位数"
    # 默认：整个文件，帧长/帧移与分析相同
    starts = _frame_starts(total, n, hop, 0, total)
    energies = np.array([_frame_energy(mic1, mic2, s, n) for s in starts])
    return float(np.percentile(energies, NOISE_PERCENTILE)), "全文件第10百分位"


def _noninterp_grid(fs, d=D_MIC, c=C_SOUND, m=M_MARGIN):
    """§11：当前 fs 下 interp=1 的角度档位（整数采样点、且在搜索范围内）。"""
    tau_max = d / c
    max_n = int(np.floor((1.0 + m) * tau_max * fs))
    angles = []
    for nn in range(-max_n, max_n + 1):
        x = c * (nn / fs) / d
        if abs(x) <= 1.0:
            angles.append(np.degrees(np.arcsin(x)))
    return sorted(set(round(a, 2) for a in angles))


def analyze(wav_path, start=None, end=None, interp=DEF_INTERP,
            frame_ms=DEF_FRAME_MS, hop_ms=DEF_HOP_MS, vad_db=DEF_VAD_DB,
            noise_start=None, noise_end=None, outdir=".",
            preloaded=None, write_outputs=True, verbose=True):
    """跑完整流程，返回结果 dict。write_outputs=False 时只计算、不写 csv/png。"""
    if preloaded is not None:
        samples, fs, bits, nchannels = preloaded
    else:
        samples, fs, bits, nchannels = wav_io.read_wav(wav_path)

    if nchannels != 4:
        raise ValueError(f"声道数不是 4（实际 {nchannels}），按 SPEC §1 报错退出")

    mic1 = samples[:, MIC1_COL].astype(np.float64)
    mic2 = samples[:, MIC2_COL].astype(np.float64)
    total = samples.shape[0]
    duration = total / fs

    # 分析时间段 [start, end]（默认整个文件）
    a_start = 0.0 if start is None else max(0.0, start)
    a_end = duration if end is None else min(duration, end)
    s_lo = int(round(a_start * fs))
    s_hi = int(round(a_end * fs))

    n = int(round(frame_ms / 1000.0 * fs))     # 帧长（采样点）
    hop = int(round(hop_ms / 1000.0 * fs))     # 帧移（采样点）
    fft_len = 1 << int(np.ceil(np.log2(2 * n)))

    # —— §4 噪声底 + 门槛 ——
    nf, nf_how = _noise_floor(mic1, mic2, n, hop, total, fs,
                              noise_start, noise_end)
    threshold = nf * (10.0 ** (vad_db / 10.0))

    # —— 逐帧处理 ——
    starts = _frame_starts(total, n, hop, s_lo, s_hi)
    rows = []
    for s in starts:
        time_s = (s + n / 2.0) / fs
        E = _frame_energy(mic1, mic2, s, n)
        voiced = 1 if E > threshold else 0
        if voiced:
            res = gcc_phat(mic1[s:s + n], mic2[s:s + n], fs, interp=interp)
            rows.append((time_s, res["tau_us"], res["angle_deg"],
                         res["confidence"], 1))
        else:
            rows.append((time_s, np.nan, np.nan, 0.0, 0))

    rows = np.array(rows, dtype=float)
    voiced_mask = rows[:, 4] == 1 if rows.size else np.zeros(0, dtype=bool)
    n_frames = rows.shape[0]
    n_voiced = int(voiced_mask.sum())
    voiced_pct = 100.0 * n_voiced / n_frames if n_frames else 0.0

    if n_voiced > 0:
        ang_med = float(np.median(rows[voiced_mask, 2]))
        conf_med = float(np.median(rows[voiced_mask, 3]))
    else:
        ang_med = float("nan")
        conf_med = float("nan")

    result = {
        "fs": fs, "bits": bits, "nchannels": nchannels,
        "duration": duration, "a_start": a_start, "a_end": a_end,
        "frame_n": n, "hop": hop, "fft_len": fft_len,
        "noise_floor": nf, "noise_how": nf_how, "threshold": threshold,
        "n_frames": n_frames, "n_voiced": n_voiced, "voiced_pct": voiced_pct,
        "angle_median": ang_med, "conf_median": conf_med,
        "interp": interp, "frame_ms": frame_ms, "hop_ms": hop_ms,
        "vad_db": vad_db, "rows": rows,
    }

    base = os.path.splitext(os.path.basename(wav_path))[0]
    tag = f"{base}_{int(round(a_start))}-{int(round(a_end))}_f{int(frame_ms)}_i{interp}"
    if write_outputs:
        os.makedirs(outdir, exist_ok=True)
        csv_path = os.path.join(outdir, tag + ".csv")
        png_path = os.path.join(outdir, tag + ".png")
        _write_csv(rows, csv_path)
        _plot(rows, png_path, base, fs, interp, frame_ms)
        result["csv_path"] = csv_path
        result["png_path"] = png_path

    if verbose:
        _print_summary(wav_path, result)

    return result


def _write_csv(rows, path):
    """§10 CSV：time_s, tau_us, angle_deg, confidence, voiced。"""
    lines = ["time_s,tau_us,angle_deg,confidence,voiced"]
    for t, tau, ang, conf, v in rows:
        tau_s = "" if np.isnan(tau) else f"{tau:.3f}"
        ang_s = "" if np.isnan(ang) else f"{ang:.3f}"
        lines.append(f"{t:.4f},{tau_s},{ang_s},{conf:.4f},{int(v)}")
    with open(path, "w", encoding="utf-8") as fh:
        fh.write("\n".join(lines) + "\n")


def _plot(rows, path, base, fs, interp, frame_ms):
    """§11 角度 vs 时间图。"""
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt

    fig, ax = plt.subplots(figsize=(12, 5))
    vmask = rows[:, 4] == 1
    if vmask.any():
        sc = ax.scatter(rows[vmask, 0], rows[vmask, 2],
                        c=rows[vmask, 3], cmap="viridis", vmin=0, vmax=1,
                        s=10)
        cb = fig.colorbar(sc, ax=ax)
        cb.set_label("confidence")

    # interp=1 的角度档位虚线
    for g in _noninterp_grid(fs):
        ax.axhline(g, ls="--", color="gray", lw=0.6, alpha=0.5)

    ax.set_ylim(-90, 90)
    ax.set_xlabel("time_s (whole-file time)")
    ax.set_ylabel("angle (deg)")
    ax.set_title(f"{base}  fs={fs}Hz  interp={interp}  frame={int(frame_ms)}ms")
    ax.grid(True, alpha=0.2)
    fig.tight_layout()
    fig.savefig(path, dpi=120)
    plt.close(fig)


def _print_summary(wav_path, r):
    """§9 终端摘要。"""
    print(f"文件: {wav_path}")
    print(f"  fs / 位深 / 声道数 : {r['fs']} Hz / {r['bits']}-bit / {r['nchannels']}")
    print(f"  文件时长           : {r['duration']:.2f} s")
    print(f"  分析时间段         : [{r['a_start']:.2f}, {r['a_end']:.2f}] s")
    print(f"  每帧采样点数 / FFT : {r['frame_n']} / {r['fft_len']}")
    print(f"  噪声底             : {r['noise_floor']:.4g}  ({r['noise_how']})")
    print(f"  VAD 门槛           : {r['threshold']:.4g}  (= 噪声底 ×10^({r['vad_db']}/10))")
    print(f"  帧数 / voiced      : {r['n_frames']} / {r['n_voiced']}"
          f"  (voiced 占比 {r['voiced_pct']:.1f}%)")
    print(f"  voiced 角度中位数  : {r['angle_median']:.2f}°")
    print(f"  voiced conf 中位数 : {r['conf_median']:.3f}")
    if "csv_path" in r:
        print(f"  CSV: {r['csv_path']}")
        print(f"  PNG: {r['png_path']}")


def _build_parser():
    p = argparse.ArgumentParser(description="单板双麦 GCC-PhaT 方向估计")
    p.add_argument("wav")
    p.add_argument("--start", type=float, default=None)
    p.add_argument("--end", type=float, default=None)
    p.add_argument("--interp", type=int, default=DEF_INTERP)
    p.add_argument("--frame-ms", type=float, default=DEF_FRAME_MS)
    p.add_argument("--hop-ms", type=float, default=DEF_HOP_MS)
    p.add_argument("--vad-db", type=float, default=DEF_VAD_DB)
    p.add_argument("--noise-start", type=float, default=None)
    p.add_argument("--noise-end", type=float, default=None)
    p.add_argument("--outdir", default=".")
    return p


if __name__ == "__main__":
    args = _build_parser().parse_args()
    analyze(args.wav, start=args.start, end=args.end, interp=args.interp,
            frame_ms=args.frame_ms, hop_ms=args.hop_ms, vad_db=args.vad_db,
            noise_start=args.noise_start, noise_end=args.noise_end,
            outdir=args.outdir, write_outputs=True, verbose=True)
