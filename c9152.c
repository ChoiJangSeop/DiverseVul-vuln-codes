void OwnedImpl::coalesceOrAddSlice(SlicePtr&& other_slice) {
  const uint64_t slice_size = other_slice->dataSize();
  // The `other_slice` content can be coalesced into the existing slice IFF:
  // 1. The `other_slice` can be coalesced. Objects of type UnownedSlice can not be coalesced. See
  //    comment in the UnownedSlice class definition;
  // 2. There are existing slices;
  // 3. The `other_slice` content length is under the CopyThreshold;
  // 4. There is enough unused space in the existing slice to accommodate the `other_slice` content.
  if (other_slice->canCoalesce() && !slices_.empty() && slice_size < CopyThreshold &&
      slices_.back()->reservableSize() >= slice_size) {
    // Copy content of the `other_slice`. The `move` methods which call this method effectively
    // drain the source buffer.
    addImpl(other_slice->data(), slice_size);
  } else {
    // Take ownership of the slice.
    slices_.emplace_back(std::move(other_slice));
    length_ += slice_size;
  }
}