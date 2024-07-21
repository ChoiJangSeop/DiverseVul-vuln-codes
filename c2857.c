unsigned Utf8Decoder<kBufferSize>::WriteUtf16(uint16_t* data,
                                              unsigned length) const {
  DCHECK(length > 0);
  if (length > utf16_length_) length = utf16_length_;
  // memcpy everything in buffer.
  unsigned buffer_length =
      last_byte_of_buffer_unused_ ? kBufferSize - 1 : kBufferSize;
  unsigned memcpy_length = length <= buffer_length ? length : buffer_length;
  v8::internal::MemCopy(data, buffer_, memcpy_length * sizeof(uint16_t));
  if (length <= buffer_length) return length;
  DCHECK(unbuffered_start_ != NULL);
  // Copy the rest the slow way.
  WriteUtf16Slow(unbuffered_start_,
                 data + buffer_length,
                 length - buffer_length);
  return length;
}