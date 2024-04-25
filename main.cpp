#include <iostream>
#include <fstream>
#include <map>
#include <vector>
#include <chrono>
#include <thread>
#include <SDL2/SDL.h>

#include "cpu.h"

constexpr auto SPACE_INVADERS_BIN = "space-invaders/invaders";
constexpr auto WIDTH = 224 * 4;
constexpr auto HEIGHT = 256 * 4;

constexpr auto VIDEO_RAM_START = 0x2400;
constexpr auto FRAME_BUFFER_WIDTH = 256;
constexpr auto FRAME_BUFFER_HEIGHT = 224;
constexpr auto VIDEO_BUFFER_SIZE = VIDEO_RAM_START + (FRAME_BUFFER_WIDTH * FRAME_BUFFER_HEIGHT) / 8;

void load_rom(CPUState& cpu, const std::string& filename) {
  std::ifstream bin_in(filename, std::ios::binary);
  if (!bin_in.is_open()) {
    throw std::runtime_error("Error: Could not open file " + filename);
  }

  // Read binary size.
  bin_in.seekg(0, std::ios::end);
  size_t bin_size = bin_in.tellg();

  // Copy binary data to memory.
  uint8_t* bin_data = new uint8_t[bin_size];
  bin_in.seekg(0, std::ios::beg);
  bin_in.read((char*)bin_data, bin_size);

  // Close binary file.
  bin_in.close();

  // Initialize RAM with the ROM.
  memset(cpu.ram, 0, 0xFFFF);
  memcpy(cpu.ram, bin_data, bin_size);
  delete[] bin_data;
}

void cpu_loop(CPUState& cpu) {
  long frame_count = 0;
  long cycle_count = 0;
  bool first_interrupt = true;

  auto last_interrupt_time = std::chrono::high_resolution_clock::now();
  auto last_cycle_check_time = std::chrono::high_resolution_clock::now();

  while (true) {
    const auto now { std::chrono::high_resolution_clock::now() };

    // Call interrupt every 8ms.
    if (std::chrono::duration_cast<std::chrono::milliseconds>(now - last_interrupt_time).count() >= 8) {
      interrupt_cpu(cpu, first_interrupt ? 1 : 2);
      first_interrupt = !first_interrupt;
      last_interrupt_time = now;
    }

    // Check cycle count every 1 second.
    if (std::chrono::duration_cast<std::chrono::seconds>(now - last_cycle_check_time).count() >= 1) {
      std::cout << "Cycles per second: " << cycle_count << std::endl;
      cycle_count = 0;
      last_cycle_check_time = now;

      std::cout << "Frames per second: " << frame_count << std::endl;
      frame_count = 0;
    }

    cycle_count += cycle_cpu(cpu);
    if (cpu.halt) {
      break;
    }
  }
}

int main(int argc, char* argv[]) {
  // Initialize SDL.
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    std::cerr << "Error: Could not initialize SDL" << std::endl;
    return 1;
  }

  // Create window.
  SDL_Window* window = nullptr;
  window = SDL_CreateWindow(
    "8080 Emulator",
    SDL_WINDOWPOS_CENTERED,
    SDL_WINDOWPOS_CENTERED,
    WIDTH,
    HEIGHT,
    0
  );

  // Create renderer.
  SDL_Renderer* renderer = nullptr;
  renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

  // Create frame buffer.
  SDL_Texture* frame_buffer_texture = nullptr;
  frame_buffer_texture = SDL_CreateTexture(
    renderer,
    SDL_PIXELFORMAT_RGBA8888,
    SDL_TEXTUREACCESS_STREAMING,
    FRAME_BUFFER_WIDTH,
    FRAME_BUFFER_HEIGHT
  );

  // Initialize frame buffer.
  uint32_t frame_buffer[FRAME_BUFFER_WIDTH * FRAME_BUFFER_HEIGHT];
  memset(frame_buffer, 0, FRAME_BUFFER_WIDTH * FRAME_BUFFER_HEIGHT * sizeof(uint32_t));
  SDL_UpdateTexture(frame_buffer_texture, NULL, frame_buffer, FRAME_BUFFER_WIDTH * sizeof(uint32_t));

  // Initialize CPU state.
  CPUState cpu;
  init_cpu_state(cpu);

  // Load Space Invaders ROM.
  load_rom(cpu, SPACE_INVADERS_BIN);

  // Start CPU loop.
  std::thread cpu_thread(cpu_loop, std::ref(cpu));

  // Bitmask for each input.
  std::map<uint8_t, uint8_t> input_map = {
    {'c', 1},             // Insert coins
    {'1', 1 << 2},        // Start 1 player
    {'2', 1 << 1},        // Start 2 player
    {SDLK_LEFT, 1 << 5},  // Move left
    {SDLK_RIGHT, 1 << 6}, // Move right
    {SDLK_SPACE, 1 << 4}  // Fire
  };
  
  // Variables for the texture rotation.
  float angle = 0;
  SDL_Point center { HEIGHT / 2, WIDTH / 2 };
  SDL_Rect dest { (WIDTH - HEIGHT) / 2, (HEIGHT - WIDTH) / 2, HEIGHT, WIDTH };

  // Variables for the frame rate.
  auto last_render_time =  std::chrono::high_resolution_clock::now();

  while (true) {
    const auto now { std::chrono::high_resolution_clock::now() };

    // Basic event handling.
    SDL_Event e;
    if (SDL_PollEvent(&e)) {
      if (e.type == SDL_QUIT)
        break;
      else if (e.type == SDL_KEYUP && e.key.keysym.sym == SDLK_ESCAPE)
        break;

      if (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP) {
        auto input_mask = input_map.find(e.key.keysym.sym);
        if (input_mask != input_map.end()) {
          cpu.input_ports[1] = e.type == SDL_KEYDOWN 
            ? cpu.input_ports[1] | input_mask->second
            : cpu.input_ports[1] & ~input_mask->second;
        }
      }
    }

    // Update frame buffer.
    for (int i = VIDEO_RAM_START; i < VIDEO_BUFFER_SIZE; i++) {
      uint8_t video_byte = cpu.ram[i];
      int byte_index = i - VIDEO_RAM_START;
      for (int j = 0; j < 8; j++) {
        frame_buffer[byte_index * 8 + j] = (video_byte & (1 << j)) != 0 
          ? 0xFF000000 
          : 0x00000000;
      }
    }

    // Render only every 16ms.
    if (std::chrono::duration_cast<std::chrono::milliseconds>(now - last_render_time).count() < 16) {
      continue;
    }

    // Update frame buffer texture.
    SDL_UpdateTexture(frame_buffer_texture, NULL, frame_buffer, FRAME_BUFFER_WIDTH * sizeof(uint32_t));

    // Clear the screen.
    SDL_RenderClear(renderer);

    // Copy the texture to the rendering context.
    SDL_RenderCopyEx(renderer, frame_buffer_texture, NULL, &dest, 270, NULL, SDL_FLIP_NONE);

    // Present the image.
    SDL_RenderPresent(renderer);
  }

  // Wait for CPU thread to finish.
  cpu.halt = true;
  cpu_thread.join();

  SDL_DestroyTexture(frame_buffer_texture);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);

  return 0;
}
