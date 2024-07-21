static int timed_out_busy_iter(void *data, void *val)
{
    stream_iter_ctx *ctx = data;
    h2_stream *stream = val;
    if (stream->task && !stream->task->worker_done
        && (ctx->now - stream->task->started_at) > stream->task->timeout) {
        /* timed out stream occupying a worker, found */
        ctx->stream = stream;
        return 0;
    }
    return 1;
}