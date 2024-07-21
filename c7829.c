static void stream_joined(h2_mplx *m, h2_stream *stream)
{
    ap_assert(!stream->task || stream->task->worker_done);
    
    h2_ihash_remove(m->shold, stream->id);
    h2_ihash_add(m->spurge, stream);
}