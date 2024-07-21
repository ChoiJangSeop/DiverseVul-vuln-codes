int transform_save(struct augeas *aug, struct tree *xfm,
                   const char *path, struct tree *tree) {
    FILE *fp = NULL;
    char *augnew = NULL, *augorig = NULL, *augsave = NULL;
    char *augorig_canon = NULL;
    int   augorig_exists;
    int   copy_if_rename_fails = 0;
    char *text = NULL;
    const char *filename = path + strlen(AUGEAS_FILES_TREE) + 1;
    const char *err_status = NULL;
    char *dyn_err_status = NULL;
    struct lns_error *err = NULL;
    const char *lens_name;
    struct lens *lens = xfm_lens(aug, xfm, &lens_name);
    int result = -1, r;
    bool force_reload;

    errno = 0;

    if (lens == NULL) {
        err_status = "lens_name";
        goto done;
    }

    copy_if_rename_fails =
        aug_get(aug, AUGEAS_COPY_IF_RENAME_FAILS, NULL) == 1;

    if (asprintf(&augorig, "%s%s", aug->root, filename) == -1) {
        augorig = NULL;
        goto done;
    }

    if (access(augorig, R_OK) == 0) {
        text = xread_file(augorig);
    } else {
        text = strdup("");
    }

    if (text == NULL) {
        err_status = "put_read";
        goto done;
    }

    text = append_newline(text, strlen(text));

    augorig_canon = canonicalize_file_name(augorig);
    augorig_exists = 1;
    if (augorig_canon == NULL) {
        if (errno == ENOENT) {
            augorig_canon = augorig;
            augorig_exists = 0;
        } else {
            err_status = "canon_augorig";
            goto done;
        }
    }

    /* Figure out where to put the .augnew file. If we need to rename it
       later on, put it next to augorig_canon */
    if (aug->flags & AUG_SAVE_NEWFILE) {
        if (xasprintf(&augnew, "%s" EXT_AUGNEW, augorig) < 0) {
            err_status = "augnew_oom";
            goto done;
        }
    } else {
        if (xasprintf(&augnew, "%s" EXT_AUGNEW, augorig_canon) < 0) {
            err_status = "augnew_oom";
            goto done;
        }
    }

    // FIXME: We might have to create intermediate directories
    // to be able to write augnew, but we have no idea what permissions
    // etc. they should get. Just the process default ?
    fp = fopen(augnew, "w");
    if (fp == NULL) {
        err_status = "open_augnew";
        goto done;
    }

    if (augorig_exists) {
        if (transfer_file_attrs(augorig_canon, augnew, &err_status) != 0) {
            err_status = "xfer_attrs";
            goto done;
        }
    }

    if (tree != NULL)
        lns_put(fp, lens, tree->children, text, &err);

    if (ferror(fp)) {
        err_status = "error_augnew";
        goto done;
    }

    if (fflush(fp) != 0) {
        err_status = "flush_augnew";
        goto done;
    }

    if (fsync(fileno(fp)) < 0) {
        err_status = "sync_augnew";
        goto done;
    }

    if (fclose(fp) != 0) {
        err_status = "close_augnew";
        fp = NULL;
        goto done;
    }

    fp = NULL;

    if (err != NULL) {
        err_status = err->pos >= 0 ? "parse_skel_failed" : "put_failed";
        unlink(augnew);
        goto done;
    }

    {
        char *new_text = xread_file(augnew);
        int same = 0;
        if (new_text == NULL) {
            err_status = "read_augnew";
            goto done;
        }
        same = STREQ(text, new_text);
        FREE(new_text);
        if (same) {
            result = 0;
            unlink(augnew);
            goto done;
        } else if (aug->flags & AUG_SAVE_NOOP) {
            result = 1;
            unlink(augnew);
            goto done;
        }
    }

    if (!(aug->flags & AUG_SAVE_NEWFILE)) {
        if (augorig_exists && (aug->flags & AUG_SAVE_BACKUP)) {
            r = asprintf(&augsave, "%s%s" EXT_AUGSAVE, aug->root, filename);
            if (r == -1) {
                augsave = NULL;
                goto done;
            }

            r = clone_file(augorig_canon, augsave, &err_status, 1);
            if (r != 0) {
                dyn_err_status = strappend(err_status, "_augsave");
                goto done;
            }
        }
        r = clone_file(augnew, augorig_canon, &err_status,
                       copy_if_rename_fails);
        if (r != 0) {
            dyn_err_status = strappend(err_status, "_augnew");
            goto done;
        }
    }
    result = 1;

 done:
    force_reload = aug->flags & AUG_SAVE_NEWFILE;
    r = add_file_info(aug, path, lens, lens_name, augorig, force_reload);
    if (r < 0) {
        err_status = "file_info";
        result = -1;
    }
    if (result > 0) {
        r = file_saved_event(aug, path);
        if (r < 0) {
            err_status = "saved_event";
            result = -1;
        }
    }
    {
        const char *emsg =
            dyn_err_status == NULL ? err_status : dyn_err_status;
        store_error(aug, filename, path, emsg, errno, err, text);
    }
    free(dyn_err_status);
    lens_release(lens);
    free(text);
    free(augnew);
    if (augorig_canon != augorig)
        free(augorig_canon);
    free(augorig);
    free(augsave);
    free_lns_error(err);

    if (fp != NULL)
        fclose(fp);
    return result;
}