# 04_resolution

- Date (file mtime): 2026-11-03
- Generating script: `single_board_doa.py`
- Recordings: `recordings/REC_015.wav`, `REC_016.WAV`, `REC_018.WAV`
- Reproduce (write to `results/scratch/`; files were then moved here), run from `onboard_tdoa/`:
  ```
  # interp 16 (example REC_016 segment 9-24 s):
  python single_board_doa.py recordings/REC_016.WAV --start 9 --end 24 --interp 16
  # interp 8 + parabolic:
  python single_board_doa.py recordings/REC_016.WAV --start 9 --end 24 --interp 8 --parabolic
  ```
  Segments by file: REC_015 = {3-19, 34-49, 66-81}; REC_016 = {9-24, 42-57, 76-91, 110-126};
  REC_018 = {5-19, 36-51, 66-80}.

## Parameters
- fs read from WAV header. Frame 64 ms, hop 32 ms, band 125-3500 Hz, d=0.036 m, c=343 m/s.
- VAD: noise floor = 10th percentile of whole-file frame energy; threshold = noise x 10^(6/10).
- Two resolution variants: `_f64_i16` = interp 16; `_f64_i8_para` = interp 8 with parabolic peak refinement.
- No onset.

## Files
- `REC_0{15,16,18}_*_f64_i16.{csv,png}` — interp 16.
- `REC_0{15,16,18}_*_f64_i8_para.{csv,png}` — interp 8 + parabolic.
