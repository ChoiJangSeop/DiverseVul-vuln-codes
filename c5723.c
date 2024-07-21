StatusWith<std::size_t> SnappyMessageCompressor::decompressData(ConstDataRange input,
                                                                DataRange output) {
    bool ret =
        snappy::RawUncompress(input.data(), input.length(), const_cast<char*>(output.data()));

    if (!ret) {
        return Status{ErrorCodes::BadValue, "Compressed message was invalid or corrupted"};
    }

    counterHitDecompress(input.length(), output.length());
    return output.length();
}