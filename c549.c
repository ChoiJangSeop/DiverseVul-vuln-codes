gdm_user_render_icon (GdmUser   *user,
                      gint       icon_size)
{
        GdkPixbuf    *pixbuf;
        GdkPixbuf    *framed;
        char         *path;
        char         *tmp;
        gboolean      res;

        g_return_val_if_fail (GDM_IS_USER (user), NULL);
        g_return_val_if_fail (icon_size > 12, NULL);

        path = NULL;

        pixbuf = render_icon_from_home (user, icon_size);
        if (pixbuf != NULL) {
                goto out;
        }

        /* Try ${GlobalFaceDir}/${username} */
        path = g_build_filename (GLOBAL_FACEDIR, user->user_name, NULL);
        res = check_user_file (path,
                               user->uid,
                               MAX_FILE_SIZE,
                               RELAX_GROUP,
                               RELAX_OTHER);
        if (res) {
                pixbuf = gdk_pixbuf_new_from_file_at_size (path,
                                                           icon_size,
                                                           icon_size,
                                                           NULL);
        } else {
                pixbuf = NULL;
        }

        g_free (path);
        if (pixbuf != NULL) {
                goto out;
        }

        /* Finally, ${GlobalFaceDir}/${username}.png */
        tmp = g_strconcat (user->user_name, ".png", NULL);
        path = g_build_filename (GLOBAL_FACEDIR, tmp, NULL);
        g_free (tmp);
        res = check_user_file (path,
                               user->uid,
                               MAX_FILE_SIZE,
                               RELAX_GROUP,
                               RELAX_OTHER);
        if (res) {
                pixbuf = gdk_pixbuf_new_from_file_at_size (path,
                                                           icon_size,
                                                           icon_size,
                                                           NULL);
        } else {
                pixbuf = NULL;
        }
        g_free (path);
 out:

        if (pixbuf != NULL) {
                framed = frame_pixbuf (pixbuf);
                if (framed != NULL) {
                        g_object_unref (pixbuf);
                        pixbuf = framed;
                }
        }

        return pixbuf;
}