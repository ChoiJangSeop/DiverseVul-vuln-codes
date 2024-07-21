bounded_iostream::bounded_iostream(Sirikata::DecoderWriter *w,
                                   const std::function<void(Sirikata::DecoderWriter*, size_t)> &size_callback,
                                   const Sirikata::JpegAllocator<uint8_t> &alloc) 
    : parent(w), err(Sirikata::JpegError::nil()) {
    this->size_callback = size_callback;
    buffer_position = 0;
    byte_position = 0;
    num_bytes_attempted_to_write = 0;
    set_bound(0);
}