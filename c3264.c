gdm_session_start_session (GdmSession *self,
                           const char *service_name)
{
        GdmSessionConversation *conversation;
        GdmSessionDisplayMode   display_mode;
        gboolean                is_x11 = TRUE;
        gboolean                run_launcher = FALSE;
        gboolean                allow_remote_connections = FALSE;
        char                   *command;
        char                   *program;

        g_return_if_fail (GDM_IS_SESSION (self));
        g_return_if_fail (self->priv->session_conversation == NULL);

        conversation = find_conversation_by_name (self, service_name);

        if (conversation == NULL) {
                g_warning ("GdmSession: Tried to start session of "
                           "nonexistent conversation %s", service_name);
                return;
        }

        stop_all_other_conversations (self, conversation, FALSE);

        display_mode = gdm_session_get_display_mode (self);

#ifdef ENABLE_WAYLAND_SUPPORT
        is_x11 = g_strcmp0 (self->priv->session_type, "wayland") != 0;
#endif

        if (display_mode == GDM_SESSION_DISPLAY_MODE_LOGIND_MANAGED ||
            display_mode == GDM_SESSION_DISPLAY_MODE_NEW_VT) {
                run_launcher = TRUE;
        }

        if (self->priv->selected_program == NULL) {
                gboolean run_xsession_script;

                command = get_session_command (self);

                run_xsession_script = !gdm_session_bypasses_xsession (self);

                if (self->priv->display_is_local) {
                        gboolean disallow_tcp = TRUE;
                        gdm_settings_direct_get_boolean (GDM_KEY_DISALLOW_TCP, &disallow_tcp);
                        allow_remote_connections = !disallow_tcp;
                } else {
                        allow_remote_connections = TRUE;
                }

                if (run_launcher) {
                        if (is_x11) {
                                program = g_strdup_printf (LIBEXECDIR "/gdm-x-session %s %s\"%s\"",
                                                           run_xsession_script? "--run-script " : "",
                                                           allow_remote_connections? "--allow-remote-connections " : "",
                                                           command);
                        } else {
                                program = g_strdup_printf (LIBEXECDIR "/gdm-wayland-session \"%s\"",
                                                           command);
                        }
                } else if (run_xsession_script) {
                        program = g_strdup_printf (GDMCONFDIR "/Xsession \"%s\"", command);
                } else {
                        program = g_strdup (command);
                }

                g_free (command);
        } else {
                if (run_launcher) {
                        if (is_x11) {
                                program = g_strdup_printf (LIBEXECDIR "/gdm-x-session \"%s\"",
                                                           self->priv->selected_program);
                        } else {
                                program = g_strdup_printf (LIBEXECDIR "/gdm-wayland-session \"%s\"",
                                                           self->priv->selected_program);
                        }
                } else {
                        program = g_strdup (self->priv->selected_program);
                }
        }

        set_up_session_environment (self);
        send_environment (self, conversation);

        gdm_dbus_worker_call_start_program (conversation->worker_proxy,
                                            program,
                                            NULL,
                                            (GAsyncReadyCallback) on_start_program_cb,
                                            conversation);
        g_free (program);
}