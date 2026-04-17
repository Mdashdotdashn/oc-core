# Cross-Platform Build Status

This document captures the current state of cross-platform build support and
what remains to be done. It is intended as a handoff note so a new session on
any machine can pick up the work without losing context.

---

## Current State (as of April 2026)

### What works (verified on Linux)

- Full compile and flash via PlatformIO CLI (`pio run`, `pio run -t upload`).
- Top-level `Makefile` convenience wrapper for `make <example>`, `make flash`,
  `make clean`.
- All six examples build cleanly: `lfo`, `display_test`, `encoder_test`,
  `cpu_meter`, `trigger_test`, `adc_calibration`.
- Hardware validated on Teensy 3.2: ADC, DAC, GPIO, encoders, buttons,
  SH1106 OLED, trigger inputs.
- Repo pushed to `https://github.com/Mdashdotdashn/oc-core` on `master`.

---

## Cross-Platform Readiness

### macOS

**Likely works without changes.**

- PlatformIO CLI is available via `brew install platformio` or
  `pipx install platformio`.
- Teensy CLI upload tool is bundled with the PlatformIO Teensy platform.
- The Makefile uses POSIX shell constructs that work on macOS out of the box.
- The `uname` branch fires correctly (not the Windows `rd /s /q` branch).
- Recommended first step: install PlatformIO and run `pio run -d examples/lfo`.

### Windows (Git Bash)

**Compile: verified. Flash: pending hardware verification.**

**What works:**

- PlatformIO resolves all include paths from `platformio.ini` internally, so
  the `../../include` relative paths are fine even on Windows.
- Git Bash provides a POSIX `make` (from MSYS2/mingw) that can run the
  Makefile shell loops.
- `python3 -m platformio run -d examples/lfo` compiles successfully on Windows.

**Makefile fix applied:**

The Makefile used `rd /s /q` and `where` in the Windows branch, which are
`cmd.exe` tools and do not work in Git Bash.

Fixed by switching the Windows branch to Git Bash-compatible tools:
`rm -rf`, `/dev/null`, and `command -v`.

```makefile
ifeq ($(OS),Windows_NT)
  DETECTED_OS := Windows
  RM_RF       := rm -rf
  NULL        := /dev/null
  HAVE_CMD     = command -v $(1) >$(NULL) 2>&1
else
  ...
endif
```

**Flash on Windows:**

`upload_protocol = teensy-cli` in `platformio.ini` relies on the PlatformIO
Teensy platform bundling `teensy-cli`. This usually works, but depends on
whether the PlatformIO package manager picks up the right tool on Windows.
If flashing fails, the fallback is to open Teensy Loader manually and drag
the `.hex` from `examples/<name>/.pio/build/teensy32/firmware.hex`.

**`PIO` path:**

On Linux the Makefile is often called with `make flash PIO=~/.local/bin/pio`
because PlatformIO is installed under `~/.local`. On Windows/macOS it will
typically be on `PATH` after installation, so plain `make flash` should work.

---

## What Still Needs To Be Done for Full Cross-Platform Support

Priority order:

1. **Verify Git Bash `make` wrapper end-to-end on Windows.**
  - Compile was verified with direct PlatformIO invocation.
  - Need to validate `make <example>` / `make clean` specifically in Git Bash
    session (not PowerShell).

2. **Add platform-specific install instructions to `docs/GETTING_STARTED.md`.**
   Currently only covers Linux. Should add:
   - macOS: `brew install platformio` or `pipx install platformio`
   - Windows: install PlatformIO VS Code extension or `pip install platformio`
     under a Python that Git Bash can see; add `pio` to PATH.

3. **Verify a real macOS compile+flash.** The Makefile and PlatformIO config
   look correct but have not been tested on hardware. First flash on macOS
   will require pressing the Teensy reset button because soft-reboot (`-s`
   flag) requires a running oc-core firmware.

4. **Verify a real Windows flash with hardware connected.**
  - Compile is already verified.
  - Upload (`pio run -t upload`) must still be tested on connected Teensy.

5. **Consider dropping the arduino-cli fallback** or explicitly marking it
   "untested/unsupported". It adds Makefile complexity and is not validated
   on any platform.

---

## Session Context for a New Copilot Session

If you are starting a new Copilot session on a different machine and want to
continue this work, paste the following prompt to restore context quickly:

> "I am working on the oc-core repo at https://github.com/Mdashdotdashn/oc-core.
> It is a Teensy 3.2 / Ornament & Crime style CV/gate framework in C++17,
> built with PlatformIO. The `master` branch is current. Read
> `docs/CROSS_PLATFORM.md`, `docs/ARCHITECTURE.md`, and `docs/PROJECT_NOTES.md`
> to understand the project state, then help me with cross-platform build
> support (or whatever the next task is)."

The four docs most useful for context:
- `docs/CROSS_PLATFORM.md` — this file; cross-platform status and pending work
- `docs/ARCHITECTURE.md`   — runtime, ISR model, display pipeline, app API
- `docs/PROJECT_NOTES.md`  — calibration design decisions and history
- `docs/GETTING_STARTED.md` — quick-start build and app shape

---

## Key Technical Facts for a New Session

- Board: Teensy 3.1/3.2 (MK20DX256), 72 MHz, 64 KB RAM, 256 KB Flash
- PlatformIO board id: `teensy31`
- Build standard: `-std=gnu++17`
- ISR rate: 10 kHz (`runtime.start(100)` → 100 µs period)
- Shared SPI0 bus: DAC8565 + SH1106 128×64 OLED
- Display model: paged DMA, one page per ISR, 8 pages = full frame
- Calibration: unified persisted blob, boot-time load, DAC-first O_C-style
  wizard planned (first app `examples/adc_calibration` already in place)
- Upload protocol: `teensy-cli` with `-s` for soft-reboot (first flash needs
  manual reset button press)
