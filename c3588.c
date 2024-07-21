void cli_set_container(cli_ctx *ctx, cli_file_t type, size_t size)
{
    ctx->containers[ctx->recursion].type = type;
    ctx->containers[ctx->recursion].size = size;
}