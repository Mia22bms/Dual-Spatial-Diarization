# 06_angle_dist

- Date (file mtime): 2026-11-16
- Generating script: `run_angle_dist.py`
- Recordings: `recordings/REC_015.wav`, `REC_016.WAV`, `REC_018.WAV`;
  segments: `segments/segments_nearfield.txt`, `segments/segments_distance.txt`,
  `segments/segments_distance2.txt`
- Reproduce (writes to `results/scratch/`; files were then moved here), run from `onboard_tdoa/`:
  ```
  python run_angle_dist.py
  ```
  Segments by file: REC_015 = {3-19, 34-49, 66-81}; REC_016 = {9-24, 42-57, 76-91, 110-126};
  REC_018 = {5-19, 36-51, 66-80}.

## Parameters
- fs read from WAV header. Frame 64 ms, hop 32 ms, interp 8, band 125-3500 Hz, d=0.036 m, c=343 m/s.
- VAD: noise floor = 10th percentile of whole-file frame energy; threshold = noise x 10^(6/10).
- Per voiced frame: GCC-PhaT curve within +/- d/c, negatives set to 0, normalized to sum 1, each
  delay point mapped to angle = arcsin(c*tau/d). Angle grid = 27 delay points (-75.4..+75.4 deg).
- Overlay histogram = per-frame interp8+parabolic angle reading, 2 deg bins, density.
- Peak = argmax of average distribution; 50% mass range = central 25-75 percentile of the PMF;
  local peaks counted if strictly above neighbors and > 30% of the max.

## Files
- `REC_0{15,16,18}_*_angdist.csv` — one row per voiced frame, one column per angle (header = angles).
- `REC_0{15,16,18}_*_angdist.png` — average distribution (left axis) + reading histogram (right axis).
