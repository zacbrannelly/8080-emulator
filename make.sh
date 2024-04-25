#!/bin/bash
g++ cpu.cpp main.cpp -o emulator -std=c++20 \
  -L/opt/homebrew/Cellar/sdl2/2.28.5/lib \
  -lSDL2 \
  -I/opt/homebrew/Cellar/sdl2/2.28.5/include \
  -I/opt/homebrew/Cellar/sdl2/2.28.5/include/SDL2
