attempt_to_load_user_settings (GdmSessionWorker *worker,
                               const char       *username)
{
        struct passwd *passwd_entry;
        uid_t          old_uid;
        gid_t          old_gid;

        old_uid = geteuid ();
        old_gid = getegid ();

        passwd_entry = getpwnam (username);

        /* User input isn't a valid username
         */
        if (passwd_entry == NULL) {
                return;
        }

        /* We may get called late in the pam conversation after
         * the user has already been authenticated.  This could
         * happen if for instance, the user's home directory isn't
         * available until late in the pam conversation so user
         * settings couldn't get loaded until late in the conversation.
         * If we get called late the seteuid/setgid calls here will fail,
         * but that's okay, because we'll already be the uid/gid we want
         * to be.
         */
        setegid (passwd_entry->pw_gid);
        seteuid (passwd_entry->pw_uid);

        gdm_session_settings_load (worker->priv->user_settings,
                                   passwd_entry->pw_dir,
                                   NULL);

        seteuid (old_uid);
        setegid (old_gid);
}