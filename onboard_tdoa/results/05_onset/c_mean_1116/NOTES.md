# 05_onset / c_mean_1116

- Date (file mtime): 2026-11-16
- Generating script: `run_onset_experiment.py`
- Recordings: `recordings/REC_016.WAV`, `REC_018.WAV`;
  segments: `segments/segments_distance.txt`, `segments/segments_distance2.txt`
- Onset rule (this folder): `--onset-ref mean` =
  keep voiced frame i if `energy[i] >= mean(energy[i-lookback:i]) * 10^(onset_db/10)`,
  onset_db 6 dB, lookback 3. The script pins `onset_ref="mean"`.
- Reproduce (writes PNGs to `results/scratch/`; files were then moved here), run from `onboard_tdoa/`:
  ```
  python run_onset_experiment.py
  ```

## Parameters
- fs read from WAV header. Frame 64 ms / 16 ms (hop = frame/2), interp 8, parabolic peak refinement.
- Settings compared in the printed table: (a) all voiced f64, (b) onset f64, (c) onset f16.
- VAD: noise floor = 10th percentile of whole-file frame energy; threshold = noise x 10^(6/10).
- The two `_avgcurve.png` are averages of the per-frame normalized GCC-PhaT curve over ALL voiced
  frames (frame 64 ms, interp 8); they do not depend on the onset rule.

## Files
- `REC_016_9-24_avgcurve.png` — 0.3 m segment, average normalized GCC-PhaT vs angle.
- `REC_016_76-91_avgcurve.png` — 1 m segment, same.
- `onset_experiment_report.md` — writeup; its onset table (settings b/c) uses the mean rule above.
