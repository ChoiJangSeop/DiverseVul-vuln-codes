bool MasteringMetadata::Write(IMkvWriter* writer) const {
  const uint64_t size = PayloadSize();

  // Don't write an empty element.
  if (size == 0)
    return true;

  if (!WriteEbmlMasterElement(writer, libwebm::kMkvMasteringMetadata, size))
    return false;
  if (luminance_max_ != kValueNotPresent &&
      !WriteEbmlElement(writer, libwebm::kMkvLuminanceMax, luminance_max_)) {
    return false;
  }
  if (luminance_min_ != kValueNotPresent &&
      !WriteEbmlElement(writer, libwebm::kMkvLuminanceMin, luminance_min_)) {
    return false;
  }
  if (r_ &&
      !r_->Write(writer, libwebm::kMkvPrimaryRChromaticityX,
                 libwebm::kMkvPrimaryRChromaticityY)) {
    return false;
  }
  if (g_ &&
      !g_->Write(writer, libwebm::kMkvPrimaryGChromaticityX,
                 libwebm::kMkvPrimaryGChromaticityY)) {
    return false;
  }
  if (b_ &&
      !b_->Write(writer, libwebm::kMkvPrimaryBChromaticityX,
                 libwebm::kMkvPrimaryBChromaticityY)) {
    return false;
  }
  if (white_point_ &&
      !white_point_->Write(writer, libwebm::kMkvWhitePointChromaticityX,
                           libwebm::kMkvWhitePointChromaticityY)) {
    return false;
  }

  return true;
}