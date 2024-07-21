Address Zone::NewExpand(int size) {
  // Make sure the requested size is already properly aligned and that
  // there isn't enough room in the Zone to satisfy the request.
  ASSERT(size == RoundDown(size, kAlignment));
  ASSERT(size > limit_ - position_);

  // Compute the new segment size. We use a 'high water mark'
  // strategy, where we increase the segment size every time we expand
  // except that we employ a maximum segment size when we delete. This
  // is to avoid excessive malloc() and free() overhead.
  Segment* head = segment_head_;
  int old_size = (head == NULL) ? 0 : head->size();
  static const int kSegmentOverhead = sizeof(Segment) + kAlignment;
  int new_size_no_overhead = size + (old_size << 1);
  int new_size = kSegmentOverhead + new_size_no_overhead;
  // Guard against integer overflow.
  if (new_size_no_overhead < size || new_size < kSegmentOverhead) {
    V8::FatalProcessOutOfMemory("Zone");
    return NULL;
  }
  if (new_size < kMinimumSegmentSize) {
    new_size = kMinimumSegmentSize;
  } else if (new_size > kMaximumSegmentSize) {
    // Limit the size of new segments to avoid growing the segment size
    // exponentially, thus putting pressure on contiguous virtual address space.
    // All the while making sure to allocate a segment large enough to hold the
    // requested size.
    new_size = Max(kSegmentOverhead + size, kMaximumSegmentSize);
  }
  Segment* segment = NewSegment(new_size);
  if (segment == NULL) {
    V8::FatalProcessOutOfMemory("Zone");
    return NULL;
  }

  // Recompute 'top' and 'limit' based on the new segment.
  Address result = RoundUp(segment->start(), kAlignment);
  position_ = result + size;
  // Check for address overflow.
  if (position_ < result) {
    V8::FatalProcessOutOfMemory("Zone");
    return NULL;
  }
  limit_ = segment->end();
  ASSERT(position_ <= limit_);
  return result;
}