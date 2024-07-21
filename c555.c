check_user_file (const char *filename,
                 uid_t       user,
                 gssize      max_file_size,
                 gboolean    relax_group,
                 gboolean    relax_other)
{
        struct stat fileinfo;

        if (max_file_size < 0) {
                max_file_size = G_MAXSIZE;
        }

        /* Exists/Readable? */
        if (stat (filename, &fileinfo) < 0) {
                return FALSE;
        }

        /* Is a regular file */
        if (G_UNLIKELY (!S_ISREG (fileinfo.st_mode))) {
                return FALSE;
        }

        /* Owned by user? */
        if (G_UNLIKELY (fileinfo.st_uid != user)) {
                return FALSE;
        }

        /* Group not writable or relax_group? */
        if (G_UNLIKELY ((fileinfo.st_mode & S_IWGRP) == S_IWGRP && !relax_group)) {
                return FALSE;
        }

        /* Other not writable or relax_other? */
        if (G_UNLIKELY ((fileinfo.st_mode & S_IWOTH) == S_IWOTH && !relax_other)) {
                return FALSE;
        }

        /* Size is kosher? */
        if (G_UNLIKELY (fileinfo.st_size > max_file_size)) {
                return FALSE;
        }

        return TRUE;
}