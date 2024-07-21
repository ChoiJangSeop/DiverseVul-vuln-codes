int transform_save(struct augeas *aug, struct tree *xfm,
                   const char *path, struct tree *tree) {
    int   fd;
    FILE *fp = NULL, *augorig_canon_fp = NULL;
    char *augtemp = NULL, *augnew = NULL, *augorig = NULL, *augsave = NULL;
    char *augorig_canon = NULL, *augdest = NULL;
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

    if (access(augorig_canon, R_OK) == 0) {
        augorig_canon_fp = fopen(augorig_canon, "r");
        text = xfread_file(augorig_canon_fp);
    } else {
        text = strdup("");
    }

    if (text == NULL) {
        err_status = "put_read";
        goto done;
    }

    text = append_newline(text, strlen(text));

    /* Figure out where to put the .augnew and temp file. If no .augnew file
       then put the temp file next to augorig_canon, else next to .augnew. */
    if (aug->flags & AUG_SAVE_NEWFILE) {
        if (xasprintf(&augnew, "%s" EXT_AUGNEW, augorig) < 0) {
            err_status = "augnew_oom";
            goto done;
        }
        augdest = augnew;
    } else {
        augdest = augorig_canon;
    }

    if (xasprintf(&augtemp, "%s.XXXXXX", augdest) < 0) {
        err_status = "augtemp_oom";
        goto done;
    }

    // FIXME: We might have to create intermediate directories
    // to be able to write augnew, but we have no idea what permissions
    // etc. they should get. Just the process default ?
    fd = mkstemp(augtemp);
    if (fd < 0) {
        err_status = "mk_augtemp";
        goto done;
    }
    fp = fdopen(fd, "w");
    if (fp == NULL) {
        err_status = "open_augtemp";
        goto done;
    }

    if (augorig_exists) {
        if (transfer_file_attrs(augorig_canon_fp, fp, &err_status) != 0) {
            err_status = "xfer_attrs";
            goto done;
        }
    }

    if (tree != NULL)
        lns_put(fp, lens, tree->children, text, &err);

    if (ferror(fp)) {
        err_status = "error_augtemp";
        goto done;
    }

    if (fflush(fp) != 0) {
        err_status = "flush_augtemp";
        goto done;
    }

    if (fsync(fileno(fp)) < 0) {
        err_status = "sync_augtemp";
        goto done;
    }

    if (fclose(fp) != 0) {
        err_status = "close_augtemp";
        fp = NULL;
        goto done;
    }

    fp = NULL;

    if (err != NULL) {
        err_status = err->pos >= 0 ? "parse_skel_failed" : "put_failed";
        unlink(augtemp);
        goto done;
    }

    {
        char *new_text = xread_file(augtemp);
        int same = 0;
        if (new_text == NULL) {
            err_status = "read_augtemp";
            goto done;
        }
        same = STREQ(text, new_text);
        FREE(new_text);
        if (same) {
            result = 0;
            unlink(augtemp);
            goto done;
        } else if (aug->flags & AUG_SAVE_NOOP) {
            result = 1;
            unlink(augtemp);
            goto done;
        }
    }

    if (!(aug->flags & AUG_SAVE_NEWFILE)) {
        if (augorig_exists && (aug->flags & AUG_SAVE_BACKUP)) {
            r = xasprintf(&augsave, "%s" EXT_AUGSAVE, augorig);
            if (r == -1) {
                augsave = NULL;
                goto done;
            }

            r = clone_file(augorig_canon, augsave, &err_status, 1, 1);
            if (r != 0) {
                dyn_err_status = strappend(err_status, "_augsave");
                goto done;
            }
        }
    }

    r = clone_file(augtemp, augdest, &err_status, copy_if_rename_fails, 0);
    if (r != 0) {
        dyn_err_status = strappend(err_status, "_augtemp");
        goto done;
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
    free(augtemp);
    free(augnew);
    if (augorig_canon != augorig)
        free(augorig_canon);
    free(augorig);
    free(augsave);
    free_lns_error(err);

    if (fp != NULL)
        fclose(fp);
    if (augorig_canon_fp != NULL)
        fclose(augorig_canon_fp);
    return result;
}