njs_generate_try_catch(njs_vm_t *vm, njs_generator_t *generator,
    njs_parser_node_t *node)
{
    njs_int_t                ret;
    njs_index_t              exit_index;
    njs_vmcode_finally_t     *finally;
    njs_generator_patch_t    *patch;
    njs_generator_block_t    *block, *try_block;
    njs_generator_try_ctx_t  *ctx;

    ctx = generator->context;

    try_block = ctx->try_block;
    exit_index = try_block->index;

    njs_code_set_jump_offset(generator, njs_vmcode_try_end_t,
                             ctx->try_offset);

    if (try_block->continuation != NULL || try_block->exit != NULL) {
        njs_generate_code_finally(generator, finally, ctx->exception_index,
                                  exit_index, NULL);

        if (try_block->continuation != NULL) {
            /*
             * block != NULL is checked
             * by njs_generate_continue_statement()
             */
            block = njs_generate_find_block(vm, generator->block,
                                            NJS_GENERATOR_LOOP,
                                            &ctx->try_cont_label);

            patch = njs_generate_make_continuation_patch(vm, block,
                                                         &ctx->try_cont_label,
                        njs_code_offset(generator, finally)
                         + offsetof(njs_vmcode_finally_t, continue_offset));
            if (njs_slow_path(patch == NULL)) {
                return NJS_ERROR;
            }
        }

        if (try_block->exit != NULL) {
            block = njs_generate_find_block(vm, generator->block,
                                            NJS_GENERATOR_ALL,
                                            &ctx->try_exit_label);

            if (block != NULL) {
                patch = njs_generate_make_exit_patch(vm, block,
                                                     &ctx->try_exit_label,
                            njs_code_offset(generator, finally)
                            + offsetof(njs_vmcode_finally_t, break_offset));
                if (njs_slow_path(patch == NULL)) {
                    return NJS_ERROR;
                }
            }
        }
    }

    /* TODO: release exception variable index. */

    ret = njs_generate_index_release(vm, generator, ctx->exception_index);
    if (njs_slow_path(ret != NJS_OK)) {
        return ret;
    }

    return njs_generator_stack_pop(vm, generator, ctx);
}