gdm_session_worker_open_session (GdmSessionWorker  *worker,
                                 GError           **error)
{
        int error_code;
        int flags;
        char *session_id = NULL;

        g_assert (worker->priv->state == GDM_SESSION_WORKER_STATE_ACCOUNT_DETAILS_SAVED);
        g_assert (geteuid () == 0);

        switch (worker->priv->display_mode) {
        case GDM_SESSION_DISPLAY_MODE_REUSE_VT:
                if (!set_up_for_current_vt (worker, error)) {
                        return FALSE;
                }
                break;
        case GDM_SESSION_DISPLAY_MODE_NEW_VT:
        case GDM_SESSION_DISPLAY_MODE_LOGIND_MANAGED:
                if (!set_up_for_new_vt (worker)) {
                        g_set_error (error,
                                     GDM_SESSION_WORKER_ERROR,
                                     GDM_SESSION_WORKER_ERROR_OPENING_SESSION,
                                     "Unable to open VT");
                        return FALSE;
                }
                break;
        }

        flags = 0;

        if (worker->priv->is_program_session) {
                flags |= PAM_SILENT;
        }

        error_code = pam_open_session (worker->priv->pam_handle, flags);

        if (error_code != PAM_SUCCESS) {
                g_set_error (error,
                             GDM_SESSION_WORKER_ERROR,
                             GDM_SESSION_WORKER_ERROR_OPENING_SESSION,
                             "%s", pam_strerror (worker->priv->pam_handle, error_code));
                goto out;
        }

        g_debug ("GdmSessionWorker: state SESSION_OPENED");
        gdm_session_worker_set_state (worker, GDM_SESSION_WORKER_STATE_SESSION_OPENED);

        session_id = gdm_session_worker_get_environment_variable (worker, "XDG_SESSION_ID");

        if (session_id != NULL) {
                g_free (worker->priv->session_id);
                worker->priv->session_id = session_id;
        }

 out:
        if (error_code != PAM_SUCCESS) {
                gdm_session_worker_uninitialize_pam (worker, error_code);
                return FALSE;
        }

        gdm_session_worker_get_username (worker, NULL);
        gdm_session_auditor_report_login (worker->priv->auditor);

        return TRUE;
}