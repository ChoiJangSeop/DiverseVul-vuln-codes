static int pam_prompt(pam_handle_t *pamh, int style, char **response, char *fmt, ...)
{
  int rv;
  struct pam_conv *conv;
  struct pam_message msg;
  struct pam_response *resp;
  /* struct pam_message *(msgp[1]) = { &msg}; */
  struct pam_message *(msgp[1]);
  msgp[0] = &msg;
  va_list va;
  char text[128];

  va_start(va, fmt);
  vsnprintf(text, sizeof text, fmt, va);
  va_end(va);

  msg.msg_style = style;
  msg.msg = text;
  rv = pam_get_item(pamh, PAM_CONV, &conv);
  if (rv != PAM_SUCCESS)
    return rv;
  if ((conv == NULL) || (conv->conv == NULL))
    return PAM_CRED_INSUFFICIENT;
  rv = conv->conv(1, msgp, &resp, conv->appdata_ptr);
  if (rv != PAM_SUCCESS)
    return rv;
  if ((resp == NULL) || (resp[0].resp == NULL))
    return !response ? PAM_SUCCESS : PAM_CRED_INSUFFICIENT;
  if (response) {
     *response = strdup(resp[0].resp);
  }
  /* overwrite memory and release it */
  memset(resp[0].resp, 0, strlen(resp[0].resp));
  free(&resp[0]);
  return PAM_SUCCESS;
}