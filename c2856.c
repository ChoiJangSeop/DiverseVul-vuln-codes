void Utf8DecoderBase::Reset(uint16_t* buffer,
                            unsigned buffer_length,
                            const uint8_t* stream,
                            unsigned stream_length) {
  // Assume everything will fit in the buffer and stream won't be needed.
  last_byte_of_buffer_unused_ = false;
  unbuffered_start_ = NULL;
  bool writing_to_buffer = true;
  // Loop until stream is read, writing to buffer as long as buffer has space.
  unsigned utf16_length = 0;
  while (stream_length != 0) {
    unsigned cursor = 0;
    uint32_t character = Utf8::ValueOf(stream, stream_length, &cursor);
    DCHECK(cursor > 0 && cursor <= stream_length);
    stream += cursor;
    stream_length -= cursor;
    bool is_two_characters = character > Utf16::kMaxNonSurrogateCharCode;
    utf16_length += is_two_characters ? 2 : 1;
    // Don't need to write to the buffer, but still need utf16_length.
    if (!writing_to_buffer) continue;
    // Write out the characters to the buffer.
    // Must check for equality with buffer_length as we've already updated it.
    if (utf16_length <= buffer_length) {
      if (is_two_characters) {
        *buffer++ = Utf16::LeadSurrogate(character);
        *buffer++ = Utf16::TrailSurrogate(character);
      } else {
        *buffer++ = character;
      }
      if (utf16_length == buffer_length) {
        // Just wrote last character of buffer
        writing_to_buffer = false;
        unbuffered_start_ = stream;
      }
      continue;
    }
    // Have gone over buffer.
    // Last char of buffer is unused, set cursor back.
    DCHECK(is_two_characters);
    writing_to_buffer = false;
    last_byte_of_buffer_unused_ = true;
    unbuffered_start_ = stream - cursor;
  }
  utf16_length_ = utf16_length;
}