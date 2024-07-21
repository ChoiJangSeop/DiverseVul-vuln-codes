StatusWith<std::size_t> SnappyMessageCompressor::compressData(ConstDataRange input,
                                                              DataRange output) {
    size_t outLength;
    snappy::RawCompress(input.data(), input.length(), const_cast<char*>(output.data()), &outLength);

    counterHitCompress(input.length(), outLength);
    return {outLength};
}