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
#include <format>
#include <chrono>

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

constexpr auto red = "\033[31m";
constexpr auto yellow = "\033[33m";
constexpr auto clear = "\033[0m";

static
wad_header read_wad_header(const std::vector<std::uint8_t>& data) {
  wad_header header;
  header.magic = utils::read_u32_be(&data[0]);
  header.timestamp = utils::read_u32_be(&data[4]);
  header.num_entries = utils::read_u32_be(&data[8]);
  header.ffotd_version = utils::read_u32_be(&data[12]);
  return header;
}

static
wad_entry read_wad_entry(const std::vector<std::uint8_t>& data,
                         std::size_t index) {
  std::size_t base = 16 + index * 44;
  std::string name(reinterpret_cast<const char*>(&data[base]), 32);
  name = name.c_str(); // null-terminated cleanup

  std::uint32_t csize = utils::read_u32_be(&data[base + 32]);
  std::uint32_t size = utils::read_u32_be(&data[base + 36]);
  std::uint32_t offset = utils::read_u32_be(&data[base + 40]);
  return wad_entry{ name, csize, size, offset };
}

static
bool decompress_wad(const std::string& file_name) {
  std::cout << std::format("decompressing: {}..\n", file_name);
  try {
    auto data = utils::read_file(file_name);
    wad_header header = read_wad_header(data);
    if (header.magic != 0x543377AB) {
      utils::print_error("WAD has incorrect magic!\n");
      std::cout << std::format(
        "Expecting: 0x543377AB, got: 0x{:08X}",
          header.magic);
      return false;
    }

    std::cout << "\nwad information:\n";
    std::cout << std::format("magic: 0x{:08X}\n", header.magic);
    auto time = utils::format_timestamp(header.timestamp);
    if (time.has_value()) {
      std::cout << std::format(
        "timestamp: {} (0x{:08X})\n",
          time.value(),
          header.timestamp);
    } else {
      std::cout << "timestamp: N/A\n"; // nice 'N/A' string instead of garbage
    }
    std::cout << std::format("entries: {}\n", header.num_entries);
    std::cout << std::format("ffotd: {}\n\n", header.ffotd_version);

    std::vector<wad_entry> entries;
    for (std::uint32_t i = 0; i < header.num_entries; ++i) {
      entries.push_back(read_wad_entry(data, i));
    }
    // check if theres entries
    if (entries.empty()) {
      utils::print_warning(file_name + " has no valid entries\n");
      return false;
    }

    fs::path output_dir = fs::absolute(fs::path(file_name).stem());
    fs::create_directories(output_dir);

    for (const auto& entry : entries) {
      try {
        if (static_cast<std::size_t>(entry.offset) +
            static_cast<std::size_t>(entry.compressed_size) > data.size()) {
          throw std::runtime_error("data out of bounds");
        }

        std::vector<std::uint8_t> compressed_data(
          data.begin() + entry.offset,
          data.begin() + entry.offset + entry.compressed_size);

        fs::path output_path = output_dir / entry.name;
        std::ofstream out(output_path, std::ios::binary);
        if (!out) {
          throw std::runtime_error("failed to open file");
        }

        auto decompressed_data = utils::decompress_file(compressed_data);
        out.write(
          reinterpret_cast<const char*>(
            decompressed_data.data()),
            decompressed_data.size());
        std::cout << std::format("decompressed: {}\n", entry.name);
      } catch (const std::exception& e) {
        utils::print_error(e.what());
        return false;
      }
    }
  } catch (const std::exception& e) {
    utils::print_error(e.what());
    return false;
  }

  std::cout << "\ndone";
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
  std::cout << std::format("compressing: {}..\n\n", folder_name);
  try {
    wad_header header{};
    header.magic = 0x543377AB;
    header.timestamp = static_cast<std::uint32_t>(std::time(nullptr));
    // since post release is 1
    // so let's just set it to 1
    header.ffotd_version = 1; // it probably doesn't matter

    std::vector<wad_entry> entries;
    std::vector<std::vector<std::uint8_t>> compressed_datas;
    std::uint32_t current_offset = 16; // header size
    current_offset += 44 *
      static_cast<std::uint32_t>(
        std::distance(
          fs::directory_iterator(folder_name), {})); // entries table size

    for (const auto& entry : fs::directory_iterator(folder_name)) {
      if (!entry.is_regular_file()) {
        continue;
      }
      // do not nest a wad
      const auto& path = entry.path();
      if (path.extension() == ".wad") {
        utils::print_warning("skipping nested .wad: ", path, "\n");
        continue;
      }
      auto& file_path = entry.path();
      auto file_name = file_path.filename().string();
      if (file_name.size() > 42) {
        throw std::runtime_error("name too long: \n" + file_name);
      }
      // skip hidden files
      if (file_name.starts_with(".")) {
        utils::print_warning("skipping potentially hidden/system file: ", file_name, "\n");
        continue;
      }
      // check for bad characters
      // explorer already does this
      // so windows users shouldn't
      // ever encounter this problem
      for (char c : file_name) {
        if (c == '\\' ||
            c == '/' ||
            c == ':' ||
            c == '*' ||
            c == '?' ||
            c == '<' ||
            c == '>' ||
            c == '|') {
          throw std::runtime_error(
            std::format(
              "{} has atleast one bad character",
                file_name));
        }
      }

      auto file_data = utils::read_file(file_path);
      auto compressed_data = utils::compress_file(file_data);

      wad_entry we{};
      we.name = file_name;
      we.size = static_cast<std::uint32_t>(file_data.size());
      we.compressed_size = static_cast<std::uint32_t>(compressed_data.size());
      we.offset = current_offset;

      // let the user know what we compressed
      std::cout << std::format("compressed {}..\n", file_name);
      current_offset += we.compressed_size;
      entries.push_back(we);
      compressed_datas.push_back(std::move(compressed_data));
    }

    std::cout << "\nwad information:\n";
    std::cout << std::format("magic: 0x{:08X}\n", header.magic);
    auto time = utils::format_timestamp(header.timestamp);
    if (time.has_value()) {
      std::cout << std::format(
        "timestamp: {} (0x{:08X})\n",
          time.value(),
          header.timestamp);
    } else {
      std::cout << "timestamp: N/A\n"; // nice 'N/A' string instead of garbage
    }
    header.num_entries = static_cast<std::uint32_t>(entries.size());
    std::cout << std::format("entries: {}\n", header.num_entries);
    std::cout << std::format("ffotd: {}\n\n", header.ffotd_version);

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
    for (const auto& comp : compressed_datas) {
      wad_data.insert(wad_data.end(), comp.begin(), comp.end());
    }

    auto out_file = fs::path(folder_name).filename().string() + ".wad";
    utils::write_file(out_file, wad_data);
  } catch (const std::exception& e) {
    utils::print_error(e.what());
    return false;
  }

  std::cout << "done";
  return true;
}

static
void help() {
  // just general help for the tool
  std::cout << "command usages:\n\n";
  std::cout << "--decompress <input>  //  decompresses the input\n";
  std::cout << "  shortcut            // -d\n";
  std::cout << "--compress   <input>  //  compresses the input into a .wad\n";
  std::cout << "  shortcut            // -c\n";
  std::cout << "--help                //  displays help for various commands\n";
  std::cout << "  shortcut            // -h, -?\n";
  std::cout << "--about               //  displays about information\n";
  std::cout << "  shortcut            // -a\n";
}

static
void about() {
  std::cout << "tool information:\n\n";
  std::cout << "wadify.exe // a compresser/decompresser tool for 3arc's .wad type\n";
  std::cout << "           // made by hindercanrun\n";
}

int main(int argc, char* argv[]) {
  if (argc < 2) {
    utils::print_warning("usage: wadify.exe <cmd>");
    return 1;
  }

  // now check what the user wants to do
  std::string cmd = argv[1];
  if (cmd == "--decompress" ||
      cmd == "-d") {
    if (argc < 3) {
      utils::print_warning("usage: wadify.exe --decompress <input>");
      return 1;
    }
    std::string file = utils::add_wad_ext(argv[2]);
    if (!decompress_wad(file)) {
      return 1;
    }
    return 0;
  }
  else if (cmd == "--compress" ||
           cmd == "-c") {
    if (argc < 3) {
      utils::print_warning("usage: wadify.exe --compress <input>");
      return 1;
    }
    std::string dir = utils::remove_wad_ext(argv[2]);
    if (!compress_folder(dir)) {
        return 1;
    }
    return 0;
  }
  else if (cmd == "--help" ||
           cmd == "-h" ||
           cmd == "-?") {
    help();
    return 0;
  }
  else if (cmd == "--about" ||
           cmd == "-a") {
    about();
    return 0;
  }
  else {
    utils::print_warning("unknown cmd '", cmd, "'");
    return 1;
  }
  return 0;
}