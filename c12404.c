void HeaderTable::setCapacity(uint32_t capacity) {
  auto oldCapacity = capacity_;
  capacity_ = capacity;
  if (capacity_ <= oldCapacity) {
    evict(0);
  } else {
    auto oldTail = tail();
    auto oldLength = table_.size();
    uint32_t newLength = (capacity_ >> 5) + 1;
    table_.resize(newLength);
    if (size_ > 0 && oldTail > head_) {
      // the list wrapped around, need to move oldTail..oldLength to the end of
      // the now-larger table_
      std::copy(table_.begin() + oldTail, table_.begin() + oldLength,
                table_.begin() + newLength - (oldLength - oldTail));
      // Update the names indecies that pointed to the old range
      for (auto& names_it: names_) {
        for (auto& idx: names_it.second) {
          if (idx >= oldTail) {
            DCHECK_LT(idx + (table_.size() - oldLength), table_.size());
            idx += (table_.size() - oldLength);
          } else {
            // remaining indecies in the list were smaller than oldTail, so
            // should be indexed from 0
            break;
          }
        }
      }
    }
  }
}