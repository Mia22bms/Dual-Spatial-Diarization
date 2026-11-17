# 05_onset / a_prev2_1103

- Date (file mtime): 2026-11-03 (CSV/PNG + report); `REC_018_segments_summary_on3.csv` 2026-11-16.
- Generating scripts: `single_board_doa.py` (per-segment onset CSV/PNG), `run_segments.py` (summary).
- Recordings: `recordings/REC_016.WAV`, `REC_018.WAV`
- Onset rule (this folder): `--onset-ref prev2` (default) =
  keep voiced frame i if `energy[i] >= energy[i-2] * 10^(onset_db/10)`.
- Reproduce (write to `results/scratch/`; files were then moved here), run from `onboard_tdoa/`:
  ```
  # per-segment, onset_db in {3,6,10} (example REC_016 segment 9-24 s):
  python single_board_doa.py recordings/REC_016.WAV --start 9 --end 24 --interp 8 --onset-db 3
  python single_board_doa.py recordings/REC_016.WAV --start 9 --end 24 --interp 8 --onset-db 6
  python single_board_doa.py recordings/REC_016.WAV --start 9 --end 24 --interp 8 --onset-db 10
  # summary (onset_db 3, prev2 default):
  python run_segments.py --wav recordings/REC_018.WAV --seg segments/segments_distance2.txt --onset-db 3
  ```
  Segments by file: REC_016 = {9-24, 42-57, 76-91, 110-126}; REC_018 = {5-19, 36-51, 66-80}.

## Parameters
- fs read from WAV header. Frame 64 ms, hop 32 ms, interp 8, band 125-3500 Hz, d=0.036 m, c=343 m/s.
- VAD: noise floor = 10th percentile of whole-file frame energy; threshold = noise x 10^(6/10).
- Frame energy = (MIC1.MIC1 + MIC2.MIC2)/(2N) = mean-square power; onset multiplier = 10^(onset_db/10).
- Statistics (median/percentiles) use kept (onset==1) frames only.

## Files
- `REC_0{16,18}_*_f64_i8_on{3,6,10}.{csv,png}` — per-segment onset-kept analysis.
- `REC_018_segments_summary_on3.csv` — summary with onset_db 3 (regenerated 2026-11-16 during the
  prev2/mean verification; content matches the 2026-11-03 prev2 rule).
- `onset_trial_report.md` — writeup of the prev2 onset trial.
