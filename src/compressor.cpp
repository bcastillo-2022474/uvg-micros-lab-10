#include "include/compressor.h"
#include "include/block_io.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <cstring>
#include <pthread.h>
#include <semaphore.h>
#include <zlib.h>
#include <time.h>

// ── Shared state between the main thread and worker threads ──────────────────

struct WorkerArgs {
    // Input (read-only after spawn)
    const uint8_t* file_data;
    uint64_t       file_size;
    uint32_t       block_size;
    uint32_t       num_blocks;

    // Work queue: semaphore counts remaining blocks; next_block is the index
    // to claim next. A thread does: sem_wait → read next_block → increment.
    sem_t           queue_sem;
    uint32_t        next_block;
    pthread_mutex_t queue_mutex;  // protects next_block only

    // Output: one slot per block, filled by workers
    std::vector<CompressedBlock> results;

    // Ordered-write gate: workers wait until it's their turn
    uint32_t        next_to_write;
    pthread_mutex_t write_mutex;
    pthread_cond_t  write_cond;
};

// ── Worker thread ─────────────────────────────────────────────────────────────

static void* compress_worker(void* arg)
{
    WorkerArgs* shared = static_cast<WorkerArgs*>(arg);

    while (true) {
        // Block until a work unit is available or a shutdown signal arrives
        sem_wait(&shared->queue_sem);

        pthread_mutex_lock(&shared->queue_mutex);
        if (shared->next_block >= shared->num_blocks) {
            // Shutdown signal: re-post so other threads also wake and exit
            pthread_mutex_unlock(&shared->queue_mutex);
            sem_post(&shared->queue_sem);
            break;
        }
        uint32_t idx = shared->next_block++;
        pthread_mutex_unlock(&shared->queue_mutex);

        // Locate this block inside the in-memory file
        uint64_t offset  = static_cast<uint64_t>(idx) * shared->block_size;
        uint64_t raw_len = (offset + shared->block_size <= shared->file_size)
                         ? shared->block_size
                         : shared->file_size - offset;

        const uint8_t* src = shared->file_data + offset;

        // Compress the block independently
        uLongf bound = compressBound(static_cast<uLong>(raw_len));
        std::vector<uint8_t> compressed(bound);

        uLongf actual_size = bound;
        int ret = compress2(compressed.data(), &actual_size,
                            src, static_cast<uLong>(raw_len),
                            Z_DEFAULT_COMPRESSION);
        if (ret != Z_OK) {
            std::cerr << "Error comprimiendo bloque " << idx
                      << " (codigo " << ret << ")\n";
            return nullptr;
        }
        compressed.resize(actual_size);

        // Wait for our turn to commit the result (ordered write)
        pthread_mutex_lock(&shared->write_mutex);
        while (shared->next_to_write != idx) {
            pthread_cond_wait(&shared->write_cond, &shared->write_mutex);
        }
        shared->results[idx] = { idx, std::move(compressed) };
        shared->next_to_write++;
        pthread_cond_broadcast(&shared->write_cond);
        pthread_mutex_unlock(&shared->write_mutex);
    }

    return nullptr;
}

// ── Public API ────────────────────────────────────────────────────────────────

CompressionResult compress_file(const std::string& input_path,
                                const std::string& output_path,
                                int num_threads,
                                uint32_t block_size)
{
    // Read entire input into memory
    std::ifstream in(input_path, std::ios::binary | std::ios::ate);
    if (!in) {
        std::cerr << "Error: no se pudo abrir " << input_path << "\n";
        return {};
    }
    uint64_t file_size = static_cast<uint64_t>(in.tellg());
    in.seekg(0);
    std::vector<uint8_t> file_data(file_size);
    in.read(reinterpret_cast<char*>(file_data.data()),
            static_cast<std::streamsize>(file_size));
    in.close();

    uint32_t num_blocks = static_cast<uint32_t>(
        (file_size + block_size - 1) / block_size);

    // Set up shared state
    WorkerArgs shared{};
    shared.file_data     = file_data.data();
    shared.file_size     = file_size;
    shared.block_size    = block_size;
    shared.num_blocks    = num_blocks;
    shared.next_block    = 0;
    shared.next_to_write = 0;
    shared.results.resize(num_blocks);

    // Semaphore is initialized to num_blocks: each post represents one block
    // of work available. Workers consume one unit per block they process.
    sem_init(&shared.queue_sem, 0, num_blocks);
    pthread_mutex_init(&shared.queue_mutex, nullptr);
    pthread_mutex_init(&shared.write_mutex, nullptr);
    pthread_cond_init(&shared.write_cond, nullptr);

    struct timespec t_start, t_end;
    clock_gettime(CLOCK_MONOTONIC, &t_start);

    std::vector<pthread_t> threads(num_threads);
    for (int i = 0; i < num_threads; ++i) {
        pthread_create(&threads[i], nullptr, compress_worker, &shared);
    }

    // One extra post after all real work is enqueued so the first thread
    // that finds next_block >= num_blocks propagates shutdown to the rest.
    sem_post(&shared.queue_sem);

    for (int i = 0; i < num_threads; ++i) {
        pthread_join(threads[i], nullptr);
    }

    clock_gettime(CLOCK_MONOTONIC, &t_end);

    sem_destroy(&shared.queue_sem);
    pthread_mutex_destroy(&shared.queue_mutex);
    pthread_mutex_destroy(&shared.write_mutex);
    pthread_cond_destroy(&shared.write_cond);

    bool ok = write_compressed_file(output_path, shared.results, block_size);
    if (!ok) {
        std::cerr << "Error escribiendo archivo comprimido.\n";
        return {};
    }

    uint64_t compressed_total = 0;
    for (const auto& b : shared.results) {
        compressed_total += b.data.size();
    }

    double elapsed = (t_end.tv_sec  - t_start.tv_sec)
                   + (t_end.tv_nsec - t_start.tv_nsec) / 1e9;

    return { elapsed, file_size, compressed_total };
}
