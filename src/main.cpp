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
void decompress_wad(const std::string& file_name) {
  std::cout << "decompressing: " << file_name << "..\n\n";
  try {
    auto data = utils::read_file(file_name);

    wad_header header = read_wad_header(data);
    if (header.magic != 0x543377AB) {
      std::cerr << "WAD has incorrect magic!\n";
      std::cerr << "Expecting: 0x543377AB, got: 0x"
                << std::hex << std::setw(8) << std::setfill('0')
                << header.magic << "\n";
      return;
    }

    // convert timestamp
    std::time_t t = header.timestamp;
    std::tm gmt = {};

    char time_buf[64]{};
    bool has_valid_time = (gmtime_s(&gmt, &t) == 0);
    if (has_valid_time) {
      std::strftime(time_buf, sizeof(time_buf), "%H:%M:%S, %d/%m/%Y", &gmt);
    }

    std::cout << "wad information:\n";
    std::cout << "magic: 0x" << std::hex << std::setw(8) << std::setfill('0') << header.magic << "\n";
    if (has_valid_time) {
      std::cout << "timestamp: " << time_buf << " (0x" << std::hex << header.timestamp << ")\n";
    }
    else {
      std::cout << "timestamp: N/A\n"; // nice 'N/A' string instead of garbage
    }
    std::cout << "entries: " << std::dec << header.num_entries << "\n";
    std::cout << "ffotd: " << header.ffotd_version << "\n\n";

    std::vector<wad_entry> entries;
    for (std::uint32_t i = 0; i < header.num_entries; ++i) {
      entries.push_back(read_wad_entry(data, i));
    }
    // check if theres entries
    if (entries.empty()) {
      std::cerr << file_name << " has no valid entries\n";
      return;
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
        out.write(reinterpret_cast<const char*>(decompressed_data.data()),
          decompressed_data.size());

        std::cout << "decompressed: " << entry.name << "...\n";
      } catch (const std::exception& e) {
        std::cerr << e.what() << "\n";
        return;
      }
    }
    std::cout << "\ndone\n";
  } catch (const std::exception& e) {
    std::cerr << e.what() << "\n";
  }
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
void compress_folder(const std::string& folder_name) {
  std::cout << "compressing: " << folder_name << "..\n\n";
  try {
    wad_header header{};
    header.magic = 0x543377AB;
    header.timestamp = static_cast<std::uint32_t>(std::time(nullptr));
    // since post release is 1
    // let's just set it to 1
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
        std::cout << "skipping nested .wad: " << path << "\n";
        continue;
      }
      auto& file_path = entry.path();
      auto file_name = file_path.filename().string();
      if (file_name.size() > 42) {
        throw std::runtime_error("name too long: \n" + file_name);
      }

      auto file_data = utils::read_file(file_path);
      auto compressed_data = utils::compress_file(file_data);

      wad_entry we{};
      we.name = file_name;
      we.size = static_cast<std::uint32_t>(file_data.size());
      we.compressed_size = static_cast<std::uint32_t>(compressed_data.size());
      we.offset = current_offset;

      // let the user know what we compressed
      std::cout << "compressed " << file_name.c_str() << "..\n";
      current_offset += we.compressed_size;
      entries.push_back(we);
      compressed_datas.push_back(std::move(compressed_data));
    }

    // convert timestamp
    std::time_t t = header.timestamp;
    std::tm gmt = {};

    char time_buf[64]{};
    bool has_valid_time = (gmtime_s(&gmt, &t) == 0);
    if (has_valid_time) {
      std::strftime(time_buf, sizeof(time_buf), "%H:%M:%S, %d/%m/%Y", &gmt);
    }

    std::cout << "\nwad information:\n";
    std::cout << "magic: 0x" << std::hex << std::setw(8) << std::setfill('0') << header.magic << "\n";
    if (has_valid_time) {
      std::cout << "timestamp: " << time_buf << " (0x" << std::hex << header.timestamp << ")\n";
    }
    else {
      std::cout << "timestamp: N/A\n"; // nice 'N/A' string instead of garbage
    }
    header.num_entries = static_cast<std::uint32_t>(entries.size());
    std::cout << "entries: " << std::dec << header.num_entries << "\n";
    std::cout << "ffotd: " << header.ffotd_version << "\n";

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
    std::cout << "\ndone\n";
  } catch (const std::exception& e) {
      std::cerr << e.what() << "\n";
  }
}

static
void help() {
  // just general help for the tool
  std::cout << "command usages:\n\n";
  std::cout << "--decompress   <input>   ::  decompresses the input\n";
  std::cout << "  shortcut               :: -d\n";
  std::cout << "--compress     <input>   ::  compresses the input into a .wad\n";
  std::cout << "  shortcut               :: -c\n";
  std::cout << "--help                   ::  displays help for various commands\n";
  std::cout << "  shortcut               :: -h, -?\n";
  std::cout << "--about                  ::  displays about information\n";
  std::cout << "  shortcut               :: -a\n";
}

static
void about() {
  std::cout << "tool information:\n\n";
  std::cout << "wadify.exe :: a compresser/decompresser tool for 3arc's .wad type\n";
  std::cout << "           :: made by hindercanrun\n";
}

int main(int argc, char* argv[]) {
  if (argc < 2) {
    std::cerr << "usage: wadify.exe <cmd>\n";
    return 1;
  }

  // now check what the user wants to do
  std::string cmd = argv[1];
  if (cmd == "--decompress" ||
      cmd == "-d") {
    if (argc < 3) {
      std::cerr << "usage: wadify.exe --decompress <input>\n";
      return 1;
    }
    std::string file = utils::add_wad_ext(argv[2]);
    // all good
    decompress_wad(file);
  }
  else if (cmd == "--compress" ||
           cmd == "-c") {
    if (argc < 3) {
      std::cerr << "usage: wadify.exe --compress <input>\n";
      return 1;
    }
    std::string dir = utils::remove_wad_ext(argv[2]);
    // all good
    compress_folder(dir);
  }
  else if (cmd == "--help" ||
           cmd == "-h" ||
           cmd == "-?") {
    help();
  }
  else if (cmd == "--about" ||
           cmd == "-a") {
    about();
  }
  else {
    std::cerr << "unknown cmd '" << cmd << "'\n";
    return 1;
  }
  return 0;
}