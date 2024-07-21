bool do_find_state(ctx_t &ctx)
{
    kernels_t &kernels = ctx.dc_kernels;
    const closure_t &closure = ctx.state;

    // empty closure corresponds to default state
    if (closure.size() == 0) {
        ctx.dc_target = dfa_t::NIL;
        ctx.dc_actions = NULL;
        return false;
    }

    // resize buffer if closure is too large
    reserve_buffers<ctx_t, stadfa>(ctx);
    kernel_t *k = ctx.dc_buffers.kernel;

    // copy closure to buffer kernel
    copy_to_buffer<stadfa>(closure, ctx.newprectbl, ctx.stadfa_actions, k);

    // hash "static" part of the kernel
    const uint32_t hash = hash_kernel(k);

    // try to find identical kernel
    kernel_eq_t<ctx_t, stadfa> cmp_eq = {ctx};
    ctx.dc_target = kernels.find_with(hash, k, cmp_eq);
    if (ctx.dc_target != kernels_t::NIL) return false;

    // else if not staDFA try to find mappable kernel
    // see note [bijective mappings]
    if (!stadfa) {
        kernel_map_t<ctx_t, regless> cmp_map = {ctx};
        ctx.dc_target = kernels.find_with(hash, k, cmp_map);
        if (ctx.dc_target != kernels_t::NIL) return false;
    }

    // otherwise add new kernel
    kernel_t *kcopy = make_kernel_copy<stadfa>(k, ctx.dc_allocator);
    ctx.dc_target = kernels.push(hash, kcopy);
    return true;
}