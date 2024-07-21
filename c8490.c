uint64_t VideoTrack::VideoPayloadSize() const {
  uint64_t size = EbmlElementSize(
      libwebm::kMkvPixelWidth,
      static_cast<uint64>((pixel_width_ > 0) ? pixel_width_ : width_));
  size += EbmlElementSize(
      libwebm::kMkvPixelHeight,
      static_cast<uint64>((pixel_height_ > 0) ? pixel_height_ : height_));
  if (display_width_ > 0)
    size += EbmlElementSize(libwebm::kMkvDisplayWidth,
                            static_cast<uint64>(display_width_));
  if (display_height_ > 0)
    size += EbmlElementSize(libwebm::kMkvDisplayHeight,
                            static_cast<uint64>(display_height_));
  if (crop_left_ > 0)
    size += EbmlElementSize(libwebm::kMkvPixelCropLeft,
                            static_cast<uint64>(crop_left_));
  if (crop_right_ > 0)
    size += EbmlElementSize(libwebm::kMkvPixelCropRight,
                            static_cast<uint64>(crop_right_));
  if (crop_top_ > 0)
    size += EbmlElementSize(libwebm::kMkvPixelCropTop,
                            static_cast<uint64>(crop_top_));
  if (crop_bottom_ > 0)
    size += EbmlElementSize(libwebm::kMkvPixelCropBottom,
                            static_cast<uint64>(crop_bottom_));
  if (stereo_mode_ > kMono)
    size += EbmlElementSize(libwebm::kMkvStereoMode,
                            static_cast<uint64>(stereo_mode_));
  if (alpha_mode_ > kNoAlpha)
    size += EbmlElementSize(libwebm::kMkvAlphaMode,
                            static_cast<uint64>(alpha_mode_));
  if (frame_rate_ > 0.0)
    size += EbmlElementSize(libwebm::kMkvFrameRate,
                            static_cast<float>(frame_rate_));
  if (colour_)
    size += colour_->ColourSize();
  if (projection_)
    size += projection_->ProjectionSize();

  return size;
}