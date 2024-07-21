gs_grab_get_mouse (GSGrab    *grab,
                   GdkWindow *window,
                   GdkScreen *screen,
                   gboolean   hide_cursor)
{
        GdkGrabStatus status;
        GdkCursor    *cursor;

        g_return_val_if_fail (window != NULL, FALSE);
        g_return_val_if_fail (screen != NULL, FALSE);

        cursor = get_cursor ();

        gs_debug ("Grabbing mouse widget=%X", (guint32) GDK_WINDOW_XID (window));
        status = gdk_pointer_grab (window, TRUE, 0, NULL,
                                   (hide_cursor ? cursor : NULL),
                                   GDK_CURRENT_TIME);

        if (status == GDK_GRAB_SUCCESS) {
                grab->priv->mouse_grab_window = window;
                grab->priv->mouse_grab_screen = screen;
                grab->priv->mouse_hide_cursor = hide_cursor;
        }

        gdk_cursor_unref (cursor);

        return status;
}