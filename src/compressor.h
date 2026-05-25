#pragma once

#include <string>
#include <cstdint>

struct CompressionResult {
    double elapsed_seconds;
    uint64_t original_bytes;
    uint64_t compressed_bytes;
};

// Compresses `input_path` into `output_path` using `num_threads` threads.
// Each block is `block_size` bytes (last block may be smaller).
// Returns timing and size information.
CompressionResult compress_file(const std::string& input_path,
                                const std::string& output_path,
                                int num_threads,
                                uint32_t block_size);
