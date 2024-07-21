acquire_caps (uid_t uid)
{
  struct __user_cap_header_struct hdr;
  struct __user_cap_data_struct data;

  /* Tell kernel not clear capabilities when dropping root */
  if (prctl (PR_SET_KEEPCAPS, 1, 0, 0, 0) < 0)
    g_error ("prctl(PR_SET_KEEPCAPS) failed");

  /* Drop root uid, but retain the required permitted caps */
  if (setuid (uid) < 0)
    g_error ("unable to drop privs");

  memset (&hdr, 0, sizeof(hdr));
  hdr.version = _LINUX_CAPABILITY_VERSION;

  /* Drop all non-require capabilities */
  data.effective = REQUIRED_CAPS;
  data.permitted = REQUIRED_CAPS;
  data.inheritable = 0;
  if (capset (&hdr, &data) < 0)
    g_error ("capset failed");
}