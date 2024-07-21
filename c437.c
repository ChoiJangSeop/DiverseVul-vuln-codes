shake_dialog (GSWindow *window)
{
        int   i;
        guint left;
        guint right;

        for (i = 0; i < 9; i++) {
                if (i % 2 == 0) {
                        left = 30;
                        right = 0;
                } else {
                        left = 0;
                        right = 30;
                }

                if (! window->priv->lock_box) {
                        break;
                }

                gtk_alignment_set_padding (GTK_ALIGNMENT (window->priv->lock_box),
                                           0, 0,
                                           left,
                                           right);

                while (gtk_events_pending ()) {
                        gtk_main_iteration ();
                }

                g_usleep (10000);
        }
}