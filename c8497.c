bool Segment::DocTypeIsWebm() const {
  const int kNumCodecIds = 9;

  // TODO(vigneshv): Tweak .clang-format.
  const char* kWebmCodecIds[kNumCodecIds] = {
      Tracks::kOpusCodecId,          Tracks::kVorbisCodecId,
      Tracks::kVp8CodecId,           Tracks::kVp9CodecId,
      Tracks::kVp10CodecId,          Tracks::kWebVttCaptionsId,
      Tracks::kWebVttDescriptionsId, Tracks::kWebVttMetadataId,
      Tracks::kWebVttSubtitlesId};

  const int num_tracks = static_cast<int>(tracks_.track_entries_size());
  for (int track_index = 0; track_index < num_tracks; ++track_index) {
    const Track* const track = tracks_.GetTrackByIndex(track_index);
    const std::string codec_id = track->codec_id();

    bool id_is_webm = false;
    for (int id_index = 0; id_index < kNumCodecIds; ++id_index) {
      if (codec_id == kWebmCodecIds[id_index]) {
        id_is_webm = true;
        break;
      }
    }

    if (!id_is_webm)
      return false;
  }

  return true;
}