bool VideoTrack::Write(IMkvWriter* writer) const {
  if (!Track::Write(writer))
    return false;

  const uint64_t size = VideoPayloadSize();

  if (!WriteEbmlMasterElement(writer, libwebm::kMkvVideo, size))
    return false;

  const int64_t payload_position = writer->Position();
  if (payload_position < 0)
    return false;

  if (!WriteEbmlElement(
          writer, libwebm::kMkvPixelWidth,
          static_cast<uint64>((pixel_width_ > 0) ? pixel_width_ : width_)))
    return false;
  if (!WriteEbmlElement(
          writer, libwebm::kMkvPixelHeight,
          static_cast<uint64>((pixel_height_ > 0) ? pixel_height_ : height_)))
    return false;
  if (display_width_ > 0) {
    if (!WriteEbmlElement(writer, libwebm::kMkvDisplayWidth,
                          static_cast<uint64>(display_width_)))
      return false;
  }
  if (display_height_ > 0) {
    if (!WriteEbmlElement(writer, libwebm::kMkvDisplayHeight,
                          static_cast<uint64>(display_height_)))
      return false;
  }
  if (crop_left_ > 0) {
    if (!WriteEbmlElement(writer, libwebm::kMkvPixelCropLeft,
                          static_cast<uint64>(crop_left_)))
      return false;
  }
  if (crop_right_ > 0) {
    if (!WriteEbmlElement(writer, libwebm::kMkvPixelCropRight,
                          static_cast<uint64>(crop_right_)))
      return false;
  }
  if (crop_top_ > 0) {
    if (!WriteEbmlElement(writer, libwebm::kMkvPixelCropTop,
                          static_cast<uint64>(crop_top_)))
      return false;
  }
  if (crop_bottom_ > 0) {
    if (!WriteEbmlElement(writer, libwebm::kMkvPixelCropBottom,
                          static_cast<uint64>(crop_bottom_)))
      return false;
  }
  if (stereo_mode_ > kMono) {
    if (!WriteEbmlElement(writer, libwebm::kMkvStereoMode,
                          static_cast<uint64>(stereo_mode_)))
      return false;
  }
  if (alpha_mode_ > kNoAlpha) {
    if (!WriteEbmlElement(writer, libwebm::kMkvAlphaMode,
                          static_cast<uint64>(alpha_mode_)))
      return false;
  }
  if (frame_rate_ > 0.0) {
    if (!WriteEbmlElement(writer, libwebm::kMkvFrameRate,
                          static_cast<float>(frame_rate_))) {
      return false;
    }
  }
  if (colour_) {
    if (!colour_->Write(writer))
      return false;
  }
  if (projection_) {
    if (!projection_->Write(writer))
      return false;
  }

  const int64_t stop_position = writer->Position();
  if (stop_position < 0 ||
      stop_position - payload_position != static_cast<int64_t>(size)) {
    return false;
  }

  return true;
}