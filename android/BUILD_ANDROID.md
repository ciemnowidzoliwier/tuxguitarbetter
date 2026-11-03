# Building TuxGuitar Android APK

## Prerequisites

- Android SDK (API 35 / Android 15)
- Android NDK (r26 or later)
- Gradle 8.7+
- Java 17+

## Project Structure

```
android/
├── build-scripts/tuxguitar-android/     # Main Gradle project
│   ├── apk/                             # APK application module
│   ├── settings.gradle                  # Module definitions
│   └── ...
├── TuxGuitar-android/                   # Core library (shared Java sources)
├── TuxGuitar-android-gervill/           # Gervill SoundFont synth (Java)
├── TuxGuitar-android-gervill-soundbank/ # Default SoundFont
├── TuxGuitar-android-midi/              # MIDI I/O
├── TuxGuitar-android-browser-saf/       # File browser (SAF)
├── TuxGuitar-android-resources/         # Resources (icons, translations)
├── TuxGuitar-android-fluidsynth/        # FluidSynth (native JNI)
└── TuxGuitar-android-nam/               # NAM Neural Amp Modeler (native JNI)
```

## Building

### From Command Line

```bash
cd android/build-scripts/tuxguitar-android

# Make gradlew executable
chmod +x gradlew

# Build debug APK
./gradlew :apk:assembleDebug

# Build release APK
./gradlew :apk:assembleRelease

# Build specific native modules
./gradlew :tuxguitar-android-fluidsynth:build
./gradlew :tuxguitar-android-nam:build
```

### From Android Studio

1. Open `android/build-scripts/tuxguitar-android` as project
2. Wait for Gradle sync
3. Build → Make Project
4. Build → Build Bundle(s) / APK(s) → Build APK(s)

## Adding SoundFont Files (.sf2)

Place `.sf2` files in:
```
android/TuxGuitar-android/assets/soundfonts/
```

Examples:
- `GeneralUser_GS.sf2`
- `FluidR3_GM.sf2`
- `PersonalCopy_Lite.sf2`

In the app settings, reference them as: `soundfonts/GeneralUser_GS.sf2`

## Adding NAM Model Files (.nam)

Place `.nam` files in:
```
android/TuxGuitar-android/assets/models/
```

Examples:
- `Modern_High_Gain.nam`
- `Clean_Fender.nam`
- `Vintage_Marshall.nam`

In the app settings, reference them as: `models/Modern_High_Gain.nam`

## Architecture Overview

### Java Modules (Shared with Desktop)
- Core tab editing/playback logic
- File format parsers (GPX, GTP, MIDI, MusicXML, etc.)
- Gervill SoundFont synthesizer (pure Java)
- MIDI I/O via Android MIDI API

### Native Modules (JNI)
| Module | Library | Purpose |
|--------|---------|---------|
| `tuxguitar-android-fluidsynth` | `libtuxguitar-fluidsynth-jni.so` | FluidSynth SoundFont player via OpenSL ES |
| `tuxguitar-android-nam` | `libtuxguitar-nam-jni.so` | Neural Amp Modeler (NAM) |

### Native Build Details

Both native modules use CMake with FetchContent:
- **FluidSynth**: Builds FluidSynth v2.3.4 statically, links against OpenSL ES
- **NAM**: Builds NeuralAmpModelerCore v0.6.0 with Eigen/nlohmann/json

ABIs supported: `arm64-v8a`, `armeabi-v7a`, `x86`, `x86_64`

## Key Differences from Desktop

| Feature | Desktop | Android |
|---------|---------|---------|
| VST3/CLAP plugins | ✅ Supported | ❌ Not applicable |
| AudioUnit | ✅ macOS | ❌ Not applicable |
| LV2 | ✅ Linux/FreeBSD | ❌ Not applicable |
| FluidSynth | ✅ JNI + libfluidsynth | ✅ JNI + OpenSL ES |
| NAM | ✅ JNI + NeuralAmpModelerCore | ✅ JNI + NeuralAmpModelerCore |
| Gervill | ✅ Pure Java | ✅ Pure Java |
| MIDI I/O | ALSA/CoreMIDI/WinMM | Android MIDI API |
| File access | Filesystem | Storage Access Framework |

## Asset Loading

Both native modules automatically copy assets to the app's cache directory on first use:
- SoundFonts: `soundfonts/filename.sf2` → cache
- NAM models: `models/filename.nam` → cache

This is required because native code needs a file path, not an asset stream.

## Troubleshooting

### Native library not loading
- Check `adb logcat | grep TG-` for JNI load messages
- Verify ABI matches device (Settings → About → Architecture)
- Ensure `android:extractNativeLibs="true"` in manifest (default)

### CMake build failures
- Verify NDK path: `export ANDROID_NDK_HOME=/path/to/ndk`
- Check CMake version in SDK Manager
- Clean build: `./gradlew clean :tuxguitar-android-fluidsynth:build`

### Audio issues
- FluidSynth uses OpenSL ES (deprecated but still works)
- Consider migrating to AAudio via Oboe library for lower latency
- NAM processing is CPU-intensive; may need buffer size tuning

## License Notes

- **FluidSynth**: LGPL-2.1 (dynamically linked via JNI)
- **NeuralAmpModelerCore**: MIT
- **Eigen**: MPL-2.0
- **nlohmann/json**: MIT
- **TuxGuitar**: GPL-2.0

All native libraries are built from source and included in the APK.