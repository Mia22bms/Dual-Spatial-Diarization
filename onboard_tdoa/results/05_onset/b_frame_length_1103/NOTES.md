# 05_onset / b_frame_length_1103

- Date (file mtime): 2026-11-03
- Generating script: `single_board_doa.py`
- Recordings: `recordings/REC_016.WAV`, `REC_018.WAV`
- Frame lengths: 32 ms (hop 16 ms) and 16 ms (hop 8 ms), keeping hop = frame/2.
- Onset rule for `_on3` files: `--onset-ref prev2` (default), onset_db 3.
- Reproduce (write to `results/scratch/`; files were then moved here), run from `onboard_tdoa/`:
  ```
  # 32 ms (example REC_016 segment 9-24 s), with and without onset:
  python single_board_doa.py recordings/REC_016.WAV --start 9 --end 24 --interp 8 --frame-ms 32 --hop-ms 16
  python single_board_doa.py recordings/REC_016.WAV --start 9 --end 24 --interp 8 --frame-ms 32 --hop-ms 16 --onset-db 3
  # 16 ms:
  python single_board_doa.py recordings/REC_016.WAV --start 9 --end 24 --interp 8 --frame-ms 16 --hop-ms 8
  python single_board_doa.py recordings/REC_016.WAV --start 9 --end 24 --interp 8 --frame-ms 16 --hop-ms 8 --onset-db 3
  ```
  Segments by file: REC_016 = {9-24, 42-57, 76-91, 110-126}; REC_018 = {5-19, 36-51, 66-80}.

## Parameters
- fs read from WAV header. interp 8, band 125-3500 Hz, d=0.036 m, c=343 m/s.
- FFT length auto-derived (`1<<ceil(log2(2N))`); noise floor / VAD threshold recomputed per frame length.
- VAD: noise floor = 10th percentile of whole-file frame energy; threshold = noise x 10^(6/10).

## Files
- `REC_0{16,18}_*_f32_i8.{csv,png}`, `_f32_i8_on3.{csv,png}` — 32 ms frame.
- `REC_0{16,18}_*_f16_i8.{csv,png}`, `_f16_i8_on3.{csv,png}` — 16 ms frame.
- `frame_trial_report.md` — frame-length comparison writeup.
