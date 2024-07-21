render_icon_from_home (GdmUser *user,
                       int      icon_size)
{
        GdkPixbuf  *retval;
        char       *path;
        gboolean    is_local;
        gboolean    is_autofs;
        gboolean    res;
        char       *filesystem_type;

        is_local = FALSE;

        /* special case: look at parent of home to detect autofs
           this is so we don't try to trigger an automount */
        path = g_path_get_dirname (user->home_dir);
        filesystem_type = get_filesystem_type (path);
        is_autofs = (filesystem_type != NULL && strcmp (filesystem_type, "autofs") == 0);
        g_free (filesystem_type);
        g_free (path);

        if (is_autofs) {
                return NULL;
        }

        /* now check that home dir itself is local */
        filesystem_type = get_filesystem_type (user->home_dir);
        is_local = ((filesystem_type != NULL) &&
                    (strcmp (filesystem_type, "nfs") != 0) &&
                    (strcmp (filesystem_type, "afs") != 0) &&
                    (strcmp (filesystem_type, "autofs") != 0) &&
                    (strcmp (filesystem_type, "unknown") != 0) &&
                    (strcmp (filesystem_type, "ncpfs") != 0));
        g_free (filesystem_type);

        /* only look at local home directories so we don't try to
           read from remote (e.g. NFS) volumes */
        if (! is_local) {
                return NULL;
        }

        /* First, try "~/.face" */
        path = g_build_filename (user->home_dir, ".face", NULL);
        res = check_user_file (path,
                               user->uid,
                               MAX_FILE_SIZE,
                               RELAX_GROUP,
                               RELAX_OTHER);
        if (res) {
                retval = gdk_pixbuf_new_from_file_at_size (path,
                                                           icon_size,
                                                           icon_size,
                                                           NULL);
        } else {
                retval = NULL;
        }
        g_free (path);

        /* Next, try "~/.face.icon" */
        if (retval == NULL) {
                path = g_build_filename (user->home_dir,
                                         ".face.icon",
                                         NULL);
                res = check_user_file (path,
                                       user->uid,
                                       MAX_FILE_SIZE,
                                       RELAX_GROUP,
                                       RELAX_OTHER);
                if (res) {
                        retval = gdk_pixbuf_new_from_file_at_size (path,
                                                                   icon_size,
                                                                   icon_size,
                                                                   NULL);
                } else {
                        retval = NULL;
                }

                g_free (path);
        }

        /* Still nothing, try the user's personal GDM config */
        if (retval == NULL) {
                path = g_build_filename (user->home_dir,
                                         ".gnome",
                                         "gdm",
                                         NULL);
                res = check_user_file (path,
                                       user->uid,
                                       MAX_FILE_SIZE,
                                       RELAX_GROUP,
                                       RELAX_OTHER);
                if (res) {
                        GKeyFile *keyfile;
                        char     *icon_path;

                        keyfile = g_key_file_new ();
                        g_key_file_load_from_file (keyfile,
                                                   path,
                                                   G_KEY_FILE_NONE,
                                                   NULL);

                        icon_path = g_key_file_get_string (keyfile,
                                                           "face",
                                                           "picture",
                                                           NULL);
                        res = check_user_file (icon_path,
                                               user->uid,
                                               MAX_FILE_SIZE,
                                               RELAX_GROUP,
                                               RELAX_OTHER);
                        if (icon_path && res) {
                                retval = gdk_pixbuf_new_from_file_at_size (path,
                                                                           icon_size,
                                                                           icon_size,
                                                                           NULL);
                        } else {
                                retval = NULL;
                        }

                        g_free (icon_path);
                        g_key_file_free (keyfile);
                } else {
                        retval = NULL;
                }

                g_free (path);
        }

        return retval;
}