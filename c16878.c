on_display_status_changed (GdmDisplay *display,
                           GParamSpec *arg1,
                           GdmManager *manager)
{
        int         status;
        int         display_number = -1;
#ifdef WITH_PLYMOUTH
        gboolean    display_is_local = FALSE;
        gboolean    quit_plymouth = FALSE;

        g_object_get (display,
                      "is-local", &display_is_local,
                      NULL);
        quit_plymouth = display_is_local && manager->priv->plymouth_is_running;
#endif

        g_object_get (display, "x11-display-number", &display_number, NULL);

        status = gdm_display_get_status (display);

        switch (status) {
                case GDM_DISPLAY_PREPARED:
                case GDM_DISPLAY_MANAGED:
                        if ((display_number == -1 && status == GDM_DISPLAY_PREPARED) ||
                            (display_number != -1 && status == GDM_DISPLAY_MANAGED)) {
                                char *session_class;

                                g_object_get (display,
                                              "session-class", &session_class,
                                              NULL);
                                if (g_strcmp0 (session_class, "greeter") == 0)
                                        set_up_session (manager, display);
                                g_free (session_class);
                        }

                        if (status == GDM_DISPLAY_MANAGED) {
                                greeter_display_started (manager, display);
                        }
                        break;
                case GDM_DISPLAY_FAILED:
                case GDM_DISPLAY_UNMANAGED:
                case GDM_DISPLAY_FINISHED:
#ifdef WITH_PLYMOUTH
                        if (quit_plymouth) {
                                plymouth_quit_without_transition ();
                                manager->priv->plymouth_is_running = FALSE;
                        }
#endif

                        maybe_start_pending_initial_login (manager, display);
                        break;
                default:
                        break;
        }

}