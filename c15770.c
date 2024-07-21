extract_lladdr_from_llao_aligned(uip_lladdr_t *dest) {
  if(dest != NULL && nd6_opt_llao != NULL) {
    memcpy(dest, &nd6_opt_llao[UIP_ND6_OPT_DATA_OFFSET], UIP_LLADDR_LEN);
    return 1;
  }
  return 0;
}