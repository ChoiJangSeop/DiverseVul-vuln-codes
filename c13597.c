nfa_t::nfa_t(const RESpec &spec)
    : max_size(estimate_size(spec.res))
    , size(0)
    , states(new nfa_state_t[max_size])
    , charset(spec.charset)
    , rules(spec.rules)
    , tags(spec.tags)
    , root(NULL)
    , ncores(0)
{
    const size_t nre = spec.res.size();

    if (nre == 0) return;

    for (size_t i = 0; i < nre; ++i) {
        rtn_ctx_t ctx = {spec.opts, *this, i};

        nfa_state_t *s = &states[size++];
        s->make_fin(i);
        s = re_to_nfa(ctx, spec.res[i], s);

        if (root) {
            nfa_state_t *t = &states[size++];
            t->make_alt(i, root, s);
            root = t;
        } else {
            root = s;
        }
    }

    // In-degree and topological index are used by POSIX disambiguation;
    // the number of core states is used for both POSIX and leftmost.
    ncores = stats(root);
}