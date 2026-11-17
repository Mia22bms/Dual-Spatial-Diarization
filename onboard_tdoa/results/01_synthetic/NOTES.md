# 01_synthetic

- Date (file mtime): 2026-10-19
- Generating script: `test_synthetic.py`
- Reproduce (writes to `results/scratch/`; files were then moved here), run from `onboard_tdoa/`:
  ```
  python test_synthetic.py
  ```

## Parameters
- Synthetic two-channel signals with known fractional delay (frequency-domain phase shift).
- Sources: white noise, speech-like. fs: 16000 / 48000 Hz. SNR: inf / 10 dB. interp: 1 / 8.
- Fixed-table angles: 0, +30, -30, +60, +90 deg. Frame length 64 ms. SEED = 12345.
- Stats mode: angles 0..90 step 10 deg, noise source only, 200 frames/condition, STATS_SEED = 20260.
- Only `gcc_phat` is exercised (no WAV, no VAD).

## Files
- `synthetic_results.csv` — fixed-table error rows.
- `synthetic_stats.csv` — per-(fs,snr,interp,angle) median/p90 |error|, median confidence.
- `synthetic_stats.png` — interp 8 error curves (16k/48k x clean/10dB).
