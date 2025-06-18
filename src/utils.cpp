// utility file
#include "utils.hpp"

namespace utils {
std::string add_wad_ext(const std::string& file) {
  if (!std::ranges::ends_with(file, ".wad")) {
    return file + ".wad";
  }
  return file;
}

std::string remove_wad_ext(const std::string& file) {
  if (std::ranges::ends_with(file, ".wad")) {
    return file.substr(0, file.size() - 4);
  }
  return file;
}

std::optional<std::string> get_out_folder(int argc, char* argv[]) {
  for (int i = 1; i < argc - 1; ++i) {
        if (std::string_view(argv[i]) == "--out") {
            return std::string(argv[i + 1]);
        }
    }
    return std::nullopt;
}

std::optional<std::string> format_timestamp(std::uint32_t timestamp) {
  std::time_t t = timestamp;
  std::tm local{};
  char buf[64]{};

  if (localtime_s(&local, &t) == 0) {
    std::strftime(buf, sizeof(buf), "%I:%M:%S %p, %d/%m/%Y", &local);
    std::string res = buf;
    std::ranges::transform(res, res.begin(),
      [](unsigned char c) { return std::toupper(c); });
    return res;
  }
  return std::nullopt; // fail
}

std::vector<std::uint8_t> read_file(const fs::path& path) {
  std::ifstream file(path, std::ios::binary);
  if (!file) {
    throw std::runtime_error("failed to open file");
  }
  return { std::istreambuf_iterator<char>(file), {} };
}

void write_file(const fs::path& path,
                const std::vector<std::uint8_t>& data) {
  std::ofstream file(path, std::ios::binary);
  if (!file) {
    throw std::runtime_error("failed to write file");
  }
  file.write(reinterpret_cast<const char*>(data.data()), data.size());
}

std::vector<std::uint8_t> decompress_file(const std::vector<std::uint8_t>& compressed_data) {
  if (compressed_data.empty()) {
    throw std::invalid_argument("Compressed data is empty");
  }

  z_stream strm{};
  strm.next_in = const_cast<Bytef*>(compressed_data.data());
  strm.avail_in = static_cast<uInt>(compressed_data.size());
  if (inflateInit(&strm) != Z_OK) {
    throw std::runtime_error("inflateInit failed");
  }

  std::vector<std::uint8_t> res;
  std::vector<std::uint8_t> temp(64 * 1024); // 64 KB chunks
  int ret;
  do {
    strm.next_out = temp.data();
    strm.avail_out = static_cast<uInt>(temp.size());

    ret = inflate(&strm, Z_NO_FLUSH);
    if (ret != Z_OK && ret != Z_STREAM_END) {
      inflateEnd(&strm);
      throw std::runtime_error("inflate failed");
    }

    auto produced = temp.size() - strm.avail_out;
    res.insert(res.end(), temp.begin(), temp.begin() + produced);
  } while (ret != Z_STREAM_END);
  inflateEnd(&strm);
  return res;
}

std::vector<std::uint8_t> compress_file(const std::vector<std::uint8_t>& input_data,
                                        int compression_level) {
  if (input_data.empty()) {
    throw std::invalid_argument("Input data is empty");
  }

  z_stream strm{};
  strm.next_in = const_cast<Bytef*>(input_data.data());
  strm.avail_in = static_cast<uInt>(input_data.size());
  if (deflateInit(&strm, compression_level) != Z_OK) {
    throw std::runtime_error("deflateInit failed");
  }

  std::vector<std::uint8_t> res;
  std::vector<std::uint8_t> temp(64 * 1024); // 64 KB chunks
  int ret;
  do {
    strm.next_out = temp.data();
    strm.avail_out = static_cast<uInt>(temp.size());

    ret = deflate(&strm, strm.avail_in ? Z_NO_FLUSH : Z_FINISH);
    if (ret == Z_STREAM_ERROR) {
      deflateEnd(&strm);
      throw std::runtime_error("deflate failed");
    }

    auto produced = temp.size() - strm.avail_out;
    res.insert(res.end(), temp.begin(), temp.begin() + produced);
  } while (ret != Z_STREAM_END);
  deflateEnd(&strm);
  return res;
}

std::uint32_t read_u32_be(const std::uint8_t* ptr) {
  return (static_cast<std::uint32_t>(ptr[0]) << 24) |
         (static_cast<std::uint32_t>(ptr[1]) << 16) |
         (static_cast<std::uint32_t>(ptr[2]) << 8)  |
          static_cast<std::uint32_t>(ptr[3]);
}

void write_u32_be(std::vector<std::uint8_t>& buf,
                  std::uint32_t value) {
  buf.push_back((value >> 24) & 0xFF);
  buf.push_back((value >> 16) & 0xFF);
  buf.push_back((value >> 8)  & 0xFF);
  buf.push_back(value         & 0xFF);
}
} // namespace utils