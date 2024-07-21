WRITE_JSON_ELEMENT(ArrStart) {
    /* increase depth, save: before first array entry no comma needed. */
    ctx->commaNeeded[++ctx->depth] = false;
    return writeChar(ctx, '[');
}