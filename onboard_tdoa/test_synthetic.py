"""
test_synthetic.py — 合成信号测试（TDOA_SPEC.md §12-A）

用代码自己生成已知延迟的两路信号，只调用 gcc_phat，检验算法本身。
延迟用频域相移实现小数采样点延迟（不是整数平移）。

条件全组合：
- 源信号：白噪音 / 类语音
- fs：16 kHz / 48 kHz
- 噪音：无噪音 / SNR≈10 dB
- interp：1 / 8
- 角度：0, +30, −30, +60, +90

输出：误差表（csv + 终端打印）。
"""

import numpy as np
import wav_io  # noqa: F401  仅为让 stdout 变成 UTF-8
from gcc_phat import gcc_phat, C_SOUND, D_MIC

ANGLES = [0.0, 30.0, -30.0, 60.0, 90.0]
FS_LIST = [16000, 48000]
SNR_LIST = [None, 10.0]       # None = 无噪音
INTERP_LIST = [1, 8]
SOURCES = ["noise", "speech"]
FRAME_MS = 64.0               # SPEC 默认帧长
SEED = 12345

# —— 统计模式参数 ——
STATS_ANGLES = list(range(0, 91, 10))   # 0..90 每 10°
STATS_NFRAMES = 200                      # 每种条件独立随机跑的帧数
STATS_SEED = 20260


def true_tau(angle_deg, d=D_MIC, c=C_SOUND):
    """真实 τ (秒) = d·sinθ/c。"""
    return d * np.sin(np.radians(angle_deg)) / c


def frac_delay(sig, tau_s, fs):
    """用频域相移把信号整体延迟 tau_s 秒（支持小数采样点）。"""
    n = sig.size
    S = np.fft.rfft(sig)
    f = np.fft.rfftfreq(n, 1.0 / fs)
    S2 = S * np.exp(-1j * 2 * np.pi * f * tau_s)  # 正延迟 = s(t-τ)
    return np.fft.irfft(S2, n=n)


def make_source(kind, n, fs, rng):
    """生成源信号：白噪音或类语音（带通噪音 + 音节包络）。"""
    if kind == "noise":
        return rng.standard_normal(n)
    # speech：把白噪音带通到 300–3400 Hz，再加 ~3.5 Hz 的音节包络
    x = rng.standard_normal(n)
    X = np.fft.rfft(x)
    f = np.fft.rfftfreq(n, 1.0 / fs)
    X *= (f >= 300) & (f <= 3400)
    y = np.fft.irfft(X, n=n)
    t = np.arange(n) / fs
    env = 0.6 + 0.4 * np.sin(2 * np.pi * 3.5 * t)
    return y * env


def add_noise(sig, snr_db, rng):
    """按给定 SNR 加独立白噪音。"""
    p = np.mean(sig ** 2)
    pn = p / (10 ** (snr_db / 10.0))
    return sig + rng.standard_normal(sig.size) * np.sqrt(pn)


def run():
    rng = np.random.default_rng(SEED)
    rows = []

    for src in SOURCES:
        for fs in FS_LIST:
            frame_n = int(round(FRAME_MS / 1000.0 * fs))
            gen_n = frame_n * 3  # 多生成一些，切中间一帧避免相移环绕的边缘
            for angle in ANGLES:
                tau_t = true_tau(angle)
                # 生成源，MIC2 = MIC1 延迟 tau_t
                source = make_source(src, gen_n, fs, rng)
                mic1_full = source
                mic2_full = frac_delay(source, tau_t, fs)
                for snr in SNR_LIST:
                    m1 = mic1_full.copy()
                    m2 = mic2_full.copy()
                    if snr is not None:
                        m1 = add_noise(m1, snr, rng)
                        m2 = add_noise(m2, snr, rng)
                    # 切中间一帧
                    s0 = (gen_n - frame_n) // 2
                    f1 = m1[s0:s0 + frame_n]
                    f2 = m2[s0:s0 + frame_n]
                    for interp in INTERP_LIST:
                        res = gcc_phat(f1, f2, fs, interp=interp)
                        rows.append({
                            "source": src,
                            "fs": fs,
                            "snr_db": "inf" if snr is None else f"{snr:g}",
                            "interp": interp,
                            "angle_true_deg": angle,
                            "tau_true_us": tau_t * 1e6,
                            "tau_est_us": res["tau_us"],
                            "angle_est_deg": res["angle_deg"],
                            "angle_err_deg": res["angle_deg"] - angle,
                            "confidence": res["confidence"],
                        })
    return rows


def write_csv(rows, path):
    cols = ["source", "fs", "snr_db", "interp", "angle_true_deg",
            "tau_true_us", "tau_est_us", "angle_est_deg",
            "angle_err_deg", "confidence"]
    lines = [",".join(cols)]
    for r in rows:
        lines.append(",".join([
            r["source"], str(r["fs"]), r["snr_db"], str(r["interp"]),
            f"{r['angle_true_deg']:.1f}", f"{r['tau_true_us']:.2f}",
            f"{r['tau_est_us']:.2f}", f"{r['angle_est_deg']:.2f}",
            f"{r['angle_err_deg']:.2f}", f"{r['confidence']:.3f}",
        ]))
    with open(path, "w", encoding="utf-8") as fh:
        fh.write("\n".join(lines) + "\n")


def print_table(rows):
    hdr = (f"{'src':6} {'fs':>6} {'snr':>4} {'itp':>3} "
           f"{'θtrue':>6} {'τtrue':>8} {'τest':>8} {'θest':>7} "
           f"{'err':>7} {'conf':>5}")
    print(hdr)
    print("-" * len(hdr))
    for r in rows:
        print(f"{r['source']:6} {r['fs']:>6} {r['snr_db']:>4} {r['interp']:>3} "
              f"{r['angle_true_deg']:>6.1f} {r['tau_true_us']:>8.2f} "
              f"{r['tau_est_us']:>8.2f} {r['angle_est_deg']:>7.2f} "
              f"{r['angle_err_deg']:>7.2f} {r['confidence']:>5.3f}")


def check(rows):
    """按 §12-A 的通过标准做自动检查。"""
    print("\n=== 通过标准检查 ===")
    # 1. 正负号全部正确（排除 angle 恰为 0 的）
    sign_bad = [r for r in rows
                if r["angle_true_deg"] != 0
                and np.sign(r["angle_est_deg"]) != np.sign(r["angle_true_deg"])]
    print(f"[符号] 符号错误的行数: {len(sign_bad)}")

    # 2. 抽查 16k +30° → τ≈+52.5µs（看无噪 interp8 噪音源那行）
    spot = [r for r in rows if r["source"] == "noise" and r["fs"] == 16000
            and r["snr_db"] == "inf" and r["interp"] == 8
            and r["angle_true_deg"] == 30.0]
    if spot:
        print(f"[抽查] 16k +30° 无噪 interp8: τ_est = {spot[0]['tau_est_us']:.2f} µs "
              f"(真值 {spot[0]['tau_true_us']:.2f} µs)")

    # 3. 无噪 interp8 误差应远小于一档
    clean8 = [r for r in rows if r["snr_db"] == "inf" and r["interp"] == 8]
    max_err = max(abs(r["angle_err_deg"]) for r in clean8)
    print(f"[误差] 无噪 interp8 最大 |角度误差| = {max_err:.2f}°  (一档≈37°)")

    # 4. 无噪 confidence 接近 1
    min_conf = min(r["confidence"] for r in clean8)
    print(f"[可信] 无噪 interp8 最小 confidence = {min_conf:.3f}")


def run_stats():
    """统计模式：每种条件跑 STATS_NFRAMES 帧（每帧独立随机源+噪音），
    角度 0..90 每 10°，只用 noise 源。
    返回按 (fs, snr, interp, angle) 聚合的统计行。
    """
    rng = np.random.default_rng(STATS_SEED)
    stats_rows = []
    for fs in FS_LIST:
        frame_n = int(round(FRAME_MS / 1000.0 * fs))
        gen_n = frame_n * 3
        for snr in SNR_LIST:
            for interp in INTERP_LIST:
                for angle in STATS_ANGLES:
                    tau_t = true_tau(angle)
                    abs_errs = np.empty(STATS_NFRAMES)
                    confs = np.empty(STATS_NFRAMES)
                    for k in range(STATS_NFRAMES):
                        # 每帧独立随机源信号
                        source = make_source("noise", gen_n, fs, rng)
                        m1 = source
                        m2 = frac_delay(source, tau_t, fs)
                        if snr is not None:
                            m1 = add_noise(m1, snr, rng)
                            m2 = add_noise(m2, snr, rng)
                        s0 = (gen_n - frame_n) // 2
                        res = gcc_phat(m1[s0:s0 + frame_n],
                                       m2[s0:s0 + frame_n], fs, interp=interp)
                        abs_errs[k] = abs(res["angle_deg"] - angle)
                        confs[k] = res["confidence"]
                    stats_rows.append({
                        "fs": fs,
                        "snr_db": "inf" if snr is None else f"{snr:g}",
                        "interp": interp,
                        "angle_true_deg": angle,
                        "nframes": STATS_NFRAMES,
                        "abserr_median_deg": float(np.median(abs_errs)),
                        "abserr_p90_deg": float(np.percentile(abs_errs, 90)),
                        "conf_median": float(np.median(confs)),
                    })
    return stats_rows


def write_stats_csv(rows, path):
    cols = ["fs", "snr_db", "interp", "angle_true_deg", "nframes",
            "abserr_median_deg", "abserr_p90_deg", "conf_median"]
    lines = [",".join(cols)]
    for r in rows:
        lines.append(",".join([
            str(r["fs"]), r["snr_db"], str(r["interp"]),
            f"{r['angle_true_deg']:.0f}", str(r["nframes"]),
            f"{r['abserr_median_deg']:.3f}", f"{r['abserr_p90_deg']:.3f}",
            f"{r['conf_median']:.3f}",
        ]))
    with open(path, "w", encoding="utf-8") as fh:
        fh.write("\n".join(lines) + "\n")


def plot_stats(rows, path):
    """画 interp8 的四条线：16k/48k × 无噪/10dB，横轴角度、纵轴 |误差|中位数。"""
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt

    plt.rcParams["font.family"] = ["DejaVu Sans"]
    fig, ax = plt.subplots(figsize=(8, 5))
    styles = {
        (16000, "inf"): ("16k clean", "o-", "#1f77b4"),
        (16000, "10"):  ("16k 10dB",  "o--", "#1f77b4"),
        (48000, "inf"): ("48k clean", "s-", "#d62728"),
        (48000, "10"):  ("48k 10dB",  "s--", "#d62728"),
    }
    for (fs, snr), (label, style, color) in styles.items():
        sub = [r for r in rows if r["fs"] == fs and r["snr_db"] == snr
               and r["interp"] == 8]
        sub.sort(key=lambda r: r["angle_true_deg"])
        xs = [r["angle_true_deg"] for r in sub]
        ys = [r["abserr_median_deg"] for r in sub]
        ax.plot(xs, ys, style, color=color, label=label)

    ax.set_xlabel("true angle (deg)")
    ax.set_ylabel("median |angle error| (deg)")
    ax.set_title(f"Synthetic noise source, interp=8, {STATS_NFRAMES} frames/condition")
    ax.grid(True, alpha=0.3)
    ax.legend()
    fig.tight_layout()
    fig.savefig(path, dpi=120)
    plt.close(fig)


def print_stats(rows):
    hdr = (f"{'fs':>6} {'snr':>4} {'itp':>3} {'θtrue':>6} "
           f"{'errMed':>7} {'errP90':>7} {'confMed':>7}")
    print(hdr)
    print("-" * len(hdr))
    for r in rows:
        print(f"{r['fs']:>6} {r['snr_db']:>4} {r['interp']:>3} "
              f"{r['angle_true_deg']:>6.0f} {r['abserr_median_deg']:>7.2f} "
              f"{r['abserr_p90_deg']:>7.2f} {r['conf_median']:>7.3f}")


if __name__ == "__main__":
    # —— 原 80 行表（保留不动）——
    rows = run()
    csv_path = "synthetic_results.csv"
    try:
        write_csv(rows, csv_path)
        wrote = f"CSV 已写出: {csv_path}  (共 {len(rows)} 行)"
    except PermissionError:
        wrote = f"[跳过] {csv_path} 被占用（可能在 Excel 中打开），未重写；内容不变"
    print_table(rows)
    check(rows)
    print(f"\n{wrote}")

    # —— 统计模式 ——
    print("\n" + "=" * 60)
    print(f"统计模式：noise 源，角度 0..90 每 10°，每条件 {STATS_NFRAMES} 帧")
    print("=" * 60)
    srows = run_stats()
    stats_csv = "synthetic_stats.csv"
    write_stats_csv(srows, stats_csv)
    print_stats(srows)
    stats_png = "synthetic_stats.png"
    plot_stats(srows, stats_png)
    print(f"\n统计 CSV 已写出: {stats_csv}  (共 {len(srows)} 行)")
    print(f"统计图已写出: {stats_png}")
