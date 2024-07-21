void problem_data_load_from_dump_dir(problem_data_t *problem_data, struct dump_dir *dd, char **excluding)
{
    char *short_name;
    char *full_name;

    dd_init_next_file(dd);
    while (dd_get_next_file(dd, &short_name, &full_name))
    {
        if (excluding && is_in_string_list(short_name, excluding))
        {
            //log("Excluded:'%s'", short_name);
            goto next;
        }

        if (short_name[0] == '#'
         || (short_name[0] && short_name[strlen(short_name) - 1] == '~')
        ) {
            //log("Excluded (editor backup file):'%s'", short_name);
            goto next;
        }

        ssize_t sz = 4*1024;
        char *text = is_text_file(full_name, &sz);
        if (!text || text == HUGE_TEXT)
        {
            int flag = !text ? CD_FLAG_BIN : (CD_FLAG_BIN+CD_FLAG_BIGTXT);
            problem_data_add(problem_data,
                    short_name,
                    full_name,
                    flag + CD_FLAG_ISNOTEDITABLE
            );
            goto next;
        }

        char *content;
        if (sz < 4*1024) /* did is_text_file read entire file? */
        {
            /* yes */
            content = text;
        }
        else
        {
            /* no, need to read it all */
            free(text);
            content = dd_load_text(dd, short_name);
        }
        /* Strip '\n' from one-line elements: */
        char *nl = strchr(content, '\n');
        if (nl && nl[1] == '\0')
            *nl = '\0';

        /* Sanitize possibly corrupted utf8.
         * Of control chars, allow only tab and newline.
         */
        char *sanitized = sanitize_utf8(content,
                (SANITIZE_ALL & ~SANITIZE_LF & ~SANITIZE_TAB)
        );
        if (sanitized)
        {
            free(content);
            content = sanitized;
        }

        bool editable = is_editable_file(short_name);
        int flags = 0;
        if (editable)
            flags |= CD_FLAG_TXT | CD_FLAG_ISEDITABLE;
        else
            flags |= CD_FLAG_TXT | CD_FLAG_ISNOTEDITABLE;

        static const char *const list_files[] = {
            FILENAME_UID       ,
            FILENAME_PACKAGE   ,
            FILENAME_CMDLINE   ,
            FILENAME_TIME      ,
            FILENAME_COUNT     ,
            FILENAME_REASON    ,
            NULL
        };
        if (is_in_string_list(short_name, (char**)list_files))
            flags |= CD_FLAG_LIST;

        if (strcmp(short_name, FILENAME_TIME) == 0)
            flags |= CD_FLAG_UNIXTIME;

        problem_data_add(problem_data,
                short_name,
                content,
                flags
        );
        free(content);
 next:
        free(short_name);
        free(full_name);
    }
}