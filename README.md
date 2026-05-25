# Lab 10 — Parallel File Compression with Pthreads

**CC3086 Programación de Microprocesadores — Ciclo 1 2026**
Universidad del Valle de Guatemala

---

## Overview

This program compresses and decompresses large files in parallel using POSIX threads (pthreads) and the zlib DEFLATE algorithm. The file is split into fixed-size blocks; each block is processed independently by a worker thread, and results are written back in the correct order using mutex + condition variable synchronization.

---

## Project structure

```
.
├── src/
│   ├── main.cpp          # Entry point, menu, timing
│   ├── compressor.cpp    # Parallel compression logic
│   ├── compressor.h
│   ├── decompressor.cpp  # Parallel decompression logic
│   ├── decompressor.h
│   ├── block_io.cpp      # File format: header read/write helpers
│   └── block_io.h
├── build/                # Compiled objects and binary (git-ignored)
├── Makefile
└── README.md
```

---

## Dependencies

- `g++` with C++17 support
- `zlib` development headers

Install on Ubuntu/WSL:

```bash
sudo apt install zlib1g-dev
```

---

## Build

```bash
make
```

The binary is produced at `build/parallel_compression`.

```bash
make clean   # remove build artifacts
make run     # build and run
```

---

## Usage

```
./build/parallel_compression
```

The program presents a menu:

```
1. Comprimir archivo
2. Descomprimir archivo previamente comprimido
```

For **compression** you will be asked for:
- Input file path
- Output file path
- Number of threads
- Block size (256 KB / 1 MB / 4 MB)

For **decompression** you will be asked for:
- Compressed input file path
- Output file path
- Number of threads

---

## Compressed file format

```
[uint32]  number of blocks
[uint64]  compressed size of block 0
[uint64]  compressed size of block 1
...
[uint64]  compressed size of block N-1
[bytes]   compressed data for block 0
[bytes]   compressed data for block 1
...
[bytes]   compressed data for block N-1
```

The header allows decompression threads to seek directly to their assigned block without scanning the file sequentially.

---

## Synchronization

| Mechanism | Purpose |
|-----------|---------|
| `pthread_mutex_t` | Protects `next_to_write` counter and file handle |
| `pthread_cond_t` | Blocks a thread until all previous blocks are written (ordered output) |

Without the condition variable, a fast thread could write block 4 before block 2 is done, producing a corrupted file.

---

## Measurements

Run the program with thread counts of 1, 2, 4, and 8 and record the compression time. Then compute:

- **Speedup** = T_sequential / T_parallel
- **Efficiency** = Speedup / N_threads

---

## Authors

- Brandon Castillo — bcastillo-2022474
- Kenett — kenett120
