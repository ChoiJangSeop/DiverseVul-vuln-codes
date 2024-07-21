const SegmentCommand* Binary::segment_from_offset(uint64_t offset) const {
  const auto it_begin = std::begin(offset_seg_);
  if (offset < it_begin->first) {
    return nullptr;
  }

  auto it = offset_seg_.lower_bound(offset);
  if (it->first == offset || it == it_begin) {
    SegmentCommand* seg = it->second;
    if (seg->file_offset() <= offset && offset < (seg->file_offset() + seg->file_size())) {
      return seg;
    }
  }

  const auto it_end = offset_seg_.crbegin();
  if (it == std::end(offset_seg_) && offset >= it_end->first) {
    SegmentCommand* seg = it_end->second;
    if (seg->file_offset() <= offset && offset < (seg->file_offset() + seg->file_size())) {
      return seg;
    }
  }

  if (it == it_begin) {
    /* It did not pass the first check */
    return nullptr;
  }

  --it;

  SegmentCommand* seg = it->second;
  if (seg->file_offset() <= offset && offset < (seg->file_offset() + seg->file_size())) {
    return seg;
  }
  return nullptr;
}