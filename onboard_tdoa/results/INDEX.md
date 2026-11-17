# results/ index

Each leaf folder has a `NOTES.md` with date (file mtime), generating script, reproducible command,
parameters, and file list. `scratch/` is the default output directory for new runs (via `--outdir`,
default `results/scratch/`); it is gitignored and intentionally not tracked.

| Folder | Date | Contents |
|---|---|---|
| `01_synthetic/` | 2026-10-19 | Synthetic-signal validation of `gcc_phat`. |
| `02_rec006/` | 2026-10-20 | Board2_REC_006 whole-file + diagnostics + A/C/D overlay. |
| `03_baseline/` | 2026-11-01/02 (fig 11-16) | REC_015/016/018 baseline f64 interp 1/8 + summaries + distance figure. |
| `04_resolution/` | 2026-11-03 | interp 16 and interp 8 + parabolic resolution variants. |
| `05_onset/a_prev2_1103/` | 2026-11-03 | Onset trial, prev2 rule (onset_db 3/6/10). |
| `05_onset/b_frame_length_1103/` | 2026-11-03 | Frame-length trial (32/16 ms), optional onset prev2. |
| `05_onset/c_mean_1116/` | 2026-11-16 | Onset experiment, mean rule; avg normalized GCC-PhaT curves. |
| `06_angle_dist/` | 2026-11-16 | Per-frame angle distributions (avg + histogram) and per-frame CSVs. |
| `scratch/` | — | Default output dir for new runs (gitignored, untracked). |

## Meeting mapping

| Meeting | Folders |
| 10/20 | 01_synthetic, 02_rec006 |
| 11/3 | 03_baseline |
| 11/17 | 04_resolution, 05_onset, 06_angle_dist |
