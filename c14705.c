static int http_receive_data(HTTPContext *c)
{
    HTTPContext *c1;
    int len, loop_run = 0;

    while (c->chunked_encoding && !c->chunk_size &&
           c->buffer_end > c->buffer_ptr) {
        /* read chunk header, if present */
        len = recv(c->fd, c->buffer_ptr, 1, 0);

        if (len < 0) {
            if (ff_neterrno() != AVERROR(EAGAIN) &&
                ff_neterrno() != AVERROR(EINTR))
                /* error : close connection */
                goto fail;
            return 0;
        } else if (len == 0) {
            /* end of connection : close it */
            goto fail;
        } else if (c->buffer_ptr - c->buffer >= 2 &&
                   !memcmp(c->buffer_ptr - 1, "\r\n", 2)) {
            c->chunk_size = strtol(c->buffer, 0, 16);
            if (c->chunk_size == 0) // end of stream
                goto fail;
            c->buffer_ptr = c->buffer;
            break;
        } else if (++loop_run > 10)
            /* no chunk header, abort */
            goto fail;
        else
            c->buffer_ptr++;
    }

    if (c->buffer_end > c->buffer_ptr) {
        len = recv(c->fd, c->buffer_ptr,
                   FFMIN(c->chunk_size, c->buffer_end - c->buffer_ptr), 0);
        if (len < 0) {
            if (ff_neterrno() != AVERROR(EAGAIN) &&
                ff_neterrno() != AVERROR(EINTR))
                /* error : close connection */
                goto fail;
        } else if (len == 0)
            /* end of connection : close it */
            goto fail;
        else {
            c->chunk_size -= len;
            c->buffer_ptr += len;
            c->data_count += len;
            update_datarate(&c->datarate, c->data_count);
        }
    }

    if (c->buffer_ptr - c->buffer >= 2 && c->data_count > FFM_PACKET_SIZE) {
        if (c->buffer[0] != 'f' ||
            c->buffer[1] != 'm') {
            http_log("Feed stream has become desynchronized -- disconnecting\n");
            goto fail;
        }
    }

    if (c->buffer_ptr >= c->buffer_end) {
        FFServerStream *feed = c->stream;
        /* a packet has been received : write it in the store, except
         * if header */
        if (c->data_count > FFM_PACKET_SIZE) {
            /* XXX: use llseek or url_seek
             * XXX: Should probably fail? */
            if (lseek(c->feed_fd, feed->feed_write_index, SEEK_SET) == -1)
                http_log("Seek to %"PRId64" failed\n", feed->feed_write_index);

            if (write(c->feed_fd, c->buffer, FFM_PACKET_SIZE) < 0) {
                http_log("Error writing to feed file: %s\n", strerror(errno));
                goto fail;
            }

            feed->feed_write_index += FFM_PACKET_SIZE;
            /* update file size */
            if (feed->feed_write_index > c->stream->feed_size)
                feed->feed_size = feed->feed_write_index;

            /* handle wrap around if max file size reached */
            if (c->stream->feed_max_size &&
                feed->feed_write_index >= c->stream->feed_max_size)
                feed->feed_write_index = FFM_PACKET_SIZE;

            /* write index */
            if (ffm_write_write_index(c->feed_fd, feed->feed_write_index) < 0) {
                http_log("Error writing index to feed file: %s\n",
                         strerror(errno));
                goto fail;
            }

            /* wake up any waiting connections */
            for(c1 = first_http_ctx; c1; c1 = c1->next) {
                if (c1->state == HTTPSTATE_WAIT_FEED &&
                    c1->stream->feed == c->stream->feed)
                    c1->state = HTTPSTATE_SEND_DATA;
            }
        } else {
            /* We have a header in our hands that contains useful data */
            AVFormatContext *s = avformat_alloc_context();
            AVIOContext *pb;
            AVInputFormat *fmt_in;
            int i;

            if (!s)
                goto fail;

            /* use feed output format name to find corresponding input format */
            fmt_in = av_find_input_format(feed->fmt->name);
            if (!fmt_in)
                goto fail;

            pb = avio_alloc_context(c->buffer, c->buffer_end - c->buffer,
                                    0, NULL, NULL, NULL, NULL);
            if (!pb)
                goto fail;

            pb->seekable = 0;

            s->pb = pb;
            if (avformat_open_input(&s, c->stream->feed_filename, fmt_in, NULL) < 0) {
                av_freep(&pb);
                goto fail;
            }

            /* Now we have the actual streams */
            if (s->nb_streams != feed->nb_streams) {
                avformat_close_input(&s);
                av_freep(&pb);
                http_log("Feed '%s' stream number does not match registered feed\n",
                         c->stream->feed_filename);
                goto fail;
            }

            for (i = 0; i < s->nb_streams; i++) {
                LayeredAVStream *fst = feed->streams[i];
                AVStream *st = s->streams[i];
                avcodec_parameters_to_context(fst->codec, st->codecpar);
                avcodec_parameters_from_context(fst->codecpar, fst->codec);
            }

            avformat_close_input(&s);
            av_freep(&pb);
        }
        c->buffer_ptr = c->buffer;
    }

    return 0;
 fail:
    c->stream->feed_opened = 0;
    close(c->feed_fd);
    /* wake up any waiting connections to stop waiting for feed */
    for(c1 = first_http_ctx; c1; c1 = c1->next) {
        if (c1->state == HTTPSTATE_WAIT_FEED &&
            c1->stream->feed == c->stream->feed)
            c1->state = HTTPSTATE_SEND_DATA_TRAILER;
    }
    return -1;
}