# Genesis Audio Software

## Status

Not cool yet.

## The Vision

 * Safe plugins. Plugins crashing must not crash the studio.
 * Cross-platform.
 * Projects must work on every computer. It's not possible to have a plugin
   that works on one person's computer and not another.
 * Tight integration with an online sample/project sharing service. Make it
   almost easier to save it open source than to save it privately.
 * Multiplayer support. Each person can simultaneously edit different sections.
 * Backend decoupled from the UI. Someone should be able to depend only
   on a C library and headlessly synthesize music.
 * Take full advantage of multiple cores.
 * Sample-accurate mixing.
 * Never require the user to restart the program
 * Let's get these things right the first time around:
   - Undo/redo
   - Ability to edit multiple projects at once. Mix and match
   - Support for N audio channels instead of hardcoded stereo

## Contributing

genesis is programmed in a tiny subset of C++:

 * No linking against libstdc++.
 * No STL.
 * No `new` or `delete`.
 * No exceptions or run-time type information.
 * Pass pointers instead of references.

### Building and Running

```
sudo apt-get install libsdl2-dev libepoxy-dev libglm-dev \
    libfreetype6-dev libpng12-dev librucksack-dev unicode-data libavcodec-dev \
    libavformat-dev libavutil-dev libavfilter-dev
make
./build/genesis
```

## Roadmap

 0. Playback to speakers with SDL
 0. Undo/Redo
 0. Multiplayer
 0. Record audio
