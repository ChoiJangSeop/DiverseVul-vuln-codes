future<fragmented_temporary_buffer> cql_server::connection::read_and_decompress_frame(size_t length, uint8_t flags)
{
    using namespace compression_buffers;
    if (flags & cql_frame_flags::compression) {
        if (_compression == cql_compression::lz4) {
            if (length < 4) {
                throw std::runtime_error(fmt::format("CQL frame truncated: expected to have at least 4 bytes, got {}", length));
            }
            return _buffer_reader.read_exactly(_read_buf, length).then([this] (fragmented_temporary_buffer buf) {
                auto linearization_buffer = bytes_ostream();
                int32_t uncomp_len = request_reader(buf.get_istream(), linearization_buffer).read_int();
                if (uncomp_len < 0) {
                    throw std::runtime_error("CQL frame uncompressed length is negative: " + std::to_string(uncomp_len));
                }
                buf.remove_prefix(4);
                auto in = input_buffer.get_linearized_view(fragmented_temporary_buffer::view(buf));
                auto uncomp = output_buffer.make_fragmented_temporary_buffer(uncomp_len, fragmented_temporary_buffer::default_fragment_size, [&] (bytes_mutable_view out) {
                    auto ret = LZ4_decompress_safe(reinterpret_cast<const char*>(in.data()), reinterpret_cast<char*>(out.data()),
                                                   in.size(), out.size());
                    if (ret < 0) {
                        throw std::runtime_error("CQL frame LZ4 uncompression failure");
                    }
                    return out.size();
                });
                on_compression_buffer_use();
                return uncomp;
            });
        } else if (_compression == cql_compression::snappy) {
            return _buffer_reader.read_exactly(_read_buf, length).then([this] (fragmented_temporary_buffer buf) {
                auto in = input_buffer.get_linearized_view(fragmented_temporary_buffer::view(buf));
                size_t uncomp_len;
                if (snappy_uncompressed_length(reinterpret_cast<const char*>(in.data()), in.size(), &uncomp_len) != SNAPPY_OK) {
                    throw std::runtime_error("CQL frame Snappy uncompressed size is unknown");
                }
                auto uncomp = output_buffer.make_fragmented_temporary_buffer(uncomp_len, fragmented_temporary_buffer::default_fragment_size, [&] (bytes_mutable_view out) {
                    size_t output_len = out.size();
                    if (snappy_uncompress(reinterpret_cast<const char*>(in.data()), in.size(), reinterpret_cast<char*>(out.data()), &output_len) != SNAPPY_OK) {
                        throw std::runtime_error("CQL frame Snappy uncompression failure");
                    }
                    return output_len;
                });
                on_compression_buffer_use();
                return uncomp;
            });
        } else {
            throw exceptions::protocol_exception(format("Unknown compression algorithm"));
        }
    }
    return _buffer_reader.read_exactly(_read_buf, length);
}