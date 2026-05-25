#include "decompressor.h"
#include "block_io.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <pthread.h>
#include <zlib.h>
#include <time.h>
#include <cstring>

// ── Shared state between the main thread and worker threads ──────────────────

struct DecompWorkerArgs {
    // Input (read-only after spawn)
    std::string            input_path;
    const std::vector<uint64_t>* compressed_sizes;
    uint32_t               original_block_size;
    uint32_t               num_blocks;

    // Work queue
    uint32_t            next_block;
    pthread_mutex_t     queue_mutex;

    // Output: decompressed bytes per block, in order
    std::vector<std::vector<uint8_t>> results;

    // Ordered-write gate
    uint32_t            next_to_write;
    pthread_mutex_t     write_mutex;
    pthread_cond_t      write_cond;
};

// ── Worker thread ─────────────────────────────────────────────────────────────

static void* decompress_worker(void* arg)
{
    DecompWorkerArgs* shared = static_cast<DecompWorkerArgs*>(arg);

    while (true) {
        // Claim the next block
        pthread_mutex_lock(&shared->queue_mutex);
        if (shared->next_block >= shared->num_blocks) {
            pthread_mutex_unlock(&shared->queue_mutex);
            break;
        }
        uint32_t idx = shared->next_block++;
        pthread_mutex_unlock(&shared->queue_mutex);

        // Read compressed block from disk
        std::vector<uint8_t> compressed;
        if (!read_compressed_block(shared->input_path, idx,
                                   *shared->compressed_sizes, compressed)) {
            std::cerr << "Error leyendo bloque comprimido " << idx << "\n";
            return nullptr;
        }

        // Decompress
        uLongf decompressed_bound = static_cast<uLongf>(shared->original_block_size) * 2;
        std::vector<uint8_t> decompressed(decompressed_bound);

        int ret;
        while (true) {
            ret = uncompress(decompressed.data(), &decompressed_bound,
                             compressed.data(),
                             static_cast<uLong>(compressed.size()));
            if (ret == Z_BUF_ERROR) {
                // Buffer was too small — double and retry
                decompressed_bound *= 2;
                decompressed.resize(decompressed_bound);
            } else {
                break;
            }
        }

        if (ret != Z_OK) {
            std::cerr << "Error descomprimiendo bloque " << idx
                      << " (codigo " << ret << ")\n";
            return nullptr;
        }
        decompressed.resize(decompressed_bound);

        // Wait for our turn, then store the result
        pthread_mutex_lock(&shared->write_mutex);
        while (shared->next_to_write != idx) {
            pthread_cond_wait(&shared->write_cond, &shared->write_mutex);
        }
        shared->results[idx] = std::move(decompressed);
        shared->next_to_write++;
        pthread_cond_broadcast(&shared->write_cond);
        pthread_mutex_unlock(&shared->write_mutex);
    }

    return nullptr;
}

// ── Integrity check ───────────────────────────────────────────────────────────

static bool files_are_equal(const std::string& a, const std::string& b)
{
    std::ifstream fa(a, std::ios::binary | std::ios::ate);
    std::ifstream fb(b, std::ios::binary | std::ios::ate);
    if (!fa || !fb) return false;

    auto sa = fa.tellg();
    auto sb = fb.tellg();
    if (sa != sb) return false;

    fa.seekg(0);
    fb.seekg(0);

    constexpr size_t CHUNK = 1 << 20; // 1 MB read window
    std::vector<char> ba(CHUNK), bb(CHUNK);

    while (fa && fb) {
        fa.read(ba.data(), static_cast<std::streamsize>(CHUNK));
        fb.read(bb.data(), static_cast<std::streamsize>(CHUNK));
        auto ra = fa.gcount();
        auto rb = fb.gcount();
        if (ra != rb) return false;
        if (std::memcmp(ba.data(), bb.data(), static_cast<size_t>(ra)) != 0)
            return false;
    }
    return true;
}

// ── Public API ────────────────────────────────────────────────────────────────

DecompressionResult decompress_file(const std::string& input_path,
                                    const std::string& output_path,
                                    int num_threads,
                                    const std::string& original_path)
{
    uint32_t block_count = 0;
    uint32_t original_block_size = 0;
    std::vector<uint64_t> compressed_sizes;

    if (!read_compressed_header(input_path, block_count,
                                original_block_size, compressed_sizes)) {
        return {};
    }

    DecompWorkerArgs shared{};
    shared.input_path          = input_path;
    shared.compressed_sizes    = &compressed_sizes;
    shared.original_block_size = original_block_size;
    shared.num_blocks          = block_count;
    shared.next_block          = 0;
    shared.next_to_write       = 0;
    shared.results.resize(block_count);
    pthread_mutex_init(&shared.queue_mutex, nullptr);
    pthread_mutex_init(&shared.write_mutex, nullptr);
    pthread_cond_init(&shared.write_cond, nullptr);

    struct timespec t_start, t_end;
    clock_gettime(CLOCK_MONOTONIC, &t_start);

    std::vector<pthread_t> threads(num_threads);
    for (int i = 0; i < num_threads; ++i) {
        pthread_create(&threads[i], nullptr, decompress_worker, &shared);
    }
    for (int i = 0; i < num_threads; ++i) {
        pthread_join(threads[i], nullptr);
    }

    clock_gettime(CLOCK_MONOTONIC, &t_end);

    pthread_mutex_destroy(&shared.queue_mutex);
    pthread_mutex_destroy(&shared.write_mutex);
    pthread_cond_destroy(&shared.write_cond);

    // Write all blocks to the output file in order
    std::ofstream out(output_path, std::ios::binary);
    if (!out) {
        std::cerr << "Error: no se pudo crear " << output_path << "\n";
        return {};
    }
    uint64_t total_bytes = 0;
    for (const auto& block : shared.results) {
        out.write(reinterpret_cast<const char*>(block.data()),
                  static_cast<std::streamsize>(block.size()));
        total_bytes += block.size();
    }
    out.close();

    double elapsed = (t_end.tv_sec  - t_start.tv_sec)
                   + (t_end.tv_nsec - t_start.tv_nsec) / 1e9;

    bool ok = original_path.empty() ? true : files_are_equal(original_path, output_path);

    return { elapsed, total_bytes, ok };
}
