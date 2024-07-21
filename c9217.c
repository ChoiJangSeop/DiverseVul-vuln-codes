pam_sm_open_session(pam_handle_t *pamh, int flags UNUSED,
		    int argc, const char **argv)
{
  int retval;
  char *val, *mntdevice = NULL;
  const void *user;
  const struct passwd *pwd;
  struct pam_params param = {
          .start_uid = PAM_USERTYPE_UIDMIN,
          .end_uid = 0,
          .fs = NULL };
  struct if_dqblk ndqblk;
  FILE *fp;
  size_t mnt_len = 0, match_size = 0;
#ifdef HAVE_GETMNTENT_R
  char buf[BUFSIZ];
  struct mntent ent;
#endif
  const struct mntent *mnt;
  const char *service;

  if (pam_get_item(pamh, PAM_SERVICE, (const void **)&service) != PAM_SUCCESS)
    service = "";

  /* Get UID_MIN for default start_uid from login.defs */
  val = pam_modutil_search_key(pamh, PATH_LOGIN_DEFS, "UID_MIN");

  /* Should UID_MIN be undefined, use current value of param.start_uid
   * pre-defined as PAM_USERTYPE_UIDMIN set by configure as safe
   * starting UID to avoid setting a quota for root and system
   * users if startuid= parameter is absent.
   */
  if (val) {
    param.start_uid = str_to_uid(pamh, val, param.start_uid, PATH_LOGIN_DEFS":UID_MIN");
  }

  /* Parse parameter values
   * Must come after pam_modutil_search_key so that the current system
   * default for UID_MIN is already in p.start_uid to serve as default
   * for str_to_uid in case of a parse error.
   * */
  parse_params(pamh, argc, argv, &param);

  if (param.debug >= 1)
    pam_syslog(pamh, LOG_DEBUG, "Config: startuid=%u enduid=%u fs=%s "
                    "debug=%d overwrite=%d",
                    param.start_uid, param.end_uid,
                    param.fs ? param.fs : "(none)",
                    param.debug, param.overwrite);

  /* Determine the user name so we can get the home directory */
  retval = pam_get_item(pamh, PAM_USER, &user);
  if (retval != PAM_SUCCESS || user == NULL || *(const char *)user == '\0') {
    pam_syslog(pamh, LOG_NOTICE, "user unknown");
    return PAM_USER_UNKNOWN;
  }

  /* Get the password entry */
  pwd = pam_modutil_getpwnam(pamh, user);
  if (pwd == NULL) {
    pam_syslog(pamh, LOG_NOTICE, "user unknown");
    return PAM_USER_UNKNOWN;
  }

  /* Check if we should not set quotas for user */
  if ((pwd->pw_uid < param.start_uid) ||
      ((param.end_uid >= param.start_uid) && (param.start_uid != 0) &&
       (pwd->pw_uid > param.end_uid)))
    return PAM_SUCCESS;

  /* Find out what device the filesystem is hosted on */
  if ((fp = setmntent("/proc/mounts", "r")) == NULL) {
    pam_syslog(pamh, LOG_ERR, "Unable to open /proc/mounts");
    return PAM_PERM_DENIED;
  }

  while (
#ifdef HAVE_GETMNTENT_R
           (mnt = getmntent_r(fp, &ent, buf, sizeof(buf))) != NULL
#else
           (mnt = getmntent(fp)) != NULL
#endif
        ) {
    /* If param.fs is not specified use filesystem with users homedir
     * as default.
     */
    if (param.fs == NULL) {
      /* Mask trailing / from mnt->mnt_dir, to get a leading / on the
       * remaining suffix returned by pam_str_skip_prefix_len()
       */
      for (mnt_len = strlen(mnt->mnt_dir); mnt_len > 0; --mnt_len)
        if (mnt->mnt_dir[mnt_len - 1] != '/')
          break;
      const char *s;
      if (param.debug >= 2)
        pam_syslog(pamh, LOG_DEBUG, "Trying to match pw_dir=\"%s\" "
                        "with mnt_dir=\"%s\"", pwd->pw_dir, mnt->mnt_dir);
      /*
       * (mnt_len > match_size) Only try matching the mnt_dir if its length
       * is longer than the last matched length, trying to find the longest
       * mnt_dir for a given pwd_dir.
       *
       * (mnt_len == 0 && mnt->mnt_dir[0] == '/') special-cases the
       * root-dir /, which is the only mnt_dir with a trailing '/', which
       * got masked earlier.
       */
      if ((mnt_len > match_size || (mnt_len == 0 && mnt->mnt_dir[0] == '/')) &&
         (s = pam_str_skip_prefix_len(pwd->pw_dir, mnt->mnt_dir, mnt_len)) != NULL &&
         (s[0] == '\0' || s[0] == '/')) {
        free(mntdevice);
        if ((mntdevice = strdup(mnt->mnt_fsname)) == NULL) {
          pam_syslog(pamh, LOG_CRIT, "Memory allocation error");
          endmntent(fp);
          return PAM_PERM_DENIED;
        }
        match_size = mnt_len;
        if (param.debug >= 2)
          pam_syslog(pamh, LOG_DEBUG, "Found pw_dir=\"%s\" in mnt_dir=\"%s\" "
                     "with suffix=\"%s\" on device=\"%s\"", pwd->pw_dir,
                     mnt->mnt_dir, s, mntdevice);
      }
    /* param.fs has been specified, find exactly matching fileystem */
    } else if ((strncmp(param.fs, mnt->mnt_dir, param.fs_len) == 0
                && mnt->mnt_dir[param.fs_len] == '\0') ||
               (strncmp(param.fs, mnt->mnt_fsname, param.fs_len) == 0
                && mnt->mnt_fsname[param.fs_len] == '\0' )) {
        free(mntdevice);
        if ((mntdevice = strdup(mnt->mnt_fsname)) == NULL) {
          pam_syslog(pamh, LOG_CRIT, "Memory allocation error");
          endmntent(fp);
          return PAM_PERM_DENIED;
        }
        if (param.debug >= 2)
          pam_syslog(pamh, LOG_DEBUG, "Found fs=\"%s\" in mnt_dir=\"%s\" "
                     "on device=\"%s\"", param.fs, mnt->mnt_dir, mntdevice);
    }
  }

  endmntent(fp);

  if (mntdevice == NULL) {
    pam_syslog(pamh, LOG_ERR, "Filesystem or device not found: %s", param.fs ? param.fs : pwd->pw_dir);
    return PAM_PERM_DENIED;
  }

  /* Get limits */
  if (quotactl(QCMD(Q_GETQUOTA, USRQUOTA), mntdevice, pwd->pw_uid,
               (void *)&ndqblk) == -1) {
    pam_syslog(pamh, LOG_ERR, "fail to get limits for user %s : %m",
               pwd->pw_name);
    free(mntdevice);
    return PAM_PERM_DENIED;
  }

  if (param.debug >= 1)
    debug(pamh, &ndqblk, mntdevice, "Quota read:");

  /* Only overwrite if quotas aren't already set or if overwrite is set */
  if ((ndqblk.dqb_bsoftlimit == 0 && ndqblk.dqb_bhardlimit == 0 &&
       ndqblk.dqb_isoftlimit == 0 && ndqblk.dqb_ihardlimit == 0) ||
      param.overwrite == 1) {

    /* Parse new limits
     * Exit with an error should only the hard- or softlimit be
     * configured but not both.
     * This avoids errors, inconsistencies and possible race conditons
     * during setquota.
     */
    ndqblk.dqb_valid = 0;
    if (!parse_dqblk(pamh, argc, argv, &ndqblk)) {
      pam_syslog(pamh, LOG_ERR,
                 "Both soft- and hardlimits for %s need to be configured "
                 "at the same time!", mntdevice);
      free(mntdevice);
      return PAM_PERM_DENIED;
    }

    /* Nothing changed? Are no limits defined at all in configuration? */
    if (ndqblk.dqb_valid == 0) {
      pam_syslog(pamh, LOG_AUTH | LOG_WARNING, "no limits defined in "
                 "configuration for user %s on %s", pwd->pw_name, mntdevice);
      free(mntdevice);
      return PAM_IGNORE;
    }

    /* Set limits */
    if (quotactl(QCMD(Q_SETQUOTA, USRQUOTA), mntdevice, pwd->pw_uid,
                 (void *)&ndqblk) == -1) {
      pam_syslog(pamh, LOG_ERR, "failed to set limits for user %s on %s: %m",
                 pwd->pw_name, mntdevice);
      free(mntdevice);
      return PAM_PERM_DENIED;
    }
    if (param.debug >= 1)
      debug(pamh, &ndqblk, mntdevice, "Quota set:");

    /* End module */
    free(mntdevice);
    return PAM_SUCCESS;

  } else {
    /* Quota exists and overwrite!=1 */
    if (param.debug >= 1) {
      pam_syslog(pamh, LOG_DEBUG, "Quota already exists for user %s "
                 "on %s, not overwriting it without \"overwrite=1\"",
                 pwd->pw_name, mntdevice);
    }
    /* End module */
    free(mntdevice);
    return PAM_IGNORE;
  }

}