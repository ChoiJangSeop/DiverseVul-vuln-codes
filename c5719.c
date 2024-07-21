std::size_t SnappyMessageCompressor::getMaxCompressedSize(size_t inputSize) {
    return snappy::MaxCompressedLength(inputSize);
}