static void stream_cleanup(h2_mplx *m, h2_stream *stream)
{
    ap_assert(stream->state == H2_SS_CLEANUP);

    if (stream->input) {
        h2_beam_on_consumed(stream->input, NULL, NULL, NULL);
        h2_beam_abort(stream->input);
    }
    if (stream->output) {
        h2_beam_on_produced(stream->output, NULL, NULL);
        h2_beam_leave(stream->output);
    }
    
    h2_stream_cleanup(stream);

    h2_ihash_remove(m->streams, stream->id);
    h2_iq_remove(m->q, stream->id);
    h2_ififo_remove(m->readyq, stream->id);
    h2_ihash_add(m->shold, stream);
    
    if (!stream->task || stream->task->worker_done) {
        stream_joined(m, stream);
    }
    else if (stream->task) {
        stream->task->c->aborted = 1;
    }
}