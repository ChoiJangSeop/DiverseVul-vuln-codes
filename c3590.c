cli_file_t cli_get_container_type(cli_ctx *ctx, int index)
{
    if (index < 0)
	index = ctx->recursion + index + 1;
    if (index >= 0 && index <= ctx->recursion)
	return ctx->containers[index].type;
    return CL_TYPE_ANY;
}