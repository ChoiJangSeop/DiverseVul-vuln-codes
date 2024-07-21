njs_generate_try_end(njs_vm_t *vm, njs_generator_t *generator,
    njs_parser_node_t *node)
{
    njs_int_t                ret;
    njs_index_t              exit_index;
    const njs_str_t          *dest_label;
    njs_vmcode_finally_t     *finally;
    njs_generator_patch_t    *patch;
    njs_generator_block_t    *block, *try_block, *catch_block;
    njs_generator_try_ctx_t  *ctx;

    ctx = generator->context;

    try_block = ctx->try_block;
    exit_index = try_block->index;
    catch_block = ctx->catch_block;

    njs_generate_code_finally(generator, finally, ctx->exception_index,
                              exit_index, node);

    if (try_block->continuation != NULL
        || (catch_block && catch_block->continuation != NULL))
    {
        dest_label = njs_generate_jump_destination(vm, generator->block,
                                                   "try continue",
                                                   NJS_GENERATOR_LOOP,
                                                   &ctx->try_cont_label,
                                                   &ctx->catch_cont_label);
        if (njs_slow_path(dest_label == NULL)) {
            return NJS_ERROR;
        }

        /*
         * block != NULL is checked
         * by njs_generate_continue_statement()
         */
        block = njs_generate_find_block(vm, generator->block,
                                        NJS_GENERATOR_LOOP, dest_label);

        patch = njs_generate_make_continuation_patch(vm, block, dest_label,
                         njs_code_offset(generator, finally)
                         + offsetof(njs_vmcode_finally_t, continue_offset));
        if (njs_slow_path(patch == NULL)) {
            return NJS_ERROR;
        }
    }

    if (try_block->exit != NULL
        || (catch_block != NULL && catch_block->exit != NULL))
    {
        dest_label = njs_generate_jump_destination(vm, generator->block,
                                                   "try break/return",
                                                   NJS_GENERATOR_ALL
                                                   | NJS_GENERATOR_TRY,
                                                   &ctx->try_exit_label,
                                                   &ctx->catch_exit_label);
        if (njs_slow_path(dest_label == NULL)) {
            return NJS_ERROR;
        }

        /*
         * block can be NULL for "return" instruction in
         * outermost try-catch block.
         */
        block = njs_generate_find_block(vm, generator->block,
                                        NJS_GENERATOR_ALL
                                        | NJS_GENERATOR_TRY, dest_label);
        if (block != NULL) {
            patch = njs_generate_make_exit_patch(vm, block, dest_label,
                            njs_code_offset(generator, finally)
                            + offsetof(njs_vmcode_finally_t, break_offset));
            if (njs_slow_path(patch == NULL)) {
                return NJS_ERROR;
            }
        }
    }

    ret = njs_generate_index_release(vm, generator, ctx->exception_index);
    if (njs_slow_path(ret != NJS_OK)) {
        return ret;
    }

    return njs_generator_stack_pop(vm, generator, ctx);
}