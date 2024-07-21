gst_rmdemux_parse_video_packet (GstRMDemux * rmdemux, GstRMDemuxStream * stream,
    GstBuffer * in, guint offset, guint16 version,
    GstClockTime timestamp, gboolean key)
{
  GstFlowReturn ret;
  GstMapInfo map;
  const guint8 *data;
  gsize size;

  gst_buffer_map (in, &map, GST_MAP_READ);

  data = map.data + offset;
  size = map.size - offset;

  /* if size <= 2, we want this method to return the same GstFlowReturn as it
   * was previously for that given stream. */
  ret = GST_PAD_LAST_FLOW_RETURN (stream->pad);

  while (size > 2) {
    guint8 pkg_header;
    guint pkg_offset;
    guint pkg_length;
    guint pkg_subseq = 0, pkg_seqnum = G_MAXUINT;
    guint fragment_size;
    GstBuffer *fragment;

    pkg_header = *data++;
    size--;

    /* packet header
     * bit 7: 1=last block in block chain
     * bit 6: 1=short header (only one block?)
     */
    if ((pkg_header & 0xc0) == 0x40) {
      /* skip unknown byte */
      data++;
      size--;
      pkg_offset = 0;
      pkg_length = size;
    } else {
      if ((pkg_header & 0x40) == 0) {
        pkg_subseq = (*data++) & 0x7f;
        size--;
      } else {
        pkg_subseq = 0;
      }

      /* length */
      PARSE_NUMBER (data, size, pkg_length, not_enough_data);

      /* offset */
      PARSE_NUMBER (data, size, pkg_offset, not_enough_data);

      /* seqnum */
      if (size < 1)
        goto not_enough_data;

      pkg_seqnum = *data++;
      size--;
    }

    GST_DEBUG_OBJECT (rmdemux,
        "seq %d, subseq %d, offset %d, length %d, size %" G_GSIZE_FORMAT
        ", header %02x", pkg_seqnum, pkg_subseq, pkg_offset, pkg_length, size,
        pkg_header);

    /* calc size of fragment */
    if ((pkg_header & 0xc0) == 0x80) {
      fragment_size = pkg_offset;
    } else {
      if ((pkg_header & 0xc0) == 0)
        fragment_size = size;
      else
        fragment_size = pkg_length;
    }
    GST_DEBUG_OBJECT (rmdemux, "fragment size %d", fragment_size);

    /* get the fragment */
    fragment =
        gst_buffer_copy_region (in, GST_BUFFER_COPY_ALL, data - map.data,
        fragment_size);

    if (pkg_subseq == 1) {
      GST_DEBUG_OBJECT (rmdemux, "start new fragment");
      gst_adapter_clear (stream->adapter);
      stream->frag_current = 0;
      stream->frag_count = 0;
      stream->frag_length = pkg_length;
    } else if (pkg_subseq == 0) {
      GST_DEBUG_OBJECT (rmdemux, "non fragmented packet");
      stream->frag_current = 0;
      stream->frag_count = 0;
      stream->frag_length = fragment_size;
    }

    /* put fragment in adapter */
    gst_adapter_push (stream->adapter, fragment);
    stream->frag_offset[stream->frag_count] = stream->frag_current;
    stream->frag_current += fragment_size;
    stream->frag_count++;

    if (stream->frag_count > MAX_FRAGS)
      goto too_many_fragments;

    GST_DEBUG_OBJECT (rmdemux, "stored fragment in adapter %d/%d",
        stream->frag_current, stream->frag_length);

    /* flush fragment when complete */
    if (stream->frag_current >= stream->frag_length) {
      GstBuffer *out;
      GstMapInfo outmap;
      guint8 *outdata;
      guint header_size;
      gint i, avail;

      /* calculate header size, which is:
       * 1 byte for the number of fragments - 1
       * for each fragment:
       *   4 bytes 0x00000001 little endian
       *   4 bytes fragment offset
       *
       * This is also the matroska header for realvideo, the decoder needs the
       * fragment offsets, both in ffmpeg and real .so, so we just give it that
       * in front of the data.
       */
      header_size = 1 + (8 * (stream->frag_count));

      GST_DEBUG_OBJECT (rmdemux,
          "fragmented completed. count %d, header_size %u", stream->frag_count,
          header_size);

      avail = gst_adapter_available (stream->adapter);

      out = gst_buffer_new_and_alloc (header_size + avail);
      gst_buffer_map (out, &outmap, GST_MAP_WRITE);
      outdata = outmap.data;

      /* create header */
      *outdata++ = stream->frag_count - 1;
      for (i = 0; i < stream->frag_count; i++) {
        GST_WRITE_UINT32_LE (outdata, 0x00000001);
        outdata += 4;
        GST_WRITE_UINT32_LE (outdata, stream->frag_offset[i]);
        outdata += 4;
      }

      /* copy packet data after the header now */
      gst_adapter_copy (stream->adapter, outdata, 0, avail);
      gst_adapter_flush (stream->adapter, avail);

      stream->frag_current = 0;
      stream->frag_count = 0;
      stream->frag_length = 0;

      if (timestamp != -1) {
        if (rmdemux->first_ts != -1 && timestamp > rmdemux->first_ts)
          timestamp -= rmdemux->first_ts;
        else
          timestamp = 0;

        if (rmdemux->base_ts != -1)
          timestamp += rmdemux->base_ts;
      }
      gst_buffer_unmap (out, &outmap);

      /* video has DTS */
      GST_BUFFER_DTS (out) = timestamp;
      GST_BUFFER_PTS (out) = GST_CLOCK_TIME_NONE;

      GST_LOG_OBJECT (rmdemux, "pushing timestamp %" GST_TIME_FORMAT,
          GST_TIME_ARGS (timestamp));

      if (stream->discont) {
        GST_BUFFER_FLAG_SET (out, GST_BUFFER_FLAG_DISCONT);
        stream->discont = FALSE;
      }

      if (!key) {
        GST_BUFFER_FLAG_SET (out, GST_BUFFER_FLAG_DELTA_UNIT);
      }

      ret = gst_pad_push (stream->pad, out);
      ret = gst_flow_combiner_update_flow (rmdemux->flowcombiner, ret);
      if (ret != GST_FLOW_OK)
        break;

      timestamp = GST_CLOCK_TIME_NONE;
    }
    data += fragment_size;
    size -= fragment_size;
  }
  GST_DEBUG_OBJECT (rmdemux, "%" G_GSIZE_FORMAT " bytes left", size);

done:
  gst_buffer_unmap (in, &map);
  gst_buffer_unref (in);

  return ret;

  /* ERRORS */
not_enough_data:
  {
    GST_ELEMENT_WARNING (rmdemux, STREAM, DECODE, ("Skipping bad packet."),
        (NULL));
    ret = GST_FLOW_OK;
    goto done;
  }
too_many_fragments:
  {
    GST_ELEMENT_ERROR (rmdemux, STREAM, DECODE,
        ("Got more fragments (%u) than can be handled (%u)",
            stream->frag_count, MAX_FRAGS), (NULL));
    ret = GST_FLOW_ERROR;
    goto done;
  }
}