bool Track::Write(IMkvWriter* writer) const {
  if (!writer)
    return false;

  // mandatory elements without a default value.
  if (!type_ || !codec_id_)
    return false;

  // |size| may be bigger than what is written out in this function because
  // derived classes may write out more data in the Track element.
  const uint64_t payload_size = PayloadSize();

  if (!WriteEbmlMasterElement(writer, libwebm::kMkvTrackEntry, payload_size))
    return false;

  uint64_t size =
      EbmlElementSize(libwebm::kMkvTrackNumber, static_cast<uint64>(number_));
  size += EbmlElementSize(libwebm::kMkvTrackUID, static_cast<uint64>(uid_));
  size += EbmlElementSize(libwebm::kMkvTrackType, static_cast<uint64>(type_));
  if (codec_id_)
    size += EbmlElementSize(libwebm::kMkvCodecID, codec_id_);
  if (codec_private_)
    size += EbmlElementSize(libwebm::kMkvCodecPrivate, codec_private_,
                            static_cast<uint64>(codec_private_length_));
  if (language_)
    size += EbmlElementSize(libwebm::kMkvLanguage, language_);
  if (name_)
    size += EbmlElementSize(libwebm::kMkvName, name_);
  if (max_block_additional_id_)
    size += EbmlElementSize(libwebm::kMkvMaxBlockAdditionID,
                            static_cast<uint64>(max_block_additional_id_));
  if (codec_delay_)
    size += EbmlElementSize(libwebm::kMkvCodecDelay,
                            static_cast<uint64>(codec_delay_));
  if (seek_pre_roll_)
    size += EbmlElementSize(libwebm::kMkvSeekPreRoll,
                            static_cast<uint64>(seek_pre_roll_));
  if (default_duration_)
    size += EbmlElementSize(libwebm::kMkvDefaultDuration,
                            static_cast<uint64>(default_duration_));

  const int64_t payload_position = writer->Position();
  if (payload_position < 0)
    return false;

  if (!WriteEbmlElement(writer, libwebm::kMkvTrackNumber,
                        static_cast<uint64>(number_)))
    return false;
  if (!WriteEbmlElement(writer, libwebm::kMkvTrackUID,
                        static_cast<uint64>(uid_)))
    return false;
  if (!WriteEbmlElement(writer, libwebm::kMkvTrackType,
                        static_cast<uint64>(type_)))
    return false;
  if (max_block_additional_id_) {
    if (!WriteEbmlElement(writer, libwebm::kMkvMaxBlockAdditionID,
                          static_cast<uint64>(max_block_additional_id_))) {
      return false;
    }
  }
  if (codec_delay_) {
    if (!WriteEbmlElement(writer, libwebm::kMkvCodecDelay,
                          static_cast<uint64>(codec_delay_)))
      return false;
  }
  if (seek_pre_roll_) {
    if (!WriteEbmlElement(writer, libwebm::kMkvSeekPreRoll,
                          static_cast<uint64>(seek_pre_roll_)))
      return false;
  }
  if (default_duration_) {
    if (!WriteEbmlElement(writer, libwebm::kMkvDefaultDuration,
                          static_cast<uint64>(default_duration_)))
      return false;
  }
  if (codec_id_) {
    if (!WriteEbmlElement(writer, libwebm::kMkvCodecID, codec_id_))
      return false;
  }
  if (codec_private_) {
    if (!WriteEbmlElement(writer, libwebm::kMkvCodecPrivate, codec_private_,
                          static_cast<uint64>(codec_private_length_)))
      return false;
  }
  if (language_) {
    if (!WriteEbmlElement(writer, libwebm::kMkvLanguage, language_))
      return false;
  }
  if (name_) {
    if (!WriteEbmlElement(writer, libwebm::kMkvName, name_))
      return false;
  }

  int64_t stop_position = writer->Position();
  if (stop_position < 0 ||
      stop_position - payload_position != static_cast<int64_t>(size))
    return false;

  if (content_encoding_entries_size_ > 0) {
    uint64_t content_encodings_size = 0;
    for (uint32_t i = 0; i < content_encoding_entries_size_; ++i) {
      ContentEncoding* const encoding = content_encoding_entries_[i];
      content_encodings_size += encoding->Size();
    }

    if (!WriteEbmlMasterElement(writer, libwebm::kMkvContentEncodings,
                                content_encodings_size))
      return false;

    for (uint32_t i = 0; i < content_encoding_entries_size_; ++i) {
      ContentEncoding* const encoding = content_encoding_entries_[i];
      if (!encoding->Write(writer))
        return false;
    }
  }

  stop_position = writer->Position();
  if (stop_position < 0)
    return false;
  return true;
}