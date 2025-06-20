// utility file
#include "utils.hpp"

namespace utils {
std::string add_wad_ext(const std::string& file) {
  using namespace std::string_view_literals;

  const auto sv = std::string_view(file);
  if (sv.size() >= 4 && 
      std::equal(sv.end() - 4, sv.end(), ".wad"sv.begin(), 
        [](char a, char b) { return std::tolower(a) == std::tolower(b); })) {
    return file;
  }
  return file + ".wad";
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

bool write_file(const fs::path& path,
                const std::vector<std::uint8_t>& data) {
  std::ofstream file(path, std::ios::binary);
  if (!file.is_open()) {
    print_err("failed to open: '{}'", path.string());
    return false;
  }
  file.write(reinterpret_cast<const char*>(data.data()), data.size());
  if (!file) {
    print_err("failed to write: '{}'", path.string());
    return false;
  }
  return true;
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

} // namespace utils