SegmentCommand* Binary::segment_from_offset(uint64_t offset) {
  return const_cast<SegmentCommand*>(static_cast<const Binary*>(this)->segment_from_offset(offset));
}