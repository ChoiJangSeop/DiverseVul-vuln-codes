uint64 WriteFrame(IMkvWriter* writer, const Frame* const frame,
                  Cluster* cluster) {
  if (!writer || !frame || !frame->IsValid() || !cluster ||
      !cluster->timecode_scale())
    return 0;

  //  Technically the timecode for a block can be less than the
  //  timecode for the cluster itself (remember that block timecode
  //  is a signed, 16-bit integer).  However, as a simplification we
  //  only permit non-negative cluster-relative timecodes for blocks.
  const int64 relative_timecode = cluster->GetRelativeTimecode(
      frame->timestamp() / cluster->timecode_scale());
  if (relative_timecode < 0 || relative_timecode > kMaxBlockTimecode)
    return 0;

  return frame->CanBeSimpleBlock() ?
             WriteSimpleBlock(writer, frame, relative_timecode) :
             WriteBlock(writer, frame, relative_timecode,
                        cluster->timecode_scale());
}