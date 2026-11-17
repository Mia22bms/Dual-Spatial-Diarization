# 02_rec006

- Date (file mtime): 2026-10-20
- Generating scripts: `single_board_doa.py`, `diag_rec006.py`, `diag_expected_overlay.py`, `run_segments.py`
- Recording: `recordings/Board2_REC_006.wav`; segments: `segments/segments.txt`
- Reproduce (write to `results/scratch/`; files were then moved here), run from `onboard_tdoa/`:
  ```
  python single_board_doa.py recordings/Board2_REC_006.wav --interp 8
  python diag_rec006.py
  python diag_expected_overlay.py
  python run_segments.py
  ```
  Note: `diag_expected_overlay.py` reads `results/02_rec006/Board2_REC_006_0-247_f64_i8.csv`
  and `segments/segments.txt`.

## Parameters
- fs read from WAV header. Frame 64 ms, hop 32 ms, interp 8, band 125-3500 Hz, d=0.036 m, c=343 m/s.
- VAD: noise floor = 10th percentile of whole-file frame energy; threshold = noise x 10^(6/10).
- `diag_rec006.py`: all frames (no VAD), energy tiers (5 quantiles), per-speaker loudness halves.

## Files
- `Board2_REC_006_0-247_f64_i8.csv`, `.png` — whole-file analysis, interp 8.
- `diag_allframes.csv`, `diag_energy_tiers.csv`, `diag_speaker_loudness.csv` — from `diag_rec006.py`.
- `diag_expected_overlay.png` — A/C/D voiced frames vs expected angles (+42/+32/+8 deg).
- `segments_summary.csv` — per-segment medians/percentiles (interp 1 and 8) from `run_segments.py`.
