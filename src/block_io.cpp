#include "block_io.h"

#include <fstream>
#include <iostream>

// File format:
//   [uint32] block_count
//   [uint32] original_block_size (bytes)
//   [uint64 x block_count] compressed size of each block
//   [raw bytes] compressed block 0, 1, ...

static constexpr size_t HEADER_FIXED_BYTES = sizeof(uint32_t) + sizeof(uint32_t);

bool write_compressed_file(const std::string& path,
                           const std::vector<CompressedBlock>& blocks,
                           uint32_t original_block_size)
{
    std::ofstream out(path, std::ios::binary);
    if (!out) {
        std::cerr << "Error: no se pudo abrir para escritura: " << path << "\n";
        return false;
    }

    uint32_t block_count = static_cast<uint32_t>(blocks.size());
    out.write(reinterpret_cast<const char*>(&block_count), sizeof(block_count));
    out.write(reinterpret_cast<const char*>(&original_block_size), sizeof(original_block_size));

    for (const auto& block : blocks) {
        uint64_t sz = static_cast<uint64_t>(block.data.size());
        out.write(reinterpret_cast<const char*>(&sz), sizeof(sz));
    }

    for (const auto& block : blocks) {
        out.write(reinterpret_cast<const char*>(block.data.data()),
                  static_cast<std::streamsize>(block.data.size()));
    }

    return out.good();
}

bool read_compressed_header(const std::string& path,
                            uint32_t& block_count,
                            uint32_t& original_block_size,
                            std::vector<uint64_t>& compressed_sizes)
{
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        std::cerr << "Error: no se pudo abrir para lectura: " << path << "\n";
        return false;
    }

    in.read(reinterpret_cast<char*>(&block_count), sizeof(block_count));
    in.read(reinterpret_cast<char*>(&original_block_size), sizeof(original_block_size));

    compressed_sizes.resize(block_count);
    for (uint32_t i = 0; i < block_count; ++i) {
        in.read(reinterpret_cast<char*>(&compressed_sizes[i]), sizeof(uint64_t));
    }

    return in.good();
}

bool read_compressed_block(const std::string& path,
                           uint32_t block_index,
                           const std::vector<uint64_t>& compressed_sizes,
                           std::vector<uint8_t>& out_data)
{
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        std::cerr << "Error: no se pudo abrir para lectura: " << path << "\n";
        return false;
    }

    // Skip fixed header and size table
    size_t offset = HEADER_FIXED_BYTES
                  + compressed_sizes.size() * sizeof(uint64_t);

    // Skip all blocks before block_index
    for (uint32_t i = 0; i < block_index; ++i) {
        offset += static_cast<size_t>(compressed_sizes[i]);
    }

    in.seekg(static_cast<std::streamoff>(offset));
    if (!in) {
        std::cerr << "Error: seek fallido en bloque " << block_index << "\n";
        return false;
    }

    size_t sz = static_cast<size_t>(compressed_sizes[block_index]);
    out_data.resize(sz);
    in.read(reinterpret_cast<char*>(out_data.data()), static_cast<std::streamsize>(sz));

    return in.good();
}
