gst_rmdemux_parse_packet (GstRMDemux * rmdemux, GstBuffer * in, guint16 version)
{
  guint16 id;
  GstRMDemuxStream *stream;
  gsize size, offset;
  GstFlowReturn cret, ret;
  GstClockTime timestamp;
  gboolean key;
  GstMapInfo map;
  guint8 *data;
  guint8 flags;
  guint32 ts;

  gst_buffer_map (in, &map, GST_MAP_READ);
  data = map.data;
  size = map.size;

  /* stream number */
  id = RMDEMUX_GUINT16_GET (data);

  stream = gst_rmdemux_get_stream_by_id (rmdemux, id);
  if (!stream || !stream->pad)
    goto unknown_stream;

  /* timestamp in Msec */
  ts = RMDEMUX_GUINT32_GET (data + 2);
  timestamp = ts * GST_MSECOND;

  rmdemux->segment.position = timestamp;

  GST_LOG_OBJECT (rmdemux, "Parsing a packet for stream=%d, timestamp=%"
      GST_TIME_FORMAT ", size %" G_GSIZE_FORMAT ", version=%d, ts=%u", id,
      GST_TIME_ARGS (timestamp), size, version, ts);

  if (rmdemux->first_ts == GST_CLOCK_TIME_NONE) {
    GST_DEBUG_OBJECT (rmdemux, "First timestamp: %" GST_TIME_FORMAT,
        GST_TIME_ARGS (timestamp));
    rmdemux->first_ts = timestamp;
  }

  /* skip stream_id and timestamp */
  data += (2 + 4);
  size -= (2 + 4);

  /* get flags */
  flags = GST_READ_UINT8 (data + 1);

  data += 2;
  size -= 2;

  /* version 1 has an extra byte */
  if (version == 1) {
    data += 1;
    size -= 1;
  }
  offset = data - map.data;
  gst_buffer_unmap (in, &map);

  key = (flags & 0x02) != 0;
  GST_DEBUG_OBJECT (rmdemux, "flags %d, Keyframe %d", flags, key);

  if (rmdemux->need_newsegment) {
    GstEvent *event;

    event = gst_event_new_segment (&rmdemux->segment);

    GST_DEBUG_OBJECT (rmdemux, "sending NEWSEGMENT event, segment.start= %"
        GST_TIME_FORMAT, GST_TIME_ARGS (rmdemux->segment.start));

    gst_rmdemux_send_event (rmdemux, event);
    rmdemux->need_newsegment = FALSE;

    if (rmdemux->pending_tags != NULL) {
      gst_rmdemux_send_event (rmdemux,
          gst_event_new_tag (rmdemux->pending_tags));
      rmdemux->pending_tags = NULL;
    }
  }

  if (stream->pending_tags != NULL) {
    GST_LOG_OBJECT (stream->pad, "tags %" GST_PTR_FORMAT, stream->pending_tags);
    gst_pad_push_event (stream->pad, gst_event_new_tag (stream->pending_tags));
    stream->pending_tags = NULL;
  }

  if ((rmdemux->offset + size) <= stream->seek_offset) {
    GST_DEBUG_OBJECT (rmdemux,
        "Stream %d is skipping: seek_offset=%d, offset=%d, size=%"
        G_GSIZE_FORMAT, stream->id, stream->seek_offset, rmdemux->offset, size);
    cret = GST_FLOW_OK;
    gst_buffer_unref (in);
    goto beach;
  }

  /* do special headers */
  if (stream->subtype == GST_RMDEMUX_STREAM_VIDEO) {
    ret =
        gst_rmdemux_parse_video_packet (rmdemux, stream, in, offset,
        version, timestamp, key);
  } else if (stream->subtype == GST_RMDEMUX_STREAM_AUDIO) {
    ret =
        gst_rmdemux_parse_audio_packet (rmdemux, stream, in, offset,
        version, timestamp, key);
  } else {
    gst_buffer_unref (in);
    ret = GST_FLOW_OK;
  }

  cret = gst_flow_combiner_update_pad_flow (rmdemux->flowcombiner, stream->pad,
      ret);

beach:
  return cret;

  /* ERRORS */
unknown_stream:
  {
    GST_WARNING_OBJECT (rmdemux, "No stream for stream id %d in parsing "
        "data packet", id);
    gst_buffer_unmap (in, &map);
    gst_buffer_unref (in);
    return GST_FLOW_OK;
  }
}