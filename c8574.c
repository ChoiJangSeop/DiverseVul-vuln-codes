yang_read_extcomplex_str(struct lys_module *module, struct lys_ext_instance_complex *ext, const char *arg_name,
                         const char *parent_name, char *value, int parent_stmt, LY_STMT stmt)
{
    int c;
    const char **str, ***p = NULL;
    void *reallocated;
    struct lyext_substmt *info;

    c = 0;
    if (stmt == LY_STMT_PREFIX && parent_stmt == LY_STMT_BELONGSTO) {
        /* str contains no NULL value */
        str = lys_ext_complex_get_substmt(LY_STMT_BELONGSTO, ext, &info);
        if (info->cardinality < LY_STMT_CARD_SOME) {
            str++;
        } else {
           /* get the index in the array to add new item */
            p = (const char ***)str;
            for (c = 0; p[0][c + 1]; c++);
            str = p[1];
        }
        str[c] = lydict_insert_zc(module->ctx, value);
    }  else {
        str = lys_ext_complex_get_substmt(stmt, ext, &info);
        if (!str) {
            LOGVAL(module->ctx, LYE_INCHILDSTMT, LY_VLOG_NONE, NULL, arg_name, parent_name);
            goto error;
        }
        if (info->cardinality < LY_STMT_CARD_SOME && *str) {
            LOGVAL(module->ctx, LYE_TOOMANY, LY_VLOG_NONE, NULL, arg_name, parent_name);
            goto error;
        }

        if (info->cardinality >= LY_STMT_CARD_SOME) {
            /* there can be multiple instances, str is actually const char *** */
            p = (const char ***)str;
            if (!p[0]) {
                /* allocate initial array */
                p[0] = calloc(2, sizeof(const char *));
                LY_CHECK_ERR_GOTO(!p[0], LOGMEM(module->ctx), error);
                if (stmt == LY_STMT_BELONGSTO) {
                    /* allocate another array for the belongs-to's prefixes */
                    p[1] = calloc(2, sizeof(const char *));
                    LY_CHECK_ERR_GOTO(!p[1], LOGMEM(module->ctx), error);
                } else if (stmt == LY_STMT_ARGUMENT) {
                    /* allocate another array for the yin element */
                    ((uint8_t **)p)[1] = calloc(2, sizeof(uint8_t));
                    LY_CHECK_ERR_GOTO(!p[1], LOGMEM(module->ctx), error);
                    /* default value of yin element */
                    ((uint8_t *)p[1])[0] = 2;
                }
            } else {
                /* get the index in the array to add new item */
                for (c = 0; p[0][c]; c++);
            }
            str = p[0];
        }

        str[c] = lydict_insert_zc(module->ctx, value);
        value = NULL;

        if (c) {
            /* enlarge the array(s) */
            reallocated = realloc(p[0], (c + 2) * sizeof(const char *));
            if (!reallocated) {
                LOGMEM(module->ctx);
                lydict_remove(module->ctx, p[0][c]);
                p[0][c] = NULL;
                return EXIT_FAILURE;
            }
            p[0] = reallocated;
            p[0][c + 1] = NULL;

            if (stmt == LY_STMT_BELONGSTO) {
                /* enlarge the second belongs-to's array with prefixes */
                reallocated = realloc(p[1], (c + 2) * sizeof(const char *));
                if (!reallocated) {
                    LOGMEM(module->ctx);
                    lydict_remove(module->ctx, p[1][c]);
                    p[1][c] = NULL;
                    return EXIT_FAILURE;
                }
                p[1] = reallocated;
                p[1][c + 1] = NULL;
            } else if (stmt == LY_STMT_ARGUMENT) {
                /* enlarge the second argument's array with yin element */
                reallocated = realloc(p[1], (c + 2) * sizeof(uint8_t));
                if (!reallocated) {
                    LOGMEM(module->ctx);
                    ((uint8_t *)p[1])[c] = 0;
                    return EXIT_FAILURE;
                }
                p[1] = reallocated;
                ((uint8_t *)p[1])[c + 1] = 0;
            }
        }
    }

    return EXIT_SUCCESS;

error:
    free(value);
    return EXIT_FAILURE;
}