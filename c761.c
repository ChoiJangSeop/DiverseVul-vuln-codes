int remove_file(struct augeas *aug, struct tree *tree) {
    char *path = NULL;
    const char *filename = NULL;
    const char *err_status = NULL;
    char *dyn_err_status = NULL;
    char *augsave = NULL, *augorig = NULL, *augorig_canon = NULL;
    int r;

    path = path_of_tree(tree);
    if (path == NULL) {
        err_status = "path_of_tree";
        goto error;
    }
    filename = path + strlen(AUGEAS_META_FILES);

    if ((augorig = strappend(aug->root, filename + 1)) == NULL) {
        err_status = "root_file";
        goto error;
    }

    augorig_canon = canonicalize_file_name(augorig);
    if (augorig_canon == NULL) {
        if (errno == ENOENT) {
            goto done;
        } else {
            err_status = "canon_augorig";
            goto error;
        }
    }

    r = file_saved_event(aug, path + strlen(AUGEAS_META_TREE));
    if (r < 0) {
        err_status = "saved_event";
        goto error;
    }

    if (aug->flags & AUG_SAVE_NOOP)
        goto done;

    if (aug->flags & AUG_SAVE_BACKUP) {
        /* Move file to one with extension .augsave */
        r = asprintf(&augsave, "%s" EXT_AUGSAVE, augorig_canon);
        if (r == -1) {
            augsave = NULL;
                goto error;
        }

        r = clone_file(augorig_canon, augsave, &err_status, 1);
        if (r != 0) {
            dyn_err_status = strappend(err_status, "_augsave");
            goto error;
        }
    } else {
        /* Unlink file */
        r = unlink(augorig_canon);
        if (r < 0) {
            err_status = "unlink_orig";
            goto error;
        }
    }
    tree_unlink(tree);
 done:
    free(path);
    free(augorig);
    free(augorig_canon);
    free(augsave);
    return 0;
 error:
    {
        const char *emsg =
            dyn_err_status == NULL ? err_status : dyn_err_status;
        store_error(aug, filename, path, emsg, errno, NULL, NULL);
    }
    free(path);
    free(augorig);
    free(augorig_canon);
    free(augsave);
    free(dyn_err_status);
    return -1;
}