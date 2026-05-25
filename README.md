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
│   ├── include/
│   │   ├── block_io.h        # File format types and read/write declarations
│   │   ├── compressor.h      # compress_file() declaration and result type
│   │   └── decompressor.h    # decompress_file() declaration and result type
│   ├── block_io.cpp          # Header read/write — defines the binary file format
│   ├── compressor.cpp        # Parallel compression logic (pthreads + zlib)
│   ├── decompressor.cpp      # Parallel decompression logic (pthreads + zlib)
│   └── main.cpp              # Entry point: menu, timing display, benchmark table
├── build/                    # Compiled objects and binary (git-ignored)
├── Makefile
└── README.md
```

---

## Dependencies

This project requires the **zlib** development headers (`zlib.h`) and the **g++** compiler with C++17 support.

### Ubuntu / Debian / WSL

```bash
sudo apt install -y zlib1g-dev
```

### Fedora / RHEL

```bash
sudo dnf install -y zlib-devel
```

Alternatively, use the Makefile shortcut:

```bash
make deps-ubuntu   # Ubuntu / WSL
make deps-fedora   # Fedora
```

---

## Build

```bash
make
```

The binary is produced at `build/parallel_compression`.

```bash
make clean   # remove build artifacts
make run     # build and launch immediately
```

---

## Usage

```bash
./build/parallel_compression
```

The program presents a menu:

```
1. Comprimir archivo
2. Descomprimir archivo previamente comprimido
3. Benchmark de speedup (1/2/4/8 hilos)
0. Salir
```

**Compression** asks for:
- Input file path
- Output file path
- Number of threads
- Block size (256 KB / 1 MB / 4 MB)

**Decompression** asks for:
- Compressed input file path
- Output file path
- Original file path (optional — used for integrity verification)
- Number of threads

**Benchmark** runs compression with 1, 2, 4, and 8 threads automatically and prints a speedup/efficiency table.

---

## Compressed file format

```
[uint32]  number of blocks
[uint32]  original block size (bytes)
[uint64 × N]  compressed size of each block
[bytes]   compressed data for block 0
[bytes]   compressed data for block 1
...
[bytes]   compressed data for block N-1
```

The fixed-size header lets decompression threads seek directly to their block without scanning the file sequentially.

---

## Synchronization

| Mechanism | Purpose |
|-----------|---------|
| `pthread_mutex_t` (queue) | Protects the shared `next_block` counter so each block is claimed by exactly one thread |
| `pthread_mutex_t` (write) + `pthread_cond_t` | Ensures blocks are stored in order — a thread waits until all previous blocks are done before committing its result |

Without the condition variable, a fast thread could write block 4 before block 2 is done, producing a corrupted output file.

---

## Measurements

Run option 3 (benchmark) or run option 1 manually with thread counts 1, 2, 4, 8 and record the times. Then:

- **Speedup** = T₁ / Tₙ
- **Efficiency** = Speedup / N

---

## Authors

- Brandon Castillo — bcastillo-2022474
- Kenett — kenett120
