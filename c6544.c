int pkcs11_pass_login(pkcs11_handle_t *h, int nullok)
{
  int rv;
  char *pin;

  /* get password */
  pin =getpass("PIN for token: ");
#ifdef DEBUG_SHOW_PASSWORD
  DBG1("PIN = [%s]", pin);
#endif

  if (NULL == pin) {
    set_error("Error encountered while reading PIN");
    return -1;
  }

  /* for safety reasons, clean PIN string from memory asap */

  /* check password length */
  if (!nullok && strlen(pin) == 0) {
    set_error("Empty passwords not allowed");
    return -1;
  }

  /* perform pkcs #11 login */
  rv = pkcs11_login(h, pin);
  memset(pin, 0, strlen(pin));
  if (rv != 0) {
    set_error("pkcs11_login() failed: %s", get_error());
    return -1;
  }
  return 0;
}