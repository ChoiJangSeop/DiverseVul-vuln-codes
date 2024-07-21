static char *handle_new_problem(GVariant *problem_info, uid_t caller_uid, char **error)
{
    problem_data_t *pd = problem_data_new();

    GVariantIter *iter;
    g_variant_get(problem_info, "a{ss}", &iter);
    gchar *key, *value;
    while (g_variant_iter_loop(iter, "{ss}", &key, &value))
    {
        problem_data_add_text_editable(pd, key, value);
    }

    if (caller_uid != 0 || problem_data_get_content_or_NULL(pd, FILENAME_UID) == NULL)
    {   /* set uid field to caller's uid if caller is not root or root doesn't pass own uid */
        log_info("Adding UID %d to problem data", caller_uid);
        char buf[sizeof(uid_t) * 3 + 2];
        snprintf(buf, sizeof(buf), "%d", caller_uid);
        problem_data_add_text_noteditable(pd, FILENAME_UID, buf);
    }

    /* At least it should generate local problem identifier UUID */
    problem_data_add_basics(pd);

    char *problem_id = problem_data_save(pd);
    if (problem_id)
        notify_new_path(problem_id);
    else if (error)
        *error = xasprintf("Cannot create a new problem");

    problem_data_free(pd);
    return problem_id;
}