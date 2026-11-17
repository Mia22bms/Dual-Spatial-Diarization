# 03_baseline

- Date (file mtime): 2026-11-01 / 2026-11-02 (CSV/PNG); `angle_vs_distance.png` re-plotted 2026-11-16.
- Generating scripts: `single_board_doa.py` (per-segment + whole-file), `run_segments.py` (summaries).
- Recordings: `recordings/REC_015.wav`, `REC_016.WAV`, `REC_018.WAV`
- Segments: `segments/segments_nearfield.txt` (REC_015), `segments/segments_distance.txt` (REC_016),
  `segments/segments_distance2.txt` (REC_018)
- Reproduce (write to `results/scratch/`; files were then moved here), run from `onboard_tdoa/`:
  ```
  # whole-file, interp 1 and 8 (example REC_016; same for REC_015 / REC_018):
  python single_board_doa.py recordings/REC_016.WAV --interp 1
  python single_board_doa.py recordings/REC_016.WAV --interp 8

  # per-segment, interp 1 and 8 (example one REC_016 segment 9-24 s):
  python single_board_doa.py recordings/REC_016.WAV --start 9 --end 24 --interp 1
  python single_board_doa.py recordings/REC_016.WAV --start 9 --end 24 --interp 8

  # per-file segment summaries:
  python run_segments.py --wav recordings/REC_015.wav --seg segments/segments_nearfield.txt
  python run_segments.py --wav recordings/REC_016.WAV --seg segments/segments_distance.txt
  python run_segments.py --wav recordings/REC_018.WAV --seg segments/segments_distance2.txt
  ```
  Segments by file: REC_015 = {3-19, 34-49, 66-81}; REC_016 = {9-24, 42-57, 76-91, 110-126};
  REC_018 = {5-19, 36-51, 66-80}. Whole-file spans: REC_015 0-85, REC_016 0-137, REC_018 0-86.

- `angle_vs_distance.png` reproduce (reads the two summaries in this folder), run from `onboard_tdoa/`:
  ```
  python - <<'PY'
  import csv, os
  import matplotlib; matplotlib.use("Agg")
  import matplotlib.pyplot as plt
  BASE = "results/03_baseline"
  def load_i8(path):
      pts=[]
      for row in csv.DictReader(open(path, newline="")):
          if int(row["interp"])!=8: continue
          pts.append((float(row["label"].rstrip("m")), float(row["angle_median"]),
                      float(row["angle_median"])-float(row["angle_p25"]),
                      float(row["angle_p75"])-float(row["angle_median"])))
      pts.sort()
      return [p[0] for p in pts],[p[1] for p in pts],[p[2] for p in pts],[p[3] for p in pts]
  d16,m16,lo16,hi16=load_i8(os.path.join(BASE,"REC_016_segments_summary.csv"))
  d18,m18,lo18,hi18=load_i8(os.path.join(BASE,"REC_018_segments_summary.csv"))
  # dodge overlapping markers on x only (display): REC_016 -0.015 m, REC_018 +0.015 m
  d16o=[x-0.015 for x in d16]; d18o=[x+0.015 for x in d18]
  fig,ax=plt.subplots(figsize=(8,5))
  ax.axvspan(0,0.5,color="tab:green",alpha=0.12,label="<= 0.5 m (near)")
  ax.axhline(60.0,ls="--",color="gray",lw=1.2,label="ground truth +60 deg")
  ax.errorbar(d16o,m16,yerr=[lo16,hi16],fmt="o",color="tab:blue",capsize=4,label="REC_016 (median, p25-p75)")
  ax.errorbar(d18o,m18,yerr=[lo18,hi18],fmt="s",color="tab:orange",capsize=4,label="REC_018 (median, p25-p75)")
  ax.set_xlabel("distance (m)"); ax.set_ylabel("angle median (deg)")
  ax.set_title("Angle median vs distance (interp 8, no parabolic)")
  ax.set_ylim(-5,90); ax.grid(True,alpha=0.3); ax.legend(fontsize=8); fig.tight_layout()
  fig.savefig(os.path.join(BASE,"angle_vs_distance.png"),dpi=120)
  PY
  ```

## Parameters
- fs read from WAV header. Frame 64 ms, hop 32 ms, band 125-3500 Hz, d=0.036 m, c=343 m/s.
- VAD: noise floor = 10th percentile of whole-file frame energy; threshold = noise x 10^(6/10).
- interp per filename (`_i1` / `_i8`). No parabolic, no onset.
- `angle_vs_distance.png`: interp 8 rows only; dashed line = +60 deg (ground truth);
  shaded region = distance <= 0.5 m. Markers are dodged on the x-axis for readability only
  (REC_016 -0.015 m, REC_018 +0.015 m); medians/percentiles are unchanged.
  Re-plotted 2026-11-16 for the 2026-11-03 meeting.

## Files
- `REC_0{15,16,18}_*_f64_i1.{csv,png}`, `_f64_i8.{csv,png}` — whole-file and per-segment.
- `REC_015_segments_summary.csv`, `REC_016_segments_summary.csv`, `REC_018_segments_summary.csv`.
- `REC_018_report.md` — distance-test writeup.
- `angle_vs_distance.png` — new figure (see command above).
