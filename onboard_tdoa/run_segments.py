"""
run_segments.py — 按 segments.txt 分段跑 single_board_doa（TDOA 项目）

读 segments.txt（每行：label start end），每段分别调用 single_board_doa，
interp 1 和 interp 8 各跑一次，汇总到 segments_summary.csv。

汇总列：label, start, end, interp, voiced 帧数,
         角度中位数, 角度25百分位, 角度75百分位, confidence中位数
"""

import numpy as np

import wav_io
from single_board_doa import analyze

WAV = "Board2_REC_006.wav"
SEG_FILE = "segments.txt"
OUT_CSV = "segments_summary.csv"
INTERPS = [1, 8]


def read_segments(path):
    segs = []
    with open(path, "r", encoding="utf-8") as fh:
        for line in fh:
            line = line.strip()
            if not line or line.startswith("#"):
                continue
            parts = line.split()
            segs.append((parts[0], float(parts[1]), float(parts[2])))
    return segs


def main():
    segs = read_segments(SEG_FILE)
    # 整个文件只读一次，分段共用（噪声底仍按 SPEC 从整个文件算）
    preloaded = wav_io.read_wav(WAV)

    out = ["label,start,end,interp,voiced_frames,"
           "angle_median,angle_p25,angle_p75,conf_median"]
    for label, start, end in segs:
        for interp in INTERPS:
            r = analyze(WAV, start=start, end=end, interp=interp,
                        preloaded=preloaded, write_outputs=False, verbose=False)
            rows = r["rows"]
            vmask = rows[:, 4] == 1 if rows.size else np.zeros(0, dtype=bool)
            nv = int(vmask.sum())
            if nv > 0:
                angs = rows[vmask, 2]
                confs = rows[vmask, 3]
                a_med = f"{np.median(angs):.2f}"
                a_p25 = f"{np.percentile(angs, 25):.2f}"
                a_p75 = f"{np.percentile(angs, 75):.2f}"
                c_med = f"{np.median(confs):.3f}"
            else:
                a_med = a_p25 = a_p75 = c_med = ""
            out.append(f"{label},{start:g},{end:g},{interp},{nv},"
                       f"{a_med},{a_p25},{a_p75},{c_med}")

    with open(OUT_CSV, "w", encoding="utf-8") as fh:
        fh.write("\n".join(out) + "\n")

    # 终端也打印一份
    print("\n".join(out))
    print(f"\n汇总已写出: {OUT_CSV}")


if __name__ == "__main__":
    main()
