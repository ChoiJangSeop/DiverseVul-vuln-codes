_change_user (GdmSessionWorker  *worker,
              uid_t              uid,
              gid_t              gid)
{
        gboolean ret;

        ret = FALSE;

#ifdef THE_MAN_PAGE_ISNT_LYING
        /* pam_setcred wants to be called as the authenticated user
         * but pam_open_session needs to be called as super-user.
         *
         * Set the real uid and gid to the user and give the user a
         * temporary super-user effective id.
         */
        if (setreuid (uid, GDM_SESSION_ROOT_UID) < 0) {
                return FALSE;
        }
#endif
        worker->priv->uid = uid;

        if (setgid (gid) < 0) {
                return FALSE;
        }

        if (initgroups (worker->priv->username, gid) < 0) {
                return FALSE;
        }

        return TRUE;
}