# Project Context for Claude Code

## What this project is
Audio data collection firmware for the Joel Walsh research group at Occidental College.
Research target: **cocktail party problem** — separating overlapping voices in noisy
indoor environments (multi-speaker dialogue, family meals, daily life). Data collected
this summer will be used in fall semester for diarization/separation experiments.

I (Mia Shen) am a first-year CS undergrad and research assistant. I'm in China for the
summer; Joel and other teammates are in the US (~13-15hr async).

## Hardware
- **2x Waveshare ESP32-S3-AUDIO-Board** ("pucks") — placed at different spatial
  positions in the room, recording in parallel
- ES7210 4-channel mic ADC + ES8311 codec, 2 physical mics per board
- Lexar 32GB microSD per board (FAT32)
- 3.7V Li battery + power bank for portable recording

## Toolchain
- ESP-IDF **v5.5.4** (Windows, offline installer at `C:\Espressif`)
- VSCode for editing; **separate CMD window** for all `idf.py` commands
- Every new terminal session: run `C:\Espressif\frameworks\esp-idf-v5.5.4\export.bat`
- Board ports: COM3 (board 1), COM4 (board 2)
- Audacity for waveform verification

## Current status (as of 2026-06-16)
- ✅ Phase 1: hello_world flashed and verified
- ✅ Phase 2: 4ch × 32-bit raw diagnostic recording verified on **both boards**
  - Audacity confirms all 4 channels (RMNM layout) contain real audio
  - Clap test: spike aligned across channels on each board
- ⏳ Phase 3: in progress — final recording firmware

## Phase 3 goals (current focus)
Write production recording firmware with:
- **4 channels × 16-bit × 16kHz, WAV format** (Joel confirmed these defaults 6/13)
- WAV header `data_size` backfill **per second** via `fseek(SEEK_SET) + fflush + fsync`
  for power-cut recovery
- `scan_existing_clips()` on boot to recover clip index from SD card (RAM is volatile)
- Per-session metadata: board x,y position + orientation in the room
- **Sync strategy: clap sync** — clap once at start of each recording, post-process
  with cross-correlation to align two boards. No GPIO/ESP-NOW yet (cheapest option,
  clap-aligned data remains usable if we upgrade later).

## Known gotchas (do not re-discover these)
- **WAV header channel count MUST match actual data** — mismatched header (e.g. declared
  2ch on 4ch data) causes byte-alignment errors, manifesting as Audacity silence with
  normal file size
- **ES7210 outputs 32-bit samples with audio in upper 16 bits** — for 16-bit output,
  right-shift by 16; for raw inspection in Audacity, amplify by ~27 dB
- **`esp_sdcard_init("/sdcard", 5)` must be called separately** — `esp_board_init`
  does NOT mount the SD card
- **Waveshare BSP path has typo**: `main\hardeware_driver\` (not "hardware")
- **Two boards have no shared clock** — they run independently, sync done in post-processing
- **Don't try to run training in Claude Code** — token-heavy. Smoke tests OK.

## Working style — read this carefully

**I'm explicitly avoiding "vibecoding."** I want to genuinely understand every decision,
because I need to be able to defend the implementation to Joel and explain it in any
future paper.

This means:
1. **Before changing code, explain your plan first.** Wait for my OK before editing.
2. **For hardware parameter choices** (sample rate, bit depth, buffer size, channel
   layout, sync strategy, etc.) — surface the trade-off, give me options, **do not pick
   a default and move on.** These are research decisions, not implementation details.
3. **Don't write decision documentation for me.** I keep a DECISIONS log per phase
   ("why I chose X over Y"); I write that myself — it's how I verify my own understanding.
   You handle code + code comments.
4. **When I ask "why did you do that?", give the real reason, not a post-hoc rationalization.**
   If the real answer is "it was the most common pattern I saw," say so.

## Communication preferences
- Default language: **Chinese** (English fine if I switch)
- I'll typically work in short iterations; concise responses preferred
- When you complete a session, give me a 3-5 line summary: what changed,
  what was decided, what's still open

## Joel's involvement
Joel delegates all hardware/firmware decisions to me. He cares about:
- Research scenario (noisy overlapping indoor dialogue)
- Final recording quality
- Metadata (board positions per session)

He does NOT want to be asked about:
- Sample rate / bit depth / buffer sizes
- Sync method
- Firmware structure / build process

I only contact Joel when there's real progress (first good sample) or a real blocker.