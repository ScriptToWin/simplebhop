# Standalone CS2 Linux Bunnyhop

A lightweight, external auto-bhop macro written in C++ for Linux (CS2). It simulates scroll-wheel inputs using `uinput` only while the Counter-Strike 2 process is running.

## Features
* **Process-Aware:** Automatically activates only when `cs2` is running and forces itself off if the game closes.
* **Toggle Control:** Press **Mouse4** (side button) to toggle the bunnyhop ON or OFF.
* **AFK / Stationary Support:** Works even while standing still or performing stationary hops.

## Requirements
* Linux with `uinput` support.
* Root privileges (required to read input devices and write to `/dev/uinput`).
* `g++` compiler supporting C++17.

## Setup & Usage

1. **Bind Controls in CS2:**
   * Open CS2 settings and bind **Mouse Wheel Up** to `+jump`.
2. **Run the Script:**
   * Open your terminal in the script directory and run:
     ```bash
     sudo bash run_bhop.sh
     ```
3. **Controls:**
   * **Mouse4:** Toggle ON/OFF
   * **Ctrl + C:** Quit the program
