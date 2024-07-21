determ_context_t<history_t>::determ_context_t(const opt_t *opts, Msg &msg
    , const std::string &condname, const nfa_t &nfa, dfa_t &dfa)
    : dc_opts(opts)
    , dc_msg(msg)
    , dc_condname(condname)
    , nfa(nfa)
    , dfa(dfa)
    , dc_allocator()
    , dc_origin(dfa_t::NIL)
    , dc_target(dfa_t::NIL)
    , dc_symbol(0)
    , dc_actions(NULL)
    , dc_tagvertbl(nfa.tags.size())
    , history()
    , dc_kernels()
    , dc_buffers()
    , dc_hc_caches()
    , dc_newvers(newver_cmp_t<history_t>(history, dc_hc_caches))
    , dc_path1()
    , dc_path2()
    , dc_path3()
    , dc_tagcount()
    , stadfa_actions(NULL)
    , stadfa_tagvers()
    , reach()
    , state()
    , gor1_topsort()
    , gor1_linear()
    , gtop_buffer()
    , gtop_cmp()
    , gtop_heap(gtop_cmp, gtop_buffer)
    , newprectbl(NULL)
    , oldprectbl(NULL)
    , oldprecdim(0)
    , histlevel(NULL)
    , sortcores()
    , fincount()
    , worklist()
    , dump_dfa_tree(*this)
    , dc_dump(opts)
    , dc_clstats()
{
    const size_t nstates = nfa.size;
    const size_t ncores = nfa.ncores;
    const size_t ntags = nfa.tags.size();

    reach.reserve(nstates);
    state.reserve(nstates);

    dc_hc_caches.resize(ntags);
    dc_path1.reserve(ntags);
    dc_path2.reserve(ntags);
    dc_path3.reserve(ntags);
    dc_tagcount.resize(ntags);

    if (opts->posix_semantics) {
        newprectbl = new prectable_t[ncores * ncores];
        histlevel = new histleaf_t[ncores];
        sortcores.reserve(ncores);
        fincount.resize(ncores + 1);
        worklist.reserve(nstates);
    }

    if (opts->posix_closure == POSIX_CLOSURE_GTOP) {
        gtop_buffer.reserve(nstates);
    }
    else {
        gor1_topsort.reserve(nstates);
        gor1_linear.reserve(nstates);
    }
}