# Tetris in C (SDL2)

A simple Tetris game written in C using SDL2, SDL2_mixer, and SDL2_ttf.

## Features
- Classic Tetris gameplay
- Sound effects and background music
- High score saving

## Requirements
- SDL2
- SDL2_mixer
- SDL2_ttf
- GCC / MinGW (Windows)

## Build (Windows - MinGW)

gcc src/tetris.c -o tetris ^
-Iinclude ^
-Llib -lSDL2 -lSDL2_mixer -lSDL2_ttf

## Controls
- Arrow keys: Move
- Space: Rotate
