/**
 ****************************************
 * wadify : wad compresser/decompresser *
 ****************************************
**/

#include "utils.hpp"

#include <array>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
#include <format>

namespace {
struct wad_header {
  std::uint32_t magic{};
  std::uint32_t timestamp{};
  std::uint32_t num_entries{};
  std::uint32_t ffotd_version{};
};

struct wad_entry {
  std::string name;
  std::uint32_t compressed_size{};
  std::uint32_t size{};
  std::uint32_t offset{};
};

wad_header read_wad_header(std::span<const std::uint8_t> data) {
  return {
    .magic = utils::read_be_u32(&data[0]),
    .timestamp = utils::read_be_u32(&data[4]),
    .num_entries = utils::read_be_u32(&data[8]),
    .ffotd_version = utils::read_be_u32(&data[12])
  };
}

wad_entry read_wad_entry(std::span<const std::uint8_t> data,
                         std::size_t index) {
    const auto base = 16 + index * 44;
    std::string name(reinterpret_cast<const char*>(&data[base]),
      strnlen(reinterpret_cast<const char*>(&data[base]), 32));
    name = name.c_str(); // null-terminated cleanup

    return {
      .name = name,
      .compressed_size = utils::read_be_u32(&data[base + 32]),
      .size = utils::read_be_u32(&data[base + 36]),
      .offset = utils::read_be_u32(&data[base + 40])
    };
}

// console colour codes
constexpr auto red = "\033[31m";
constexpr auto yellow = "\033[33m";
constexpr auto clear = "\033[0m";

namespace fs = std::filesystem;

bool decompress_wad(const std::string& file_name,
                    const std::optional<std::string>& output_folder) {
  std::cout << std::format("decompressing: {}...\n", file_name);

  const auto data = utils::read_file(file_name);
  if (data.empty()) {
    utils::print_err("failed to read '{}'", file_name);
    return false;
  }

  const auto header = read_wad_header(data);
  if (header.magic != 0x543377AB) {
    utils::print_err(
      "WAD has bad magic! expected 0x543377AB, got 0x{:08X}",
      header.magic);
    return false;
  }

  std::cout << "\nfile info:\n";
  std::cout << std::format("magic: 0x{:08X}\n", header.magic);
  const auto time = utils::format_timestamp(header.timestamp);
  if (time) {
    std::cout << std::format("timestamp: {} (0x{:08X})\n", *time, header.timestamp);
  } else {
    std::cout << "timestamp: N/A\n"; // nice clean string instead of garbage
  }
  std::cout << std::format("entries: {}\n", header.num_entries);
  std::cout << std::format("ffotd version: {}\n\n", header.ffotd_version);

  std::vector<wad_entry> entries;
  for (std::uint32_t i = 0; i < header.num_entries; ++i) {
    entries.push_back(read_wad_entry(data, i));
  }
  // check if theres entries
  if (entries.empty()) {
    utils::print_warn("{} has no entries!?", file_name);
    return false;
  }

  fs::path output_dir = output_folder
    ? fs::absolute(*output_folder)
    : fs::absolute(fs::path(file_name).stem());
  // create a folder if one doesnt exist
  if (!fs::exists(output_dir)) {
    fs::create_directories(output_dir);
  }

  for (const auto& entry : entries) {
    if (static_cast<std::size_t>(entry.offset) +
        entry.compressed_size > data.size()) {
      utils::print_err("{} data is out of bounds", entry.name);
      return false;
    }

    const std::vector compressed_data(
      data.begin() + entry.offset,
      data.begin() + entry.offset + entry.compressed_size);
    auto decompressed_data = utils::decompress_file(compressed_data);
    if (decompressed_data.empty()) {
      utils::print_err("failed to decompress entry: {}", entry.name);
      return false;
    }
    auto output_path = output_dir / entry.name;
    std::ofstream out(output_path, std::ios::binary);
    if (!out.is_open()) {
      utils::print_err("failed to write: {}", entry.name);
      return false;
    }

    out.write(reinterpret_cast<const char*>(
      decompressed_data.data()),
      decompressed_data.size());
    out.close();

    // set file time stamps
    auto file_time = std::chrono::system_clock::from_time_t(header.timestamp);
    fs::last_write_time(output_path,
      fs::file_time_type::clock::now() +
      (file_time - std::chrono::system_clock::now()));

    // tell the user what we decompressed
    std::cout << std::format("decompressed: {}\n", entry.name);
  }
  std::cout << "\ndone";
  return true;
}

// important info:
// magic HAS to be 0x543377AB
// 0x543377AB = T3w«
// timestamp is auto set
// num_entries is auto set
// ffotd_version is auto set as 1
// however you can change this to 0
// the game doesn't care if it's 1 or 0
bool compress_folder(const std::string& folder_name) {
  std::cout << std::format("compressing: {}...\n\n", folder_name);
  // if the input has .wad
  // strip it
  fs::path path(folder_name);
  auto folder_base = path.extension() == ".wad"
    ? path.stem().string()
    : path.string();
  if (!fs::exists(path)) {
    utils::print_err("'{}' does not exist", folder_base);
    return false;
  }

  wad_header header{
    .magic = 0x543377AB,
    .timestamp = static_cast<std::uint32_t>(std::time(nullptr)),
    .ffotd_version = 1 // what retail bo2 set's it to
  };

  std::vector<wad_entry> entries;
  std::vector<std::vector<std::uint8_t>> compressed_datas;
  const auto dir_it = fs::directory_iterator(folder_base);
  const std::uint32_t entry_count =
    static_cast<std::uint32_t>(std::distance(dir_it, {}));
  std::uint32_t current_offset = 16 + (entry_count * 44);
  for (const auto& entry : fs::directory_iterator(folder_name)) {
    if (!entry.is_regular_file()) {
      continue;
    }
    const auto& path = entry.path();
    auto file_name = path.filename().string();
    // skip hidden/system files
    if (file_name.starts_with(".")) {
      utils::print_warn("skipping hidden/system file: {}\n", file_name);
      continue;
    }
    // skip nested wads
    if (path.extension() == ".wad") {
      utils::print_warn("skipping nested wad: {}\n", file_name);
      continue;
    }
    if (file_name.size() > 42) {
      utils::print_err("name too long in: '{}'", file_name);
      return false;
    }
    constexpr std::string_view invalid_chars = R"(\ /:*?<>|)";
    if (invalid_chars.find_first_of(file_name) != std::string_view::npos) {
      utils::print_err("invalid character in: '{}'", file_name);
      return false;
    }
    const auto file_data = utils::read_file(path);
    if (file_data.empty()) {
      utils::print_err("failed to read: '{}'", file_name);
      return false;
    }
    auto compressed_data = utils::compress_file(file_data);
    if (compressed_data.empty()) {
      utils::print_err("failed to compress: '{}'", file_name);
      return false;
    }

    entries.push_back({
      .name = file_name,
      .compressed_size = static_cast<std::uint32_t>(compressed_data.size()),
      .size = static_cast<std::uint32_t>(file_data.size()),
      .offset = current_offset
    });

    current_offset += static_cast<std::uint32_t>(compressed_data.size());
    compressed_datas.push_back(std::move(compressed_data));
    // let the user know what we compressed
    std::cout << std::format("compressed: {}\n", file_name);
  }

  std::cout << "\nfile info:\n";
  std::cout << std::format("magic: 0x{:08X}\n", header.magic);
  const auto time = utils::format_timestamp(header.timestamp);
  if (time) {
    std::cout << std::format("timestamp: {} (0x{:08X})\n", *time, header.timestamp);
  } else {
    std::cout << "timestamp: N/A\n"; // nice clean string instead of garbage
  }
  header.num_entries = static_cast<std::uint32_t>(entries.size());
  std::cout << std::format("entries: {}\n", header.num_entries);
  std::cout << std::format("ffotd version: {}\n\n", header.ffotd_version);

  // write header
  std::vector<std::uint8_t> wad_data;
  utils::write_be_u32(wad_data, header.magic);
  utils::write_be_u32(wad_data, header.timestamp);
  utils::write_be_u32(wad_data, header.num_entries);
  utils::write_be_u32(wad_data, header.ffotd_version);

  // write entries
  for (const auto& entry : entries) {
    std::array<char, 32> name_buf{};
    std::memcpy(name_buf.data(), entry.name.c_str(), entry.name.size());
    wad_data.insert(wad_data.end(),
      reinterpret_cast<std::uint8_t*>(name_buf.data()),
      reinterpret_cast<std::uint8_t*>(name_buf.data()) + 32);

    utils::write_be_u32(wad_data, entry.compressed_size);
    utils::write_be_u32(wad_data, entry.size);
    utils::write_be_u32(wad_data, entry.offset);
  }
  // write compressed data blocks
  for (const auto& blocks : compressed_datas) {
    wad_data.insert(wad_data.end(), blocks.begin(), blocks.end());
  }
  const auto out_file = fs::path(folder_base).filename().string() + ".wad";
  if (!utils::write_file(out_file, wad_data)) {
    utils::print_err("failed to write output file: '{}'", out_file);
    return false;
  }
  std::cout << "done\n";
  return true;
}

bool decompress_cmd(int argc, char* argv[]) {
  if (argc < 3) {
    std::cerr << yellow << "usage: wadify.exe --decompress <input>" << clear;
    return false;
  }

  const std::basic_string input_file = utils::add_wad_ext(argv[2]);
  std::optional<std::string> output_folder;

  for (auto i = 3; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "--output-folder" ||
        arg == "-o") {
      if (i + 1 < argc) {
        output_folder = argv[++i];
      } else {
        std::cerr << yellow << "usage: wadify.exe --output-folder <input>" << clear;
        return false;
      }
    }
  }
  return decompress_wad(input_file, output_folder);
}

bool compress_cmd(int argc, char* argv[]) {
  if (argc < 3) {
    std::cerr << yellow << "usage: wadify.exe --compress <input>" << clear;
    return false;
  }
  return compress_folder(argv[2]);
}

bool help_cmd(int /*argc*/, char** /*argv*/) {
  // just general help for the tool
  std::cout << "Usage:\n"
    << "--decompress, -d <input>\n"
    << "--compress, -c <input>\n"
    << "--output-folder, -o <path>\n"
    << "--help, -h, ?\n"
    << "--about, -a\n";
  return true;
}

bool about_cmd(int /*argc*/, char** /*argv*/) {
  std::cout << "wadify.exe: 3arc wad tool by indoorhinge\n";
  return true;
}
} // namespace

int main(int argc, char* argv[]) {
  if (argc < 2) {
    std::cerr << yellow << "usage: wadify.exe <cmd>" << clear;
    return 1;
  }
  // now check what the user wants to do
  const std::string cmd = argv[1];
  const std::unordered_map<std::string_view,
    std::function<bool(int, char**)>> cmd_list = {
    {"--decompress", decompress_cmd},
    {"-d",           decompress_cmd},
    {"--compress",   compress_cmd},
    {"-c",           compress_cmd},
    {"--help",       help_cmd},
    {"-h",           help_cmd},
    {"-?",           help_cmd},
    {"--about",      about_cmd},
    {"-a",           about_cmd},
  };

  auto it = cmd_list.find(cmd);
  if (it != cmd_list.end()) {
    return it->second(argc, argv) ? 0 : 1;
  }

  std::cerr << yellow << std::format("unknown cmd: '{}'", cmd) << clear;
  return 1;
}