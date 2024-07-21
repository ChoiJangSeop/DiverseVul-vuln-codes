WRITE_JSON_ELEMENT(ObjStart) {
    /* increase depth, save: before first key-value no comma needed. */
    ctx->depth++;
    ctx->commaNeeded[ctx->depth] = false;
    return writeChar(ctx, '{');
}