static void WriteBinaryGltfStream(std::ostream &stream,
                                  const std::string &content,
                                  const std::vector<unsigned char> &binBuffer) {
  const std::string header = "glTF";
  const int version = 2;

  const uint32_t content_size = uint32_t(content.size());
  const uint32_t binBuffer_size = uint32_t(binBuffer.size());
  // determine number of padding bytes required to ensure 4 byte alignment
  const uint32_t content_padding_size = content_size % 4 == 0 ? 0 : 4 - content_size % 4;
  const uint32_t bin_padding_size = binBuffer_size % 4 == 0 ? 0 : 4 - binBuffer_size % 4;

  // 12 bytes for header, JSON content length, 8 bytes for JSON chunk info.
  // Chunk data must be located at 4-byte boundary, which may require padding
  const uint32_t length =
      12 + 8 + content_size + content_padding_size +
      (binBuffer_size ? (8 + binBuffer_size + bin_padding_size) : 0);

  stream.write(header.c_str(), std::streamsize(header.size()));
  stream.write(reinterpret_cast<const char *>(&version), sizeof(version));
  stream.write(reinterpret_cast<const char *>(&length), sizeof(length));

  // JSON chunk info, then JSON data
  const uint32_t model_length = uint32_t(content.size()) + content_padding_size;
  const uint32_t model_format = 0x4E4F534A;
  stream.write(reinterpret_cast<const char *>(&model_length),
               sizeof(model_length));
  stream.write(reinterpret_cast<const char *>(&model_format),
               sizeof(model_format));
  stream.write(content.c_str(), std::streamsize(content.size()));

  // Chunk must be multiplies of 4, so pad with spaces
  if (content_padding_size > 0) {
    const std::string padding = std::string(size_t(content_padding_size), ' ');
    stream.write(padding.c_str(), std::streamsize(padding.size()));
  }
  if (binBuffer.size() > 0) {
    // BIN chunk info, then BIN data
    const uint32_t bin_length = uint32_t(binBuffer.size()) + bin_padding_size;
    const uint32_t bin_format = 0x004e4942;
    stream.write(reinterpret_cast<const char *>(&bin_length),
                 sizeof(bin_length));
    stream.write(reinterpret_cast<const char *>(&bin_format),
                 sizeof(bin_format));
    stream.write(reinterpret_cast<const char *>(binBuffer.data()),
                 std::streamsize(binBuffer.size()));
    // Chunksize must be multiplies of 4, so pad with zeroes
    if (bin_padding_size > 0) {
      const std::vector<unsigned char> padding =
          std::vector<unsigned char>(size_t(bin_padding_size), 0);
      stream.write(reinterpret_cast<const char *>(padding.data()),
                   std::streamsize(padding.size()));
    }
  }
}