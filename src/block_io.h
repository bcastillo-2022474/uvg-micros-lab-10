#pragma once

#include <cstdint>
#include <string>
#include <vector>

// A single compressed block: its index and the raw compressed bytes.
struct CompressedBlock {
    uint32_t index;
    std::vector<uint8_t> data;
};

// Writes the header + all compressed blocks to a file in index order.
// Returns true on success.
bool write_compressed_file(const std::string& path,
                           const std::vector<CompressedBlock>& blocks,
                           uint32_t original_block_size);

// Reads the header from a compressed file.
// Fills out block_count, original_block_size, and the per-block compressed sizes.
// Returns true on success.
bool read_compressed_header(const std::string& path,
                            uint32_t& block_count,
                            uint32_t& original_block_size,
                            std::vector<uint64_t>& compressed_sizes);

// Reads one compressed block by index from the file.
// Caller must have already read the header to know offsets.
bool read_compressed_block(const std::string& path,
                           uint32_t block_index,
                           const std::vector<uint64_t>& compressed_sizes,
                           std::vector<uint8_t>& out);
