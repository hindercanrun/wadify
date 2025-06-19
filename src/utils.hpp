#pragma once

#include "dependencies/zlib/zlib.h"

#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <format>
#include <optional>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace utils {
std::string add_wad_ext(const std::string& file);
std::string remove_wad_ext(const std::string& file);
std::optional<std::string> format_timestamp(std::uint32_t timestamp);

std::vector<std::uint8_t> read_file(const fs::path& path);
void write_file(const fs::path& path,
                const std::vector<std::uint8_t>& data);

std::vector<std::uint8_t> decompress_file(const std::vector<std::uint8_t>&
                                          compressed_data);
std::vector<std::uint8_t> compress_file(const std::vector<std::uint8_t>& input_data,
                                        int compression_level = Z_BEST_COMPRESSION);

inline std::uint32_t read_be_u32(const void* data) noexcept {
  std::uint32_t value;
  std::memcpy(&value, data, sizeof(value));
  if constexpr (std::endian::native == std::endian::little) {
    return std::byteswap(value);
  }
  return value;
}


inline void write_be_u32(std::vector<std::uint8_t>& out, std::uint32_t value) {
  if constexpr (std::endian::native == std::endian::little) {
    value = std::byteswap(value);
  }

  auto* ptr = reinterpret_cast<std::uint8_t*>(&value);
  out.insert(out.end(), ptr, ptr + sizeof(value));
}
} // namespace utils