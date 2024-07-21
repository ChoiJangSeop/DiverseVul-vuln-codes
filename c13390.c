static int DoPamAuth(struct pam_handle* pamh,
                     const char* username,
                     std::string& authenticated_username)
{
  int err = pam_set_item(pamh, PAM_RUSER, username);
  if (err != PAM_SUCCESS) {
    Dmsg1(debuglevel, "PAM set_item failed: %s\n", pam_strerror(pamh, err));
    return err;
  }

  err = pam_authenticate(pamh, 0);
  if (err != PAM_SUCCESS) {
    Dmsg1(debuglevel, "PAM authentication failed: %s\n",
          pam_strerror(pamh, err));
    return err;
  }

#if defined(__sun)
  void* data;
#else
  const void* data;
#endif
  err = pam_get_item(pamh, PAM_USER, &data);
  if (err != PAM_SUCCESS) {
    Dmsg1(debuglevel, "PAM get_item failed: %s\n", pam_strerror(pamh, err));
    return err;
  } else {
    if (data) { authenticated_username = static_cast<const char*>(data); }
  }
  return err;
}