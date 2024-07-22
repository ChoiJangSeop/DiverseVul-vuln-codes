size_t cli_get_container_size(cli_ctx *ctx, int index)
{
    if (index < 0)
	index = ctx->recursion + index + 1;
    if (index >= 0 && index <= ctx->recursion)
	return ctx->containers[index].size;
    return 0;
}