# Piper

**Piper** is a small utility that **synchronizes** the **PipeWire node volume** with a corresponding **ALSA hardware mixer volume**.

## Problem Statement

EPOS GSX 1000 expects fine-grained volume control via decibel scaling.  
However:
- PipeWire operates in a linear floating-point volume space (0.0 to 1.0),
- ALSA often maps the hardware volume into simple 0–100% steps,
- The fine dB resolution is lost, causing audio artifacts, jumps, or coarse volume changes.

**Piper** solves this by:
- Listening to PipeWire node volume changes,
- Converting the linear volume to real dB using the correct formula,
- Setting the closest matching ALSA mixer control in hardware dB,

## Building

You need:
- PipeWire development libraries (`libpipewire-0.3-dev`)
- ALSA development libraries (`libasound2-dev`)
- CMake (>= 3.10)
- A C99 or newer compiler (GCC or Clang)

**Clone and build:**

```bash
git clone https://github.com/FakeApate/piper.git
cd piper
mkdir build
cd build
cmake ..
make
```

This will produce a `piper` binary in the `build` directory.

## License

This project is licensed under the [MIT License](LICENSE).