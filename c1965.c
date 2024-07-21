static void sdap_initgr_nested_search(struct tevent_req *subreq)
{
    struct tevent_req *req;
    struct sdap_initgr_nested_state *state;
    struct sysdb_attrs **groups;
    size_t count;
    int ret;

    req = tevent_req_callback_data(subreq, struct tevent_req);
    state = tevent_req_data(req, struct sdap_initgr_nested_state);

    ret = sdap_get_generic_recv(subreq, state, &count, &groups);
    talloc_zfree(subreq);
    if (ret) {
        tevent_req_error(req, ret);
        return;
    }

    if (count == 1) {
        state->groups[state->groups_cur] = talloc_steal(state->groups,
                                                        groups[0]);
        state->groups_cur++;
    } else {
        DEBUG(SSSDBG_OP_FAILURE,
              "Search for group %s, returned %zu results. Skipping\n",
               state->group_dns[state->cur], count);
    }

    state->cur++;
    /* note that state->memberof->num_values is the count of original
     * memberOf which might not be only groups, but permissions, etc.
     * Use state->groups_cur for group index cap */
    if (state->cur < state->memberof->num_values) {
        subreq = sdap_get_generic_send(state, state->ev,
                                       state->opts, state->sh,
                                       state->group_dns[state->cur],
                                       LDAP_SCOPE_BASE,
                                       state->filter, state->grp_attrs,
                                       state->opts->group_map,
                                       SDAP_OPTS_GROUP,
                                       dp_opt_get_int(state->opts->basic,
                                                      SDAP_SEARCH_TIMEOUT),
                                       false);
        if (!subreq) {
            tevent_req_error(req, ENOMEM);
            return;
        }
        tevent_req_set_callback(subreq, sdap_initgr_nested_search, req);
    } else {
        sdap_initgr_nested_store(req);
    }
}