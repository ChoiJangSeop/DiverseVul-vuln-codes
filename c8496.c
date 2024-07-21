uint64 WriteBlock(IMkvWriter* writer, const Frame* const frame, int64 timecode,
                  uint64 timecode_scale) {
  uint64 block_additional_elem_size = 0;
  uint64 block_addid_elem_size = 0;
  uint64 block_more_payload_size = 0;
  uint64 block_more_elem_size = 0;
  uint64 block_additions_payload_size = 0;
  uint64 block_additions_elem_size = 0;
  if (frame->additional()) {
    block_additional_elem_size =
        EbmlElementSize(libwebm::kMkvBlockAdditional, frame->additional(),
                        frame->additional_length());
    block_addid_elem_size = EbmlElementSize(
        libwebm::kMkvBlockAddID, static_cast<uint64>(frame->add_id()));

    block_more_payload_size =
        block_addid_elem_size + block_additional_elem_size;
    block_more_elem_size =
        EbmlMasterElementSize(libwebm::kMkvBlockMore, block_more_payload_size) +
        block_more_payload_size;
    block_additions_payload_size = block_more_elem_size;
    block_additions_elem_size =
        EbmlMasterElementSize(libwebm::kMkvBlockAdditions,
                              block_additions_payload_size) +
        block_additions_payload_size;
  }

  uint64 discard_padding_elem_size = 0;
  if (frame->discard_padding() != 0) {
    discard_padding_elem_size =
        EbmlElementSize(libwebm::kMkvDiscardPadding,
                        static_cast<int64>(frame->discard_padding()));
  }

  const uint64 reference_block_timestamp =
      frame->reference_block_timestamp() / timecode_scale;
  uint64 reference_block_elem_size = 0;
  if (!frame->is_key()) {
    reference_block_elem_size =
        EbmlElementSize(libwebm::kMkvReferenceBlock, reference_block_timestamp);
  }

  const uint64 duration = frame->duration() / timecode_scale;
  uint64 block_duration_elem_size = 0;
  if (duration > 0)
    block_duration_elem_size =
        EbmlElementSize(libwebm::kMkvBlockDuration, duration);

  const uint64 block_payload_size = 4 + frame->length();
  const uint64 block_elem_size =
      EbmlMasterElementSize(libwebm::kMkvBlock, block_payload_size) +
      block_payload_size;

  const uint64 block_group_payload_size =
      block_elem_size + block_additions_elem_size + block_duration_elem_size +
      discard_padding_elem_size + reference_block_elem_size;

  if (!WriteEbmlMasterElement(writer, libwebm::kMkvBlockGroup,
                              block_group_payload_size)) {
    return 0;
  }

  if (!WriteEbmlMasterElement(writer, libwebm::kMkvBlock, block_payload_size))
    return 0;

  if (WriteUInt(writer, frame->track_number()))
    return 0;

  if (SerializeInt(writer, timecode, 2))
    return 0;

  // For a Block, flags is always 0.
  if (SerializeInt(writer, 0, 1))
    return 0;

  if (writer->Write(frame->frame(), static_cast<uint32>(frame->length())))
    return 0;

  if (frame->additional()) {
    if (!WriteEbmlMasterElement(writer, libwebm::kMkvBlockAdditions,
                                block_additions_payload_size)) {
      return 0;
    }

    if (!WriteEbmlMasterElement(writer, libwebm::kMkvBlockMore,
                                block_more_payload_size))
      return 0;

    if (!WriteEbmlElement(writer, libwebm::kMkvBlockAddID,
                          static_cast<uint64>(frame->add_id())))
      return 0;

    if (!WriteEbmlElement(writer, libwebm::kMkvBlockAdditional,
                          frame->additional(), frame->additional_length())) {
      return 0;
    }
  }

  if (frame->discard_padding() != 0 &&
      !WriteEbmlElement(writer, libwebm::kMkvDiscardPadding,
                        static_cast<int64>(frame->discard_padding()))) {
    return false;
  }

  if (!frame->is_key() &&
      !WriteEbmlElement(writer, libwebm::kMkvReferenceBlock,
                        reference_block_timestamp)) {
    return false;
  }

  if (duration > 0 &&
      !WriteEbmlElement(writer, libwebm::kMkvBlockDuration, duration)) {
    return false;
  }
  return EbmlMasterElementSize(libwebm::kMkvBlockGroup,
                               block_group_payload_size) +
         block_group_payload_size;
}