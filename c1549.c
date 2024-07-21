int dane_state_set_dlv_file(dane_state_t s, const char* file)
{
int ret;
  
  ret = ub_ctx_set_option(s->ctx, (char*)"dlv-anchor-file:", (void*)file);
  if (ret != 0)
    return gnutls_assert_val(DANE_E_FILE_ERROR);
  
  return 0;
}