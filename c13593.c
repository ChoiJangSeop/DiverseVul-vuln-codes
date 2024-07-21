static smart_ptr<DFA> ast_to_dfa(const spec_t &spec, Output &output)
{
    OutputBlock &block = output.block();
    const opt_t *opts = block.opts;
    const loc_t &loc = block.loc;
    Msg &msg = output.msg;
    const std::vector<ASTRule> &rules = spec.rules;
    const std::string
        &cond = spec.name,
        name = make_name(output, cond, loc),
        &setup = spec.setup.empty() ? "" : spec.setup[0]->text;

    RangeMgr rangemgr;

    RESpec re(rules, opts, msg, rangemgr);
    split_charset(re);
    find_fixed_tags(re);
    insert_default_tags(re);
    warn_nullable(re, cond);

    nfa_t nfa(re);
    DDUMP_NFA(opts, nfa);

    dfa_t dfa(nfa, spec.def_rule, spec.eof_rule);
    determinization(nfa, dfa, opts, msg, cond);
    DDUMP_DFA_DET(opts, dfa);

    rangemgr.clear();

    // skeleton must be constructed after DFA construction
    // but prior to any other DFA transformations
    Skeleton skeleton(dfa, opts, name, cond, loc, msg);
    warn_undefined_control_flow(skeleton);
    if (opts->target == TARGET_SKELETON) {
        emit_data(skeleton);
    }

    cutoff_dead_rules(dfa, opts, cond, msg);

    insert_fallback_tags(opts, dfa);

    // try to minimize the number of tag variables
    compact_and_optimize_tags(opts, dfa);
    DDUMP_DFA_TAGOPT(opts, dfa);

    freeze_tags(dfa);

    minimization(dfa, opts->dfa_minimization);
    DDUMP_DFA_MIN(opts, dfa);

    // find strongly connected components and calculate argument to YYFILL
    std::vector<size_t> fill;
    fillpoints(dfa, fill);

    // ADFA stands for 'DFA with actions'
    DFA *adfa = new DFA(dfa, fill, skeleton.sizeof_key, loc, name, cond,
        setup, opts, msg);

    // see note [reordering DFA states]
    adfa->reorder();

    // skeleton is constructed, do further DFA transformations
    adfa->prepare(opts);
    DDUMP_ADFA(opts, *adfa);

    // gather overall DFA statistics and add it to the output block
    adfa->calc_stats(block);
    block.max_fill = std::max(block.max_fill, adfa->max_fill);
    block.max_nmatch = std::max(block.max_nmatch, adfa->max_nmatch);
    block.used_yyaccept = block.used_yyaccept || adfa->need_accept;

    return make_smart_ptr(adfa);
}