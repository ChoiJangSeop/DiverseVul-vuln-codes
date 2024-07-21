static int latest_repeatable_unsubmitted_iter(void *data, void *val)
{
    stream_iter_ctx *ctx = data;
    h2_stream *stream = val;
    
    if (stream->task && !stream->task->worker_done 
        && h2_task_can_redo(stream->task) 
        && !h2_ihash_get(ctx->m->sredo, stream->id)) {
        if (!h2_stream_is_ready(stream)) {
            /* this task occupies a worker, the response has not been submitted 
             * yet, not been cancelled and it is a repeatable request
             * -> it can be re-scheduled later */
            if (!ctx->stream 
                || (ctx->stream->task->started_at < stream->task->started_at)) {
                /* we did not have one or this one was started later */
                ctx->stream = stream;
            }
        }
    }
    return 1;
}