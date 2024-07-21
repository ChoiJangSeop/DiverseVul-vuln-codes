unsigned int bounded_iostream::write_no_buffer(const void *from, size_t bytes_to_write) {
    //return iostream::write(from,tpsize,dtsize);
    std::pair<unsigned int, Sirikata::JpegError> retval;
    if (byte_bound != 0 && byte_position + bytes_to_write > byte_bound) {
        size_t real_bytes_to_write = byte_bound - byte_position;
        byte_position += real_bytes_to_write;
        retval = parent->Write(reinterpret_cast<const unsigned char*>(from), real_bytes_to_write);
        if (retval.first < real_bytes_to_write) {
            err = retval.second;
            return retval.first;
        }
        return bytes_to_write; // pretend we wrote it all
    }
    size_t total = bytes_to_write;
    retval = parent->Write(reinterpret_cast<const unsigned char*>(from), total);
    unsigned int written = retval.first;
    byte_position += written;
    if (written < total ) {
        err = retval.second;
        return written;
    }
    return bytes_to_write;
}