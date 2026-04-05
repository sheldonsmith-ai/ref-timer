# Referee Timer

A dual-timer application for Pebble smartwatches designed for sports referees and officials. Features a play clock and game clock with persistent state tracking.

## Features

- **Play Clock**: 25-second countdown timer with visual and haptic feedback
  - Custom 7-segment display that scales to any screen size
  - Haptic alerts at 10 seconds (2 buzzes) and 1-5 seconds (short pulses)
  - Auto-reset after expiration

- **Game Clock**: Configurable game timer (1-99 minutes)
  - Adjustable from 1 second to 99:59
  - Shows time in MM:SS format using JetBrains Mono font
  - Haptic alert when 2 minutes remaining
  - Auto-resets to default when expired

- **Persistent Storage**: All settings and state are saved
  - Game clock time and running state persist across app restarts
  - Play clock state is preserved
  - Default game time is customizable

- **Multi-Platform Support**: Works on all Pebble devices
  - Round and rectangular displays
  - Adaptive UI based on screen size

## Controls

### Normal Mode

| Button | Action |
|--------|--------|
| **Up** | Start/stop/reset play clock |
| **Down** | Start/stop game clock |
| **Select** | Exit edit mode (when in edit mode) |
| **Back** | Save state and exit app |

### Edit Mode (Long-press Select)

| Button | Action |
|--------|--------|
| **Up (single)** | Increase game time by 1 second |
| **Up (long-press)** | Continuously increase game time |
| **Down (single)** | Decrease game time by 1 second |
| **Down (long-press)** | Continuously decrease game time |
| **Select (single)** | Save default time & exit edit mode |

**Fine-Grained Adjustment**: When editing an already-started game, adjustments are made in 1-second increments. For a new game, adjustments are made in 1-minute increments.

## Building

This is a native Pebble SDK 3 application. To build:

```bash
# Install Pebble SDK if not already installed
# https://developer.pebble.com/docs/sdk/

# Build for all platforms
pebble build
```

The application targets all Pebble platforms:
- aplite (Black & white square)
- basalt (Color square)
- chalk (Black & white round)
- diorite (Color round)
- emery, flint, gabbro (Pebble 2015+ devices)

## Installation

1. Build the application using `pebble build`
2. Transfer the `.pbw` file from the `build` directory to your Pebble device
3. Or use `pebble install` to install directly from the build directory

## Architecture

- **Single Window**: Two-tier display with play clock on top (63% of screen) and game clock on bottom (37%)
- **Custom 7-Segment Display**: Hexagonal segment geometry computed at runtime from screen dimensions
- **App Timers**: 1-second tick handler for countdown, long-press repeat for rapid adjustment
- **Vibration Patterns**: Custom haptic feedback for timing alerts

## Resources

- Custom font: JetBrains Mono Nerd Font Mono Bold (48pt and 32pt variants)
- Character set optimized for digits and colon

## License

MIT
