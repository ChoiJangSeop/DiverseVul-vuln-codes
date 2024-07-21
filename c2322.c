HttpTransact::handle_trace_and_options_requests(State* s, HTTPHdr* incoming_hdr)
{
  ink_assert(incoming_hdr->type_get() == HTTP_TYPE_REQUEST);

  // This only applies to TRACE and OPTIONS
  if ((s->method != HTTP_WKSIDX_TRACE) && (s->method != HTTP_WKSIDX_OPTIONS))
    return false;

  // If there is no Max-Forwards request header, just return false.
  if (!incoming_hdr->presence(MIME_PRESENCE_MAX_FORWARDS)) {
    // Trace and Options requests should not be looked up in cache.
    // s->cache_info.action = CACHE_DO_NO_ACTION;
    s->current.mode = TUNNELLING_PROXY;
    HTTP_INCREMENT_TRANS_STAT(http_tunnels_stat);
    return false;
  }

  int max_forwards = incoming_hdr->get_max_forwards();
  if (max_forwards <= 0) {
    //////////////////////////////////////////////
    // if max-forward is 0 the request must not //
    // be forwarded to the origin server.       //
    //////////////////////////////////////////////
    DebugTxn("http_trans", "[handle_trace] max-forwards: 0, building response...");
    SET_VIA_STRING(VIA_DETAIL_TUNNEL, VIA_DETAIL_TUNNEL_NO_FORWARD);
    build_response(s, &s->hdr_info.client_response, s->client_info.http_version, HTTP_STATUS_OK);

    ////////////////////////////////////////
    // if method is trace we should write //
    // the request header as the body.    //
    ////////////////////////////////////////
    if (s->method == HTTP_WKSIDX_TRACE) {
      DebugTxn("http_trans", "[handle_trace] inserting request in body.");
      int req_length = incoming_hdr->length_get();
      HTTP_RELEASE_ASSERT(req_length > 0);

      s->internal_msg_buffer_index = 0;
      s->internal_msg_buffer_size = req_length * 2;
      s->free_internal_msg_buffer();

      if (s->internal_msg_buffer_size <= max_iobuffer_size) {
        s->internal_msg_buffer_fast_allocator_size = buffer_size_to_index(s->internal_msg_buffer_size);
        s->internal_msg_buffer = (char *) ioBufAllocator[s->internal_msg_buffer_fast_allocator_size].alloc_void();
      } else {
        s->internal_msg_buffer_fast_allocator_size = -1;
        s->internal_msg_buffer = (char *)ats_malloc(s->internal_msg_buffer_size);
      }

      // clear the stupid buffer
      memset(s->internal_msg_buffer, '\0', s->internal_msg_buffer_size);

      int offset = 0;
      int used = 0;
      int done;
      done = incoming_hdr->print(s->internal_msg_buffer, s->internal_msg_buffer_size, &used, &offset);
      HTTP_RELEASE_ASSERT(done);
      s->internal_msg_buffer_size = used;

      s->hdr_info.client_response.set_content_length(used);
    } else {
      // For OPTIONS request insert supported methods in ALLOW field
      DebugTxn("http_trans", "[handle_options] inserting methods in Allow.");
      HttpTransactHeaders::insert_supported_methods_in_response(&s->hdr_info.client_response, s->scheme);

    }
    return true;
  } else {                      /* max-forwards != 0 */

    if ((max_forwards <= 0) || (max_forwards > INT_MAX)) {
      Log::error("HTTP: snapping invalid max-forwards value %d to %d", max_forwards, INT_MAX);
      max_forwards = INT_MAX;
    }

    --max_forwards;
    DebugTxn("http_trans", "[handle_trace_options] Decrementing max_forwards to %d", max_forwards);
    incoming_hdr->set_max_forwards(max_forwards);

    // Trace and Options requests should not be looked up in cache.
    // s->cache_info.action = CACHE_DO_NO_ACTION;
    s->current.mode = TUNNELLING_PROXY;
    HTTP_INCREMENT_TRANS_STAT(http_tunnels_stat);
  }

  return false;
}