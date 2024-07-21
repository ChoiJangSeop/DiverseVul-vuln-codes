const FieldID& activeUnionMemberId(const void* object, ptrdiff_t offset) {
  return *reinterpret_cast<const FieldID*>(
      offset + static_cast<const char*>(object));
}