bool FontData::Bound(int32_t offset, int32_t length) {
  if (offset + length > Size() || offset < 0 || length < 0)
    return false;

  bound_offset_ += offset;
  bound_length_ = length;
  return true;
}