"""
wav_io.py — 读 WAV 的基础模块（TDOA 项目第一阶段）

职责（本阶段只做这些）：
- 从文件头读取：采样率 fs、位深 bits、声道数 nchannels（一律不写死）
- 支持 16-bit 和 24-bit PCM
- 把样点解码成 numpy 数组，形状 (nframes, nchannels)
- 直接运行本文件会打印摘要：fs、位深、声道数、时长、每轨最大幅度

实现说明：
手工解析 RIFF 分块，不用标准库 wave 模块。原因是 ESP32 多声道 24-bit 录音
常用 WAVE_FORMAT_EXTENSIBLE（格式码 0xFFFE），wave 模块对这种格式有时会报错；
手工解析对 16/24-bit 都能稳定处理，也方便后续按 SPEC §1.1 取第 1、3 列。
"""

import sys
import struct
import numpy as np

# 本项目统一用 UTF-8 输出，避免 Windows 控制台把中文显示成乱码
try:
    sys.stdout.reconfigure(encoding="utf-8")
except (AttributeError, ValueError):
    pass

# WAV fmt chunk 里的格式码
WAVE_FORMAT_PCM = 0x0001
WAVE_FORMAT_EXTENSIBLE = 0xFFFE


def read_wav(path):
    """读 WAV，返回 (samples, fs, bits, nchannels)。

    samples: np.ndarray，形状 (nframes, nchannels)，dtype=int32，
             数值是原始整数样点（未归一化）。
    fs:       采样率 (Hz)，从文件头读。
    bits:     位深 (16 或 24)，从文件头读。
    nchannels: 声道数，从文件头读。
    """
    with open(path, "rb") as f:
        raw = f.read()

    # ---- RIFF 头 ----
    if raw[0:4] != b"RIFF" or raw[8:12] != b"WAVE":
        raise ValueError(f"不是合法的 RIFF/WAVE 文件: {path}")

    fmt = None
    data_bytes = None

    # ---- 遍历分块，找 'fmt ' 和 'data' ----
    pos = 12
    while pos + 8 <= len(raw):
        chunk_id = raw[pos:pos + 4]
        chunk_size = struct.unpack_from("<I", raw, pos + 4)[0]
        body_start = pos + 8

        if chunk_id == b"fmt ":
            fmt = _parse_fmt(raw, body_start, chunk_size)
        elif chunk_id == b"data":
            # data 块声明大小 vs 实际剩余字节数：不一致不静默截断，先警告
            available = len(raw) - body_start
            if chunk_size > available:
                print(f"[警告] data 块声明 {chunk_size} 字节，但文件只剩 "
                      f"{available} 字节，少了 {chunk_size - available} 字节；"
                      f"按实际可用字节读取。")
                chunk_size = available
            data_bytes = raw[body_start:body_start + chunk_size]

        # 分块按偶数字节对齐
        pos = body_start + chunk_size + (chunk_size & 1)

    if fmt is None:
        raise ValueError("没找到 fmt 分块")
    if data_bytes is None:
        raise ValueError("没找到 data 分块")

    audio_format, nchannels, fs, bits, block_align = fmt

    if bits not in (16, 24):
        raise ValueError(f"只支持 16-bit 和 24-bit，文件是 {bits}-bit")

    # 校验 block_align：每个采样帧（所有声道一组）应为 nchannels * bits/8 字节
    expected_align = nchannels * bits // 8
    if block_align != expected_align:
        raise ValueError(
            f"block_align 不一致：文件头声明 {block_align}，"
            f"但 nchannels({nchannels}) × bits({bits})/8 = {expected_align}")

    samples = _decode_pcm(data_bytes, bits, nchannels)
    return samples, fs, bits, nchannels


def _parse_fmt(raw, start, size):
    """解析 fmt 分块，返回 (audio_format, nchannels, fs, bits, block_align)。"""
    audio_format, nchannels, fs, _byte_rate, block_align, bits = \
        struct.unpack_from("<HHIIHH", raw, start)
    # EXTENSIBLE：真正的子格式在后面的 GUID 里，但 bits/nchannels/fs 仍然有效，
    # 对 PCM 来说直接用这些字段即可。
    return audio_format, nchannels, fs, bits, block_align


def _decode_pcm(data_bytes, bits, nchannels):
    """把原始 PCM 字节解码成 (nframes, nchannels) 的 int32 数组。"""
    if bits == 16:
        flat = np.frombuffer(data_bytes, dtype="<i2").astype(np.int32)
    elif bits == 24:
        # 24-bit = 每样点 3 字节，小端。numpy 没有 int24，手工拼并符号扩展。
        b = np.frombuffer(data_bytes, dtype=np.uint8)
        n = b.size // 3
        b = b[:n * 3].reshape(n, 3).astype(np.int32)
        flat = b[:, 0] | (b[:, 1] << 8) | (b[:, 2] << 16)
        # 最高位为 1 → 负数，减 2^24 做符号扩展
        flat = np.where(flat >= (1 << 23), flat - (1 << 24), flat)
    else:
        raise ValueError(f"不支持的位深: {bits}")

    nframes = flat.size // nchannels
    return flat[:nframes * nchannels].reshape(nframes, nchannels)


def _summary(path):
    """打印本阶段的自检摘要。"""
    samples, fs, bits, nchannels = read_wav(path)
    nframes = samples.shape[0]
    duration = nframes / fs
    full_scale = float(1 << (bits - 1))  # 满量程绝对值，用于算相对幅度

    print(f"文件: {path}")
    print(f"采样率 fs      : {fs} Hz           (从文件头读)")
    print(f"位深           : {bits}-bit")
    print(f"声道数         : {nchannels}")
    print(f"总样点数/轨    : {nframes}")
    print(f"时长           : {duration:.2f} s  ({int(duration)//60} 分 {duration % 60:.2f} 秒)")
    print(f"满量程 |max|   : {int(full_scale)}  (= 2^{bits - 1})")
    print("每轨最大幅度:")
    for ch in range(nchannels):
        peak = int(np.max(np.abs(samples[:, ch])))
        pct = 100.0 * peak / full_scale
        tag = ""
        if ch == 1:
            tag = "  <- 轨2 = MIC1 (SPEC §1.1)"
        elif ch == 3:
            tag = "  <- 轨4 = MIC2 (SPEC §1.1)"
        print(f"  列{ch} (轨{ch + 1}): |max| = {peak:>8d}  ({pct:5.1f}% 满量程){tag}")

    if nchannels != 4:
        print(f"\n[注意] 声道数不是 4（实际 {nchannels}）。按 SPEC §1，正式流程此时应报错退出。")


if __name__ == "__main__":
    import sys
    wav_path = sys.argv[1] if len(sys.argv) > 1 else "Board2_REC_006.wav"
    _summary(wav_path)
