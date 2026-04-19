# Makefile for oc-core framework examples
# Builds Teensy 3.2 firmware via PlatformIO (preferred) or arduino-cli fallback.
#
# Prerequisites:
#   PlatformIO CLI  https://platformio.org/install/cli
#     pipx install platformio
#   teensy_loader_cli (for stand-alone flash target)
#     sudo apt-get install teensy-loader-cli

# ---- OS detection -----------------------------------------------------------
ifeq ($(OS),Windows_NT)
  DETECTED_OS := Windows
  RM_RF       := rm -rf
  NULL        := /dev/null
  HAVE_CMD     = command -v $(1) >$(NULL) 2>&1
  # On Windows, pio.exe is installed in the Python Scripts dir which may not
  # be on PATH in Git Bash.  The Windows Python Launcher (py.exe) lives at
  # C:\Windows\py.exe and is always on PATH in every shell (Git Bash,
  # PowerShell, cmd.exe), so it is the most reliable invocation.
  PIO_DEFAULT  := py -3 -m platformio
else
  DETECTED_OS := $(shell uname -s)
  RM_RF       := rm -rf
  NULL        := /dev/null
  HAVE_CMD     = command -v $(1) >$(NULL) 2>&1
  PIO_DEFAULT  := pio
endif
# -----------------------------------------------------------------------------

PIO           ?= $(PIO_DEFAULT)
ARDUINO_CLI   ?= arduino-cli
TEENSY_LOADER := teensy_loader_cli
FQBN          ?= teensy:avr:teensy31:usb=serialmidi,speed=72,opt=osstd,keys=en-us

# Auto-discover examples under examples/oc/ and examples/tu/
EXAMPLE_DIRS  := $(wildcard examples/oc/*/platformio.ini) $(wildcard examples/tu/*/platformio.ini)
EXAMPLES      := $(patsubst examples/%/platformio.ini,%,$(EXAMPLE_DIRS))

# Default example for flash / upload (use oc/ or tu/ prefix, e.g. oc/lfo, tu/clock_test)
EXAMPLE ?= oc/lfo

# ============================================================================
# Targets
# ============================================================================

.PHONY: all $(EXAMPLES) flash upload clean help check-pio check-cli auto-build pio-build cli-build

all: auto-build

auto-build:
	@if $(PIO) --version >$(NULL) 2>&1; then \
	  echo "Using PlatformIO..."; \
	  $(MAKE) pio-build; \
	else \
	  echo "PlatformIO not found, falling back to arduino-cli..."; \
	  $(MAKE) cli-build; \
	fi

# Build every discovered example via PlatformIO
pio-build: check-pio
	@for ex in $(EXAMPLES); do \
	  echo ">>> Building example: $$ex"; \
	  $(PIO) run -d examples/$$ex || exit 1; \
	done

# Build every discovered example via arduino-cli
cli-build: check-cli
	@for ex in $(EXAMPLES); do \
	  echo ">>> Building example: $$ex"; \
	  $(ARDUINO_CLI) compile --fqbn $(FQBN) examples/$$ex || exit 1; \
	done

# Per-example shorthand: e.g. `make lfo`
$(EXAMPLES): check-pio
	@echo "Building example: $@"
	$(PIO) run -d examples/$@

# Flash via PlatformIO (uses upload_protocol from platformio.ini)
flash: check-pio
	@echo "Flashing example: $(EXAMPLE)"
	$(PIO) run -d examples/$(EXAMPLE) -t upload

upload: flash

clean:
	@echo "Cleaning all example build artifacts..."
	@for ex in $(EXAMPLES); do \
	  echo "  $(RM_RF) examples/$$ex/.pio"; \
	  $(RM_RF) examples/$$ex/.pio 2>$(NULL) || true; \
	done

# ============================================================================
# Prerequisite checks
# ============================================================================

check-pio:
	@$(PIO) --version >$(NULL) 2>&1 || \
	  (echo "ERROR: '$(PIO)' not found or not working" && \
	   echo "Install PlatformIO: pipx install platformio" && \
	   exit 1)

check-cli:
	@$(call HAVE_CMD,$(ARDUINO_CLI)) || \
	  (echo "ERROR: $(ARDUINO_CLI) not found in PATH" && \
	   echo "Install arduino-cli: https://arduino.github.io/arduino-cli/" && \
	   exit 1)

# ============================================================================
# Help
# ============================================================================

help:
	@echo "oc-core Example Build System"
	@echo "============================"
	@echo ""
	@echo "Discovered examples: $(EXAMPLES)"
	@echo ""
	@echo "Targets:"
	@echo "  make                    Build all examples (auto: pio > arduino-cli)"
	@echo "  make pio-build          Build all examples via PlatformIO"
	@echo "  make cli-build          Build all examples via arduino-cli"
	@echo "  make lfo                Build only the lfo example"
	@echo "  make flash              Flash EXAMPLE to Teensy (default: $(EXAMPLE))"
	@echo "  make flash EXAMPLE=lfo  Flash a specific example"
	@echo "  make clean              Remove all .pio build artifacts"
	@echo "  make help               Show this message"
	@echo ""
	@echo "Prerequisites:"
	@echo "  pipx install platformio"
	@echo "  # Teensy AVR platform (first time only):"
	@echo "  pio pkg install --global -p platformio/platform-teensy"
