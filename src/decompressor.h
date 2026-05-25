#pragma once

#include <string>
#include <cstdint>

struct DecompressionResult {
    double elapsed_seconds;
    uint64_t decompressed_bytes;
    bool     integrity_ok;
};

// Decompresses `input_path` into `output_path` using `num_threads` threads.
// If `original_path` is non-empty, verifies byte-for-byte integrity afterward.
DecompressionResult decompress_file(const std::string& input_path,
                                    const std::string& output_path,
                                    int num_threads,
                                    const std::string& original_path = "");
