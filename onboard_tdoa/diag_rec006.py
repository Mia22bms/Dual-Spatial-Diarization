"""
diag_rec006.py — Board2_REC_006 诊断脚本（只读，不改任何现有文件/参数）

复用 wav_io.read_wav 和 gcc_phat.gcc_phat 的现有计算。
固定参数（与 SPEC 一致，不改）：帧长 64 ms、帧移 32 ms、interp 8、频带 125–3500 Hz。
所有输出文件加 diag_ 前缀，不覆盖已有结果。

产物：
- diag_allframes.csv       每一帧（不经 VAD）的 τ/angle/confidence/energy/energy_db
- diag_energy_tiers.csv     按 energy 百分位分 5 档的统计
- diag_speaker_loudness.csv A/C/D 各自按 energy 中位数分高低两半的统计
"""

import numpy as np

import wav_io
from gcc_phat import gcc_phat, C_SOUND, D_MIC, M_MARGIN

WAV = "Board2_REC_006.wav"
SEG_FILE = "segments.txt"

FRAME_MS = 64.0
HOP_MS = 32.0
INTERP = 8
NOISE_PERCENTILE = 10          # SPEC §4 默认
MIC1_COL, MIC2_COL = 1, 3      # SPEC §1.1


def frame_energy(mic1, mic2, s, n):
    """SPEC §4：两路、加窗前、每采样点平方后的平均。"""
    a = mic1[s:s + n]
    b = mic2[s:s + n]
    return (np.dot(a, a) + np.dot(b, b)) / (2.0 * n)


def interp8_angle_grid(fs):
    """interp=8 在该 fs 下、搜索范围内可取到的角度档位（排序、去重）。"""
    tau_max = D_MIC / C_SOUND
    max_k = int(np.floor((1.0 + M_MARGIN) * tau_max * fs * INTERP))
    grid = []
    for k in range(-max_k, max_k + 1):
        tau = k / (fs * INTERP)
        x = C_SOUND * tau / D_MIC
        if abs(x) <= 1.0:
            grid.append(round(float(np.degrees(np.arcsin(x))), 2))
        elif abs(x) <= 1.0 + M_MARGIN:
            grid.append(90.0 if x > 0 else -90.0)
    return sorted(set(grid))


def nearest_grid(angle, grid):
    """把角度归到最近的档位（用于直方图计数）。"""
    g = np.asarray(grid)
    return float(g[int(np.argmin(np.abs(g - angle)))])


def compute_allframes():
    samples, fs, bits, nch = wav_io.read_wav(WAV)
    mic1 = samples[:, MIC1_COL].astype(np.float64)
    mic2 = samples[:, MIC2_COL].astype(np.float64)
    total = samples.shape[0]
    n = int(round(FRAME_MS / 1000.0 * fs))
    hop = int(round(HOP_MS / 1000.0 * fs))

    starts = np.arange(0, total - n + 1, hop, dtype=int)
    energies = np.array([frame_energy(mic1, mic2, s, n) for s in starts])
    noise_floor = float(np.percentile(energies, NOISE_PERCENTILE))

    recs = []
    for s, E in zip(starts, energies):
        res = gcc_phat(mic1[s:s + n], mic2[s:s + n], fs, interp=INTERP)
        recs.append((
            (s + n / 2.0) / fs,            # time_s
            res["tau_us"], res["angle_deg"], res["confidence"],
            E, 10.0 * np.log10(E / noise_floor),
        ))
    recs = np.array(recs, dtype=float)
    return recs, fs, noise_floor


def write_allframes(recs, path):
    lines = ["time_s,tau_us,angle_deg,confidence,energy,energy_db"]
    for t, tau, ang, conf, E, edb in recs:
        lines.append(f"{t:.4f},{tau:.3f},{ang:.3f},{conf:.4f},{E:.4f},{edb:.3f}")
    with open(path, "w", encoding="utf-8") as fh:
        fh.write("\n".join(lines) + "\n")


def _stats(angs, confs):
    return (f"{np.median(angs):.2f}", f"{np.percentile(angs, 25):.2f}",
            f"{np.percentile(angs, 75):.2f}", f"{np.median(confs):.3f}")


def energy_tiers(recs, grid, path):
    E = recs[:, 4]
    edges = np.percentile(E, [0, 20, 40, 60, 80, 100])
    labels = ["0-20", "20-40", "40-60", "60-80", "80-100"]
    header = ("tier_pct,n_frames,energy_db_min,energy_db_max,"
              "angle_median,angle_p25,angle_p75,conf_median,angle_hist")
    lines = [header]
    for i, lab in enumerate(labels):
        lo, hi = edges[i], edges[i + 1]
        if i < 4:
            mask = (E >= lo) & (E < hi)
        else:
            mask = (E >= lo) & (E <= hi)
        sub = recs[mask]
        if sub.shape[0] == 0:
            lines.append(f"{lab},0,,,,,,,")
            continue
        angs, confs, edb = sub[:, 2], sub[:, 3], sub[:, 5]
        a_med, a25, a75, c_med = _stats(angs, confs)
        # 每个角度档位的帧数
        counts = {}
        for a in angs:
            g = nearest_grid(a, grid)
            counts[g] = counts.get(g, 0) + 1
        hist = ";".join(f"{g:g}:{counts[g]}" for g in sorted(counts))
        lines.append(f"{lab},{sub.shape[0]},{edb.min():.2f},{edb.max():.2f},"
                     f"{a_med},{a25},{a75},{c_med},{hist}")
    with open(path, "w", encoding="utf-8") as fh:
        fh.write("\n".join(lines) + "\n")
    return lines


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


def speaker_loudness(recs, segs, path):
    t = recs[:, 0]
    header = ("speaker,half,n_frames,energy_db_median,"
              "angle_median,angle_p25,angle_p75,conf_median")
    lines = [header]
    for spk in sorted(segs):
        # 该说话人所有帧（时间落在其任一段内）
        mask = np.zeros(recs.shape[0], dtype=bool)
        for s, e in segs[spk]:
            mask |= (t >= s) & (t < e)
        sub = recs[mask]
        if sub.shape[0] == 0:
            continue
        med_E = np.median(sub[:, 4])
        for half, hmask in (("low", sub[:, 4] <= med_E),
                            ("high", sub[:, 4] > med_E)):
            h = sub[hmask]
            if h.shape[0] == 0:
                lines.append(f"{spk},{half},0,,,,,")
                continue
            a_med, a25, a75, c_med = _stats(h[:, 2], h[:, 3])
            lines.append(f"{spk},{half},{h.shape[0]},"
                         f"{np.median(h[:, 5]):.2f},{a_med},{a25},{a75},{c_med}")
    with open(path, "w", encoding="utf-8") as fh:
        fh.write("\n".join(lines) + "\n")
    return lines


if __name__ == "__main__":
    recs, fs, nf = compute_allframes()
    grid = interp8_angle_grid(fs)

    write_allframes(recs, "diag_allframes.csv")
    tier_lines = energy_tiers(recs, grid, "diag_energy_tiers.csv")
    spk_lines = speaker_loudness(recs, read_segments(SEG_FILE),
                                 "diag_speaker_loudness.csv")

    print(f"总帧数: {recs.shape[0]}  fs: {fs}  噪声底: {nf:.4g}")
    print("\n[diag_energy_tiers.csv]")
    print("\n".join(tier_lines))
    print("\n[diag_speaker_loudness.csv]")
    print("\n".join(spk_lines))
    print("\n文件: diag_allframes.csv, diag_energy_tiers.csv, diag_speaker_loudness.csv")
