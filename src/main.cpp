/*/
 *
 * tool to compress/decompress wad's
 * this was originally made for T6_greenlight_mp
 * however it also supports all versions of T6 and T5
 *
/*/

#include "utils.hpp"

#include <fstream>
#include <iostream>
#include <filesystem>
#include <cstdint>
#include <array>
#include <string>
#include <string_view>
#include <format>
#include <chrono>
#include <ranges>
#include <vector>

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

struct wad {
  wad_header header;
  std::vector<wad_entry> entries;
};

namespace fs = std::filesystem;

static
wad_header read_wad_header(std::span<const std::uint8_t> data) {
  return {
    .magic = utils::read_u32_be(&data[0]),
    .timestamp = utils::read_u32_be(&data[4]),
    .num_entries = utils::read_u32_be(&data[8]),
    .ffotd_version = utils::read_u32_be(&data[12])
  };
}

static
wad_entry read_wad_entry(std::span<const std::uint8_t> data,
                         std::size_t index) {
    const auto base = 16 + index * 44;
    std::string name(reinterpret_cast<const char*>(&data[base]),
      strnlen(reinterpret_cast<const char*>(&data[base]), 32));
    name = name.c_str(); // null-terminated cleanup

    return {
      .name = name,
      .compressed_size = utils::read_u32_be(&data[base + 32]),
      .size = utils::read_u32_be(&data[base + 36]),
      .offset = utils::read_u32_be(&data[base + 40])
    };
}

// console colour codes
constexpr auto red = "\033[31m";
constexpr auto yellow = "\033[33m";
constexpr auto clear = "\033[0m";

static
bool decompress_wad(const std::string& file_name,
                    const std::optional<std::string>& output_folder) {
  std::cout << std::format("decompressing: {}...\n", file_name);
  try {
    const auto data = utils::read_file(file_name);
    const auto header = read_wad_header(data);
    if (header.magic != 0x543377AB) {
      std::cerr << red << "Error: Invalid WAD magic! Expected 0x543377AB, got 0x" 
                << std::format("{:08X}", header.magic) << clear << "\n";
      return false;
    }

    std::cout << "\nfile info:\n";
    std::cout << std::format("magic: 0x{:08X}\n", header.magic);
    auto time = utils::format_timestamp(header.timestamp);
    if (time) {
      std::cout << std::format(
        "timestamp: {} (0x{:08X})\n",
        *time, header.timestamp);
    } else {
      std::cout << "timestamp: N/A\n"; // clean string instead of garbage
    }
    std::cout << std::format("entries: {}\n", header.num_entries);
    std::cout << std::format("ffotd version: {}\n\n", header.ffotd_version);

    std::vector<wad_entry> entries;
    for (std::uint32_t i = 0; i < header.num_entries; ++i) {
      entries.push_back(read_wad_entry(data, i));
    }
    // check if theres entries
    if (entries.empty()) {
      std::cerr << yellow << std::format("warning: {} has no valid entries.", file_name) << clear;
      return false;
    }

    // create a folder if one doesnt exist
    fs::path output_dir = output_folder
        ? fs::absolute(*output_folder)
        : fs::absolute(fs::path(file_name).stem());
    fs::create_directories(output_dir);

    for (const auto& entry : entries) {
      try {
          if (static_cast<std::size_t>(entry.offset) +
            entry.compressed_size > data.size()) {
            throw std::runtime_error("entry data out of bounds");
          }

        std::vector compressed_data(
          data.begin() + entry.offset,
          data.begin() + entry.offset +
          entry.compressed_size);
        auto decompressed_data = utils::decompress_file(compressed_data);
        auto output_path = output_dir / entry.name;
        std::ofstream out(output_path, std::ios::binary);
        if (!out) {
          throw std::runtime_error("failed to create file");
        }

        out.write(reinterpret_cast<const char*>(decompressed_data.data()),
            decompressed_data.size());
        out.close();
        // set file time stamps
        auto file_time = std::chrono::system_clock::from_time_t(header.timestamp);
        fs::last_write_time(output_path,
          fs::file_time_type::clock::now() +
          (file_time - std::chrono::system_clock::now()));
        // tell the user what we decompressed
        std::cout << std::format("decompressed: {}\n", entry.name);
      } catch (const std::exception& e) {
        std::cerr << red << "error: " << e.what() << clear;
        return false;
      }
    }
  } catch (const std::exception& e) {
    std::cerr << red << "error: " << e.what() << clear;
    return false;
  }

  std::cout << "\ndone\n";
  return true;
}

// important info:
// magic HAS to be 0x543377AB
// 0x543377AB = T3w«
// timestamp is auto set
// num_entries is auto set
// you can manually define ffotd_version
// the game only sets it to 0 or 1
// so i manually put it as 0
static
bool compress_folder(const std::string& folder_name) {
  std::cout << std::format("compressing: {}...\n\n", folder_name);
  try {
    wad_header header{
      .magic = 0x543377AB,
      .timestamp = static_cast<std::uint32_t>(std::time(nullptr)),
      // since post release is 1
      // so let's just set it to 1
      .ffotd_version = 1 // it probably doesn't matter
    };

    std::vector<wad_entry> entries;
    std::vector<std::vector<std::uint8_t>> compressed_datas;
    // Precalculate offset for data start
    std::uint32_t current_offset = 16 + 44 *
      static_cast<std::uint32_t>(
        std::distance(fs::directory_iterator(folder_name), {}));

    for (const auto& entry : fs::directory_iterator(folder_name)) {
      if (!entry.is_regular_file()) {
        continue;
      }
      const auto& path = entry.path();
      auto file_name = path.filename().string();
      // skip hidden/system files
      if (file_name.starts_with(".")) {
        std::cout << yellow
          << std::format("skipping hidden/system file: {}\n",
          file_name) << clear;
        continue;
      }
      // skip nested wads
      if (path.extension() == ".wad") {
        std::cout << yellow
          << std::format("skipping nested wad: {}\n",
          file_name) << clear;
        continue;
      }
      if (file_name.size() > 42) {
        throw std::runtime_error("file name too long: " + file_name);
      }
      for (char c : file_name) {
        if (std::string_view(R"(\ /:*?<>|)").contains(c)) {
          throw std::runtime_error("atleast one bad "
            "character in file name: " + file_name);
        }
      }

      auto file_data = utils::read_file(path);
      auto compressed_data = utils::compress_file(file_data);

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

    header.num_entries = static_cast<std::uint32_t>(entries.size());

    std::cout << "\nfile info:\n";
    std::cout << std::format("magic: 0x{:08X}\n", header.magic);
    auto time = utils::format_timestamp(header.timestamp);
    if (time) {
      std::cout << std::format(
        "timestamp: {} (0x{:08X})\n",
        *time, header.timestamp);
    } else {
      std::cout << "timestamp: N/A\n"; // clean string instead of garbage
    }
    std::cout << std::format("entries: {}\n", header.num_entries);
    std::cout << std::format("ffotd version: {}\n\n", header.ffotd_version);

    // build final wad_data
    std::vector<std::uint8_t> wad_data;
    // write header
    utils::write_u32_be(wad_data, header.magic);
    utils::write_u32_be(wad_data, header.timestamp);
    utils::write_u32_be(wad_data, header.num_entries);
    utils::write_u32_be(wad_data, header.ffotd_version);

    // write entries
    for (const auto& e : entries) {
      std::array<char, 32> name_buf{};
      std::memcpy(name_buf.data(), e.name.c_str(), e.name.size());
      wad_data.insert(wad_data.end(),
        reinterpret_cast<std::uint8_t*>(name_buf.data()),
        reinterpret_cast<std::uint8_t*>(name_buf.data()) + 32);
      utils::write_u32_be(wad_data, e.compressed_size);
      utils::write_u32_be(wad_data, e.size);
      utils::write_u32_be(wad_data, e.offset);
    }
    // write compressed data blocks
    for (const auto& c : compressed_datas) {
      wad_data.insert(wad_data.end(), c.begin(), c.end());
    }

    auto out_file = fs::path(folder_name).filename().string() + ".wad";
    utils::write_file(out_file, wad_data);
  } catch (const std::exception& e) {
    std::cerr << red << "error: " << e.what() << clear;
    return false;
  }

  std::cout << "done\n";
  return true;
}

static
void help() {
  // just general help for the tool
  std::cout << "Usage:\n"
    << "--decompress, -d <input>\n"
    << "--compress, -c <input>\n"
    << "--output-folder, -o <path>\n"
    << "--help, -h, ?\n"
    << "--about, -a\n";
}

static
void about() {
  std::cout << "wadify.exe: 3arc wad tool by indoorhinge\n";
}

int main(int argc, char* argv[]) {
  if (argc < 2) {
    std::cerr << yellow << "usage: wadify.exe <cmd>\n" << clear;
    return 1;
  }
  // now check what the user wants to do
  std::string_view cmd = argv[1];
  if (cmd == "--decompress" ||
      cmd == "-d") {
    if (argc < 3) {
      std::cerr << yellow
        << "usage: wadify.exe --decompress <input>\n" << clear;
      return 1;
    }
    std::string input_file = utils::add_wad_ext(argv[2]);
    std::optional<std::string> output_folder;
    for (auto i = 3; i < argc; ++i) {
      if (std::string_view(argv[i]) == "--output-folder" && i + 1 < argc) {
        output_folder = argv[i + 1];
        ++i; // skip next
      }
    }
    return decompress_wad(input_file, output_folder) ? 0 : 1;
  }
  
  if (cmd == "--compress" ||
      cmd == "-c") {
    if (argc < 3) {
      std::cerr << yellow
        << "usage: wadify.exe --compress <input>\n" << clear;
      return 0;
    }
    return compress_folder(utils::remove_wad_ext(argv[2]))
      ? 0
      : 1;
  }
  
  if (cmd == "--help" ||
      cmd == "-h" ||
      cmd == "-?") {
    help();
    return 0;
  }
  
  if (cmd == "--about" ||
      cmd == "-a") {
    about();
    return 0;
  }

  std::cerr << yellow
    << std::format("unknown cmd: {}\n", cmd) << clear;
  return 1;
}