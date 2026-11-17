# Dual-Spatial-Diarization

Spatial speaker diarization for multi-speaker conversations using low-cost ESP32-S3 microphone boards ("pucks"). Each board estimates speaker direction from its own two microphones; a computer combines bearings from two or more boards to localize speakers and separate who spoke when.

Undergraduate research at Occidental College.

## Pipeline

| Layer | What | Runs on | Status |
|---|---|---|---|
| 0 | Single-board direction of arrival (GCC-PHAT TDOA) | board | Partial: reliable within ~0.5 m; biased toward 0° beyond that (Python prototype) |
| 1 | Per-frame output: time / angle / confidence | board | Python per-frame CSV works; C port planned |
| 2 | Time alignment across boards | computer | planned |
| 3 | Intersect bearings → speaker (x, y) and distance to each board | computer | planned |
| 4 | Cluster speakers → 3D scene view | computer | planned |
| 5 | Beamforming + ASR | computer | separate subgroup |

A single board can only measure angle, not distance; distance comes from intersecting bearings from two boards.

## Repository layout

- `ESP32-S3-AUDIO-Board-Demo/ESP-IDF/mp3_play_03/`: active firmware (4-channel WAV recording to microSD, button start/stop, status LED)
- `ESP32-S3-AUDIO-Board-Demo/ESP-IDF/mp3_play_03_summer2026/`: frozen snapshot of the summer 2026 firmware (reference only, not built)
- `onboard_tdoa/`: Python prototype of the single-board direction estimator, to be ported to C
  - `*.py`: the estimator and experiment scripts (see Usage)
  - `segments/`: segment lists (`label start_s end_s`) for each recording
  - `recordings/`: WAV files (local only, not in this repo; see Data)
  - `results/`: one folder per research question, in the order we asked them. `results/INDEX.md` lists every folder and which meeting it was presented at; each folder's `NOTES.md` gives the date, parameters, and the exact command that reproduces it
  - `TDOA_SPEC.md`: algorithm spec the prototype follows
- `Hello_World/`: ESP-IDF toolchain sanity check

| Results folder | Question |
|---|---|
| `01_synthetic/` | Does the estimator work on synthetic signals? |
| `02_rec006/` | Does it work on a first real conversation recording? |
| `03_baseline/` | Near-field test and distance sweep: where does it work? |
| `04_resolution/` | Is the far-range error a resolution (tick size) problem? |
| `05_onset/` | Is it reverberation? (onset-frame selection) |
| `06_angle_dist/` | Can we output a per-frame probability distribution over angle? |

## Hardware

- 2 × Waveshare ESP32-S3-AUDIO-Board (ES7210 4-channel ADC)
- Recording format: 4 ch × 16-bit × 16 kHz WAV
- Mic mapping: track 2 = MIC1, track 4 = MIC2 (36 mm apart, hand-measured; not published by the manufacturer); tracks 1 and 3 have no microphone
- Angle convention: 0° = perpendicular to the MIC1–MIC2 line, pointing outward; positive toward MIC1. τ = arrival time at MIC2 − arrival time at MIC1

## onboard_tdoa: current status (Nov 2026)

- **Synthetic test** (`01`): passes (sign convention, τ→angle, interpolation, confidence). With 10 dB noise, 48 kHz is not clearly better than 16 kHz (8× interpolation).
- **REC_006** (`02`; dining room, speakers 1.5–2.5 m away): all speakers read ≈4°. Fails.
- **Near-field test** (`03`; REC_015, phone at 30 cm): +60° / 0° / −60° read +48° / −4° / −55° (τ = +78 / −8 / −86 µs). Signs correct, positions separated; crosstalk ruled out.
- **Distance sweep** (`03`; REC_016, REC_018, +60°, small study room): ≈48° up to 0.5 m; 31° at 0.7 m, 27° at 1 m, 17° at 1.5 m. Reproducible across two recordings. High-confidence frames are biased too.
- **Resolution ruled out** (`04`): finer interpolation (16×) and parabolic peak refinement move the 1 m reading by ≤1° (26.5° → 26.9–27.5°). The error is bias, not resolution.
- **Onset frames** (`05`): using only frames where energy rises sharply (direct sound arrives before reflections) moves far readings toward the true angle, e.g. 1 m: ≈27° → 33–37°, in both recordings and with two onset rules. Supports the reverberation hypothesis, but recovers only about a third of the gap.
- **Angle distribution** (`06`): treating the GCC-PHAT curve as a distribution gives one broad peak in every frame (no second, reflection peak in ~3,100 frames). The curve is about as wide as the whole physical delay range for a 36 mm pair, so its width reflects resolution, not uncertainty; the spread of frame readings is a better uncertainty measure. Neither shows the bias.
- **Working hypothesis:** reverberation (supported, not confirmed). Next: PHAT-β weighting, onset selection combined with other fixes, retest in a different room.

## Usage

All scripts are in `onboard_tdoa/` and can be run from any folder. Requires `numpy`, and `matplotlib` for scripts that write PNGs. New outputs go to `onboard_tdoa/results/scratch/` by default (gitignored); use `--outdir` to write elsewhere.

Recordings are not included in this repo. Place WAV files in `onboard_tdoa/recordings/` locally. `test_synthetic.py` needs no recording and is the easiest place to start.

Examples below are run from inside `onboard_tdoa/`.

### Core tools

- **`single_board_doa.py`**: full single-board direction estimate on one WAV file. Writes a per-frame CSV (`time_s, tau_us, angle_deg, confidence, voiced`, plus `onset` when onset selection is on) and an angle-vs-time PNG, and prints a summary.
```
python single_board_doa.py <wav> [--start SEC] [--end SEC] [--interp 8] [--parabolic]
    [--frame-ms 64] [--hop-ms 32] [--vad-db 6] [--noise-start SEC --noise-end SEC]
    [--onset-db DB] [--onset-ref prev2|mean] [--onset-lookback 3] [--outdir DIR]
```
  - `--parabolic`: refine the correlation peak with a parabola through the peak and its two neighbors (file suffix `_para`)
  - `--onset-db`: keep only onset frames. `prev2` (default): frame energy ≥ energy two frames earlier × 10^(dB/10). `mean`: frame energy ≥ mean of the previous `--onset-lookback` frames × 10^(dB/10) (file suffix `_mean`)

- **`run_segments.py`**: runs `single_board_doa` on each segment of a recording at interp 1 and 8, and writes `<wav>_segments_summary.csv` (frames, angle median, p25, p75, confidence median). Accepts the same `--onset-db / --onset-ref / --onset-lookback` options.
```
python run_segments.py --wav recordings/REC_016.WAV --seg segments/segments_distance.txt
```

  | Recording | Segment file | Test |
  |---|---|---|
  | `Board2_REC_006.wav` | `segments/segments.txt` | REC_006 conversation (speakers A/C/D) |
  | `REC_015.wav` | `segments/segments_nearfield.txt` | near-field, +60° / 0° / −60° at 30 cm |
  | `REC_016.WAV` | `segments/segments_distance.txt` | +60°, 0.3 / 0.5 / 1 / 1.5 m |
  | `REC_018.WAV` | `segments/segments_distance2.txt` | +60°, 0.3 / 0.7 / 1 m (repeat) |

- **`test_synthetic.py`**: generates synthetic two-mic signals with known delays, runs the estimator, and checks sign and error against pass criteria. No recording needed.
```
python test_synthetic.py
```

- **`wav_io.py`**: WAV reader module (16/24-bit, reads fs from the header). Run directly, it prints a header summary for the given file.
```
python wav_io.py recordings/REC_015.wav
```

- `gcc_phat.py`: imported module only (GCC-PHAT for one frame).

### Experiment scripts

Each reproduces one results folder; see that folder's `NOTES.md` for details.

- **`run_onset_experiment.py`** (`05_onset/c_mean_1116`): onset selection with the `mean` rule on REC_016/018, plus averaged GCC-PHAT curves.
- **`run_angle_dist.py`** (`06_angle_dist`): per-frame angle distributions from the GCC-PHAT curve, with frame-reading histograms.
- **`diag_rec006.py`** (`02_rec006`): REC_006 diagnostics (all frames, energy tiers, speaker loudness).
- **`diag_expected_overlay.py`** (`02_rec006`): overlays expected speaker angles on the committed REC_006 per-frame CSV. Does not need the WAV.

All four take `--outdir`.

## Firmware build

ESP-IDF v5.5.4: run `idf.py build flash monitor` from `ESP32-S3-AUDIO-Board-Demo/ESP-IDF/mp3_play_03/` (on Windows, run ESP-IDF's `export.bat` first).

## Data

Audio recordings are not stored in this repository (file size, and they contain real people's voices).

## License

MIT
