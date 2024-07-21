void* OwnedImpl::linearize(uint32_t size) {
  RELEASE_ASSERT(size <= length(), "Linearize size exceeds buffer size");
  if (slices_.empty()) {
    return nullptr;
  }
  uint64_t linearized_size = 0;
  uint64_t num_slices_to_linearize = 0;
  for (const auto& slice : slices_) {
    num_slices_to_linearize++;
    linearized_size += slice->dataSize();
    if (linearized_size >= size) {
      break;
    }
  }
  if (num_slices_to_linearize > 1) {
    auto new_slice = OwnedSlice::create(linearized_size);
    uint64_t bytes_copied = 0;
    Slice::Reservation reservation = new_slice->reserve(linearized_size);
    ASSERT(reservation.mem_ != nullptr);
    ASSERT(reservation.len_ == linearized_size);
    auto dest = static_cast<uint8_t*>(reservation.mem_);
    do {
      uint64_t data_size = slices_.front()->dataSize();
      memcpy(dest, slices_.front()->data(), data_size);
      bytes_copied += data_size;
      dest += data_size;
      slices_.pop_front();
    } while (bytes_copied < linearized_size);
    ASSERT(dest == static_cast<const uint8_t*>(reservation.mem_) + linearized_size);
    new_slice->commit(reservation);
    slices_.emplace_front(std::move(new_slice));
  }
  return slices_.front()->data();
}