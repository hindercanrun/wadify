/*/
 *
 * tool to compress/decompress wad's
 * this was originally made for T6_greenlight_mp
 * however it also supports all versions of T6 and T5
 *
/*/

#include "dependencies/zlib/zlib.h"

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
std::vector<std::uint8_t> decompress_file(const std::vector<std::uint8_t>& compressed_data) {
  if (compressed_data.empty()) {
    throw std::invalid_argument("data is empty");
  }

  z_stream strm{};
  strm.next_in = const_cast<Bytef*>(compressed_data.data());
  strm.avail_in = static_cast<uInt>(compressed_data.size());
  if (inflateInit(&strm) != Z_OK) {
      throw std::runtime_error("inflateInit failed");
  }

  std::vector<std::uint8_t> result;
  std::vector<std::uint8_t> temp_buffer(64 * 1024); // 64 KB chunks

  int ret;
  do {
    strm.next_out = temp_buffer.data();
    strm.avail_out = static_cast<uInt>(temp_buffer.size());

    ret = inflate(&strm, Z_NO_FLUSH);
    if (ret != Z_OK && ret != Z_STREAM_END) {
      inflateEnd(&strm);
      throw std::runtime_error("inflate failed");
    }

    std::size_t produced = temp_buffer.size() - strm.avail_out;
    result.insert(result.end(), temp_buffer.begin(),
                  temp_buffer.begin() + produced);
  } while (ret != Z_STREAM_END);
  inflateEnd(&strm);
  return result;
}

static
void unlink_entries(const std::vector<wad_entry>& entries,
                    const std::vector<std::uint8_t>& data,
                    const fs::path& output_dir) {
  for (const auto& entry : entries) {
    try {
      if (static_cast<std::size_t>(entry.offset) +
          static_cast<std::size_t>(entry.compressed_size) > data.size()) {
        throw std::runtime_error("data is out of bounds");
      }

      std::vector<std::uint8_t>
        compressed_data(data.begin() + entry.offset,
                        data.begin() + entry.offset
                                     + entry.compressed_size);
      auto decompressed_data = decompress_file(compressed_data);

      fs::path output_path = output_dir / entry.name;
      std::ofstream out(output_path, std::ios::binary);
      if (!out) {
        throw std::runtime_error("failed to open output file");
      }
      out.write(reinterpret_cast<const char*>(decompressed_data.data()),
        decompressed_data.size());
      // tell the user what we decompressed
      std::cout << "decompressed: " << entry.name << "...\n";
    } catch (const std::exception& e) {
        std::cerr << "failed to unlink: " << entry.name << " :: " << e.what() << "!\n";
        return;
    }
  }
}

static
std::uint32_t read_u32_be(const std::uint8_t* ptr) {
  return (ptr[0] << 24) |
         (ptr[1] << 16) |
         (ptr[2] << 8)  | ptr[3];
}

static
wad_header read_wad_header(const std::vector<std::uint8_t>& data) {
  wad_header header;
  header.magic = read_u32_be(&data[0]);
  header.timestamp = read_u32_be(&data[4]);
  header.num_entries = read_u32_be(&data[8]);
  header.ffotd_version = read_u32_be(&data[12]);
  return header;
}

static
wad_entry read_wad_entry(const std::vector<std::uint8_t>& data, std::size_t index) {
  std::size_t base = 16 + index * 44;
  std::string name(reinterpret_cast<const char*>(&data[base]), 32);
  name = name.c_str(); // null-terminated cleanup

  std::uint32_t compressed_size = read_u32_be(&data[base + 32]);
  std::uint32_t size = read_u32_be(&data[base + 36]);
  std::uint32_t offset = read_u32_be(&data[base + 40]);

  return wad_entry{ name, compressed_size, size, offset };
}

static
std::vector<wad_entry> process_online_wad(const std::vector<std::uint8_t>& bytes) {
  wad_header header = read_wad_header(bytes);
  if (header.magic != 0x543377AB) {
    std::cerr << "WAD has incorrect magic!\n";
    std::cerr << "Expecting: 0x543377AB, got: 0x" << std::hex
                                                  << std::setw(8)
                                                  << std::setfill('0')
                                                  << header.magic
                                                  << "\n";
    return {};
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
  } else {
    std::cout << "timestamp: N/A\n"; // nice 'N/A' string instead of garbage
  }
  std::cout << "entries: " << std::dec << header.num_entries << "\n";
  std::cout << "ffotd: " << header.ffotd_version << "\n\n";

  std::vector<wad_entry> entries;
  for (std::uint32_t i = 0; i < header.num_entries; ++i) {
    entries.push_back(read_wad_entry(bytes, i));
  }
  return entries;
}

static
std::vector<std::uint8_t> read_file(const fs::path& path) {
  std::ifstream file(path, std::ios::binary);
  if (!file) {
    throw std::runtime_error("failed to open file");
  }
  return { std::istreambuf_iterator<char>(file), {} };
}

static
void decompress_wad(const std::string& file_name) {
  std::cout << "decompressing: " << file_name.c_str() << "..\n\n";
  try {
    auto data = read_file(file_name);
    auto entries = process_online_wad(data);
    if (entries.empty()) {
      std::cerr << file_name << " has no valid entries!\n";
      return;
    }

    fs::path output_dir = fs::absolute(fs::path(file_name).stem());
    fs::create_directories(output_dir);
    unlink_entries(entries, data, output_dir);
    std::cout << "\ndone!\n";
  } catch (const std::exception& e) {
      std::cerr << "failed to decompress: " << file_name << " -> " << e.what() << "!\n";
  }
}

static
void write_file(const fs::path& path,
    const std::vector<std::uint8_t>& data) {
    std::ofstream file(path, std::ios::binary);
    if (!file) {
        throw std::runtime_error("failed to write file");
    }
    file.write(reinterpret_cast<const char*>(data.data()), data.size());
}

static
std::vector<std::uint8_t> compress_file(const std::vector<std::uint8_t>& input_data,
                                        int compression_level = Z_BEST_COMPRESSION) {
  if (input_data.empty()) {
    throw std::invalid_argument("Input data is empty");
  }

  z_stream strm{};
  strm.next_in = const_cast<Bytef*>(input_data.data());
  strm.avail_in = static_cast<uInt>(input_data.size());
  if (deflateInit(&strm, compression_level) != Z_OK) {
    throw std::runtime_error("deflateInit failed");
  }

  std::vector<std::uint8_t> result;
  std::vector<std::uint8_t> temp_buffer(64 * 1024); // 64KB chunks

  int ret;
  do {
    strm.next_out = temp_buffer.data();
    strm.avail_out = static_cast<uInt>(temp_buffer.size());

    ret = deflate(&strm, strm.avail_in ? Z_NO_FLUSH : Z_FINISH);
    if (ret == Z_STREAM_ERROR) {
      deflateEnd(&strm);
      throw std::runtime_error("deflate failed");
    }

    std::size_t produced = temp_buffer.size() - strm.avail_out;
    result.insert(result.end(), temp_buffer.begin(), temp_buffer.begin() + produced);
  } while (ret != Z_STREAM_END);
  deflateEnd(&strm);
  return result;
}

static
void compress_folder(const std::string& folder_name) {
  std::cout << "compressing: " << folder_name << "..\n\n";
  try {
    wad_header header{};
    header.magic = 0x543377AB;
    header.timestamp = static_cast<std::uint32_t>(std::time(nullptr));
    header.ffotd_version = 0; // or whatever you want

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

      auto& file_path = entry.path();
      auto file_name = file_path.filename().string();
      if (file_name.size() > 42) {
        throw std::runtime_error("file name is too long: \n" + file_name);
      }

      auto file_data = read_file(file_path);
      auto compressed_data = compress_file(file_data);

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
    auto write_u32_be = [&](std::uint32_t value) {
      wad_data.push_back((value >> 24) & 0xFF);
      wad_data.push_back((value >> 16) & 0xFF);
      wad_data.push_back((value >> 8) & 0xFF);
      wad_data.push_back(value & 0xFF);
    };

    write_u32_be(header.magic);
    write_u32_be(header.timestamp);
    write_u32_be(header.num_entries);
    write_u32_be(header.ffotd_version);

    // write entries
    for (const auto& e : entries) {
      std::array<char, 32> name_buf{};
      std::memcpy(name_buf.data(), e.name.c_str(), e.name.size());
      wad_data.insert(wad_data.end(),
        reinterpret_cast<std::uint8_t*>(name_buf.data()),
        reinterpret_cast<std::uint8_t*>(name_buf.data()) + 32);

      write_u32_be(e.compressed_size);
      write_u32_be(e.size);
      write_u32_be(e.offset);
    }
    // write compressed data blocks
    for (const auto& comp : compressed_datas) {
      wad_data.insert(wad_data.end(), comp.begin(), comp.end());
    }

    auto out_file = fs::path(folder_name).filename().string() + ".wad";
    write_file(out_file, wad_data);
    std::cout << "\ndone!\n";
  } catch (const std::exception& e) {
      std::cerr << "failed to compress: " << folder_name << " -> " << e.what() << "!\n";
  }
}

static
void help() {
  // just general help for the tool
  std::cout << "command usages:\n\n";
  std::cout << "--decompress   <input .wad>   ::  decompresses the input .wad\n";
  std::cout << "  shortcut                    :: -d\n";
  std::cout << "--compress     <input folder> ::  compresses the input folder into a .wad\n";
  std::cout << "  shortcut                    :: -c\n";
  std::cout << "--help                        ::  displays help for various commands\n";
  std::cout << "  shortcut                    :: -h, -?\n";
  std::cout << "--about                       ::  displays about information\n";
  std::cout << "  shortcut                    :: -a\n";
}

static
void about() {
  std::cout << "tool information:\n\n";
  std::cout << "wadify.exe :: a compresser/decompresser tool for 3arc's .wad type\n";
  std::cout << "           :: made by hindercanrun\n";
}

int main(int argc, char* argv[]) {
  if (argc < 2) {
    std::cerr << "usage: wadify.exe <command>\n";
    return 1;
  }

  // now check what the user wants to do
  std::string cmd = argv[1];
  if (cmd == "--decompress" ||
      cmd == "-d") {
    if (argc < 3) {
      std::cerr << "usage: wadify.exe --decompress <input.wad>\n";
      return 1;
    }
    std::string file = argv[2];
    if (!file.ends_with(".wad")) {
      std::cerr << "tried to unlink a non .wad file\n";
      return 1;
    }
    // all good
    decompress_wad(file);
  }
  else if (cmd == "--compress" ||
           cmd == "-c") {
    if (argc < 3) {
      std::cerr << "usage: wadify.exe --compress <input folder>\n";
      return 1;
    }
    std::string dir = argv[2];
    if (dir.ends_with(".wad")) {
      std::cerr << "your input folder has .wad extension? check your command\n";
      return 1;
    }
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
    std::cerr << "unknown command '" << cmd << "'\n";
    return 1;
  }
  return 0;
}