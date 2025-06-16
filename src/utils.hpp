#pragma once

#include "dependencies/zlib/zlib.h"

#include <fstream>
#include <iostream>
#include <filesystem>
#include <cstdint>
#include <array>
#include <string>
#include <format>
#include <chrono>

namespace fs = std::filesystem;

namespace utils {
std::string add_wad_ext(const std::string& file);
std::string remove_wad_ext(const std::string& file);

std::vector<std::uint8_t> read_file(const fs::path& path);
void write_file(const fs::path& path,
                const std::vector<std::uint8_t>& data);

std::vector<std::uint8_t> decompress_file(const std::vector<std::uint8_t>&
                                          compressed_data);
std::vector<std::uint8_t> compress_file(const std::vector<std::uint8_t>& input_data,
                                        int compression_level = Z_BEST_COMPRESSION);

std::uint32_t read_u32_be(const std::uint8_t* ptr);
void write_u32_be(std::vector<std::uint8_t>& buf,
                  std::uint32_t value);
} // namespace utils