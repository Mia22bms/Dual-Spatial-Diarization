"""
gcc_phat.py — 单帧 GCC-PhaT 方向估计（TDOA 项目）

输入两路已经切好的帧（MIC1、MIC2）、fs、interp，按 TDOA_SPEC.md §3 的
第 2、4–9 步，返回 tau_us、angle_deg、confidence。

约定（来自 SPEC）：
- 互谱 G(f) = X2(f)·conj(X1(f))（X1=MIC1, X2=MIC2），峰出现在 +τ
- 默认不做抛物线拟合（parabolic=False）；可选 parabolic=True 对峰及左右相邻点做抛物线
  拟合取顶点做亚采样点细化。interp=1 且未启用 parabolic 时严格取整数采样点峰
- confidence 按 §5 直接在频带内求和，不从 GCC 峰值反推
"""

import numpy as np

# —— SPEC 里的常量默认值 ——
C_SOUND = 343.0      # 声速 m/s（约 20 ℃）
D_MIC = 0.036        # 两麦间距 m
FMIN = 125.0         # 频带下限 Hz
FMAX = 3500.0        # 频带上限 Hz
M_MARGIN = 0.1       # 出界容差 m
EPS = 1e-10          # PhaT 防除零


def _next_pow2_ge(x):
    """返回 ≥ x 的最小 2 的幂。"""
    return 1 << (int(np.ceil(np.log2(x))))


def gcc_phat(frame1, frame2, fs, interp=8, parabolic=False,
             d=D_MIC, c=C_SOUND, fmin=FMIN, fmax=FMAX,
             m=M_MARGIN, eps=EPS, return_curve=False):
    """对一帧估计方向。

    frame1: MIC1 帧 (1D), frame2: MIC2 帧 (1D), 等长 N
    parabolic: False=整数插值峰（原样）；True=对峰及左右相邻点做抛物线拟合，取顶点
    return_curve: True 时额外返回搜索窗内的 GCC-PhaT 曲线 curve 及对应 lags（秒）
    返回 dict: tau_us, angle_deg, confidence, tau_s [, curve, lags]
    """
    frame1 = np.asarray(frame1, dtype=np.float64)
    frame2 = np.asarray(frame2, dtype=np.float64)
    n = frame1.shape[0]
    if frame2.shape[0] != n:
        raise ValueError("两路帧长度必须相同")

    # —— 步骤 2：两路乘同一个 Hann 窗 ——
    win = np.hanning(n)
    x1 = frame1 * win
    x2 = frame2 * win

    # —— 步骤 4：FFT 长度 L = 不小于 2N 的 2 的幂（避免循环互相关回绕）——
    L = _next_pow2_ge(2 * n)
    X1 = np.fft.rfft(x1, L)
    X2 = np.fft.rfft(x2, L)
    freqs = np.fft.rfftfreq(L, 1.0 / fs)

    # —— 步骤 5：互谱（顺序不能反）——
    G = X2 * np.conj(X1)

    # —— 步骤 6：PhaT 加权 + 频带 ——
    R = G / (np.abs(G) + eps)
    band = (freqs >= fmin) & (freqs <= fmax)
    R_band = R * band  # 频带外置 0，用于逆变换找峰

    # —— 步骤 7：频域补零到 L*interp 再逆变换（= 时域 sinc 上采样），找峰 ——
    M = L * int(interp)
    r = np.fft.irfft(R_band, n=M)          # 实数输出
    r = np.fft.fftshift(r)                  # 把 lag=0 移到中间
    lags = (np.arange(M) - M // 2) / (fs * interp)  # 每个点对应的延迟（秒）

    tau_max = d / c
    search = (1.0 + m) * tau_max
    in_win = np.abs(lags) <= search
    sel_r = r[in_win]
    sel_lags = lags[in_win]
    k = int(np.argmax(sel_r))               # 峰位置
    tau = sel_lags[k]                        # 整数插值峰即 τ（秒）

    # —— 可选：抛物线亚采样细化（对峰及左右相邻点拟合，取顶点）——
    if parabolic and 0 < k < sel_r.size - 1:
        y0, y1, y2 = sel_r[k - 1], sel_r[k], sel_r[k + 1]
        denom = y0 - 2.0 * y1 + y2
        if denom != 0.0:
            delta = 0.5 * (y0 - y2) / denom          # 顶点相对 k 的偏移（上采样点）
            tau = sel_lags[k] + delta / (fs * interp)  # 换算回秒

    # —— 步骤 9：confidence 按 §5 直接在频带内求和 ——
    fb = freqs[band]
    Rb = R[band]
    K = fb.size
    conf = float(np.mean(np.real(Rb * np.exp(1j * 2 * np.pi * fb * tau)))) if K > 0 else 0.0
    conf = min(max(conf, 0.0), 1.0)

    # —— 步骤 8：换算角度 + §2.4 出界处理 ——
    x = c * tau / d
    ax = abs(x)
    if ax <= 1.0:
        angle = np.degrees(np.arcsin(x))
        penalty = 1.0
    elif ax <= 1.0 + m:
        angle = 90.0 if x > 0 else -90.0
        penalty = 0.5
    else:
        # 搜索范围已限制在 ±(1+m)τ_max，正常不会到这里
        angle = float("nan")
        penalty = 0.0

    confidence = conf * penalty

    out = {
        "tau_us": tau * 1e6,     # 照实记录原始 τ（微秒）
        "angle_deg": angle,
        "confidence": confidence,
        "tau_s": tau,
    }
    if return_curve:
        out["curve"] = sel_r          # 搜索窗内 GCC-PhaT 曲线（未归一化）
        out["lags"] = sel_lags        # 每点对应的延迟（秒），与 curve 等长
    return out
