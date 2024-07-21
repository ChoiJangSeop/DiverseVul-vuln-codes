gst_rmdemux_parse_audio_packet (GstRMDemux * rmdemux, GstRMDemuxStream * stream,
    GstBuffer * in, guint offset, guint16 version,
    GstClockTime timestamp, gboolean key)
{
  GstFlowReturn ret;
  GstBuffer *buffer;

  buffer = gst_buffer_copy_region (in, GST_BUFFER_COPY_MEMORY, offset, -1);

  if (rmdemux->first_ts != -1 && timestamp > rmdemux->first_ts)
    timestamp -= rmdemux->first_ts;
  else
    timestamp = 0;

  if (rmdemux->base_ts != -1)
    timestamp += rmdemux->base_ts;

  GST_BUFFER_PTS (buffer) = timestamp;
  GST_BUFFER_DTS (buffer) = timestamp;

  if (stream->needs_descrambling) {
    GST_LOG_OBJECT (rmdemux, "descramble timestamp %" GST_TIME_FORMAT,
        GST_TIME_ARGS (timestamp));
    ret = gst_rmdemux_handle_scrambled_packet (rmdemux, stream, buffer, key);
  } else {
    GST_LOG_OBJECT (rmdemux,
        "Pushing buffer of size %" G_GSIZE_FORMAT ", timestamp %"
        GST_TIME_FORMAT "to pad %s", gst_buffer_get_size (buffer),
        GST_TIME_ARGS (timestamp), GST_PAD_NAME (stream->pad));

    if (stream->discont) {
      GST_BUFFER_FLAG_SET (buffer, GST_BUFFER_FLAG_DISCONT);
      stream->discont = FALSE;
    }
    ret = gst_pad_push (stream->pad, buffer);
  }

  gst_buffer_unref (in);

  return ret;
}