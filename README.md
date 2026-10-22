# Dual-Spatial-Diarization

Spatial speaker diarization for multi-speaker conversations using low-cost ESP32-S3 microphone boards ("pucks"). Each board estimates speaker direction from its own two microphones; a computer combines bearings from two or more boards to localize speakers and separate who spoke when.

Undergraduate research at Occidental College.

## Pipeline

| Layer | What | Runs on | Status |
|---|---|---|---|
| 0 | Single-board direction of arrival (GCC-PHAT TDOA) | board | in progress (Python prototype) |
| 1 | Per-frame output: time / angle / confidence | board | planned |
| 2 | Time alignment across boards | computer | planned |
| 3 | Intersect bearings → speaker (x, y) and distance to each board | computer | planned |
| 4 | Cluster speakers → 3D scene view | computer | planned |
| 5 | Beamforming + ASR | computer | separate subgroup |

A single board can only measure angle, not distance; distance comes from intersecting bearings from two boards.

## Repository layout

- `ESP32-S3-AUDIO-Board-Demo/ESP-IDF/mp3_play_03/` — active firmware: 4-channel WAV recording to microSD, button start/stop, status LED
- `ESP32-S3-AUDIO-Board-Demo/ESP-IDF/mp3_play_03_summer2026/` — frozen snapshot of the summer 2026 firmware (reference only, not built)
- `onboard_tdoa/` — Python prototype of the single-board direction estimator, to be ported to C
- `Hello_World/` — ESP-IDF toolchain sanity check

## Hardware

- 2 × Waveshare ESP32-S3-AUDIO-Board (ES7210 4-channel ADC)
- Recording format: 4 ch × 16-bit × 16 kHz WAV
- Mic mapping: track 2 = MIC1, track 4 = MIC2 (36 mm apart, hand-measured); tracks 1 and 3 have no microphone
- Angle convention: 0° = perpendicular to the MIC1–MIC2 line, pointing outward; positive toward MIC1. τ = arrival time at MIC2 − arrival time at MIC1

## onboard_tdoa: current status (Oct 2026)

- Synthetic tests: sign convention correct; ~1.4° error at 30° (16 kHz, 8× interpolation). Error grows near ±90°, so speakers should stay within roughly ±60° of each board (to be verified).
- First real recording (REC_006): pipeline runs end to end, but estimates cluster near 4° regardless of true angle (8°–42°). Background noise is largely ruled out; reverberation vs. hardware crosstalk is under investigation via near-field tests.

### Usage

Run all scripts from inside `onboard_tdoa/` (they use relative paths). Requires `numpy`, and `matplotlib` for the scripts that write PNGs.

`test_synthetic.py` needs no recording and is the easiest place to start. Recordings are not included in this repo: scripts that read `Board2_REC_006.wav` need that file placed in `onboard_tdoa/` locally.

#### General tools

- **`single_board_doa.py`** — Runs the full single-board direction estimate on one WAV file; writes a per-frame CSV (`time_s, tau_us, angle_deg, confidence, voiced`) and an angle-vs-time PNG to `--outdir`, and prints a summary.
```
  python single_board_doa.py <wav> [--start SEC] [--end SEC] [--interp 8] [--frame-ms 64] [--hop-ms 32] [--vad-db 6] [--noise-start SEC --noise-end SEC] [--outdir .]
```

- **`test_synthetic.py`** — Generates synthetic two-mic signals with known delays, runs the estimator, and checks sign/error against pass criteria. Writes `synthetic_results.csv`, `synthetic_stats.csv`, and `synthetic_stats.png`, and prints tables. No arguments; no recording needed.
```
  python test_synthetic.py
```

- **`wav_io.py`** — Primarily an imported helper module (WAV reading); run directly it prints a header summary (fs, bit depth, channel count, duration, per-track peak) for the given WAV, defaulting to `Board2_REC_006.wav`.
```
  python wav_io.py [wav]
```

(`gcc_phat.py` is an imported module only and is not run directly.)

#### REC_006 analysis (one-off, hardcoded paths)

- **`run_segments.py`** — Reads `segments.txt`, runs `single_board_doa` on each segment at interp 1 and 8, and writes/prints `segments_summary.csv`. Needs `Board2_REC_006.wav`.
```
  python run_segments.py
```

- **`diag_rec006.py`** — Read-only diagnostic: writes `diag_allframes.csv`, `diag_energy_tiers.csv`, and `diag_speaker_loudness.csv` and prints a summary. Needs `Board2_REC_006.wav`.
```
  python diag_rec006.py
```

- **`diag_expected_overlay.py`** — Reads the committed `Board2_REC_006_0-247_f64_i8.csv` and `segments.txt` (does not re-run analysis) and writes `diag_expected_overlay.png`. Does not need the WAV.
```
  python diag_expected_overlay.py
```

## Firmware build

ESP-IDF v5.5.4: run `idf.py build flash monitor` from `ESP32-S3-AUDIO-Board-Demo/ESP-IDF/mp3_play_03/` (on Windows, run ESP-IDF's `export.bat` first).

## Data

Audio recordings are not stored in this repository (file size, and they contain real people's voices).

## License

MIT