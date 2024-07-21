bool FontData::Bound(int32_t offset) {
if (offset > Size() || offset < 0)
    return false;

  bound_offset_ += offset;
  return true;
}