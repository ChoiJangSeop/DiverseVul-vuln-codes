bool Webm2Pes::WritePesPacket(const VideoFrame& frame,
                              PacketDataBuffer* packet_data) {
  if (frame.buffer().data.get() == nullptr || frame.buffer().length < 1)
    return false;

  Ranges frame_ranges;
  if (frame.codec() == VideoFrame::kVP9) {
    const bool has_superframe_index = ParseVP9SuperFrameIndex(
        frame.buffer().data.get(), frame.buffer().length, &frame_ranges);
    if (has_superframe_index == false) {
      frame_ranges.push_back(Range(0, frame.buffer().length));
    }
  } else {
    frame_ranges.push_back(Range(0, frame.buffer().length));
  }

  const std::int64_t khz90_pts =
      NanosecondsTo90KhzTicks(frame.nanosecond_pts());
  PesHeader header;
  header.optional_header.SetPtsBits(khz90_pts);

  packet_data->clear();

  for (const Range& packet_payload_range : frame_ranges) {
    std::size_t extra_bytes = 0;
    if (packet_payload_range.length > kMaxPayloadSize) {
      extra_bytes = packet_payload_range.length - kMaxPayloadSize;
    }

    // First packet of new frame. Always include PTS and BCMV header.
    header.packet_length =
        packet_payload_range.length - extra_bytes + BCMVHeader::size();
    if (header.Write(true, packet_data) != true) {
      std::fprintf(stderr, "Webm2Pes: packet header write failed.\n");
      return false;
    }

    BCMVHeader bcmv_header(static_cast<uint32_t>(packet_payload_range.length));
    if (bcmv_header.Write(packet_data) != true) {
      std::fprintf(stderr, "Webm2Pes: BCMV write failed.\n");
      return false;
    }

    // Insert the payload at the end of |packet_data|.
    const std::uint8_t* const payload_start =
        frame.buffer().data.get() + packet_payload_range.offset;

    const std::size_t bytes_to_copy = packet_payload_range.length - extra_bytes;
    if (CopyAndEscapeStartCodes(payload_start, bytes_to_copy, packet_data) ==
        false) {
      fprintf(stderr, "Webm2Pes: Payload write failed.\n");
      return false;
    }

    std::size_t bytes_copied = bytes_to_copy;
    while (extra_bytes) {
      // Write PES packets for the remaining data, but omit the PTS and BCMV
      // header.
      const std::size_t extra_bytes_to_copy =
          std::min(kMaxPayloadSize, extra_bytes);
      extra_bytes -= extra_bytes_to_copy;
      header.packet_length = extra_bytes_to_copy;
      if (header.Write(false, packet_data) != true) {
        fprintf(stderr, "Webm2pes: fragment write failed.\n");
        return false;
      }

      const std::uint8_t* fragment_start = payload_start + bytes_copied;
      if (CopyAndEscapeStartCodes(fragment_start, extra_bytes_to_copy,
                                  packet_data) == false) {
        fprintf(stderr, "Webm2Pes: Payload write failed.\n");
        return false;
      }

      bytes_copied += extra_bytes_to_copy;
    }
  }

  return true;
}