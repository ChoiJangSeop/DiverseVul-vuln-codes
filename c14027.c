FontData::FontData(FontData* data, int32_t offset) {
  Init(data->array_);
  Bound(data->bound_offset_ + offset,
        (data->bound_length_ == GROWABLE_SIZE)
            ? GROWABLE_SIZE : data->bound_length_ - offset);
}