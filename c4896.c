dirserv_get_status_impl(const char *id_digest, const char *nickname,
                        uint32_t addr, uint16_t or_port,
                        const char *platform, const char **msg, int severity)
{
  uint32_t result = 0;
  router_status_t *status_by_digest;

  if (!fingerprint_list)
    fingerprint_list = authdir_config_new();

  log_debug(LD_DIRSERV, "%d fingerprints, %d digests known.",
            strmap_size(fingerprint_list->fp_by_name),
            digestmap_size(fingerprint_list->status_by_digest));

  /* Versions before Tor 0.2.4.18-rc are too old to support, and are
   * missing some important security fixes too. Disable them. */
  if (platform && !tor_version_as_new_as(platform,"0.2.4.18-rc")) {
    if (msg)
      *msg = "Tor version is insecure or unsupported. Please upgrade!";
    return FP_REJECT;
  }

  status_by_digest = digestmap_get(fingerprint_list->status_by_digest,
                                   id_digest);
  if (status_by_digest)
    result |= *status_by_digest;

  if (result & FP_REJECT) {
    if (msg)
      *msg = "Fingerprint is marked rejected -- please contact us?";
    return FP_REJECT;
  } else if (result & FP_INVALID) {
    if (msg)
      *msg = "Fingerprint is marked invalid";
  }

  if (authdir_policy_badexit_address(addr, or_port)) {
    log_fn(severity, LD_DIRSERV,
           "Marking '%s' as bad exit because of address '%s'",
               nickname, fmt_addr32(addr));
    result |= FP_BADEXIT;
  }

  if (!authdir_policy_permits_address(addr, or_port)) {
    log_fn(severity, LD_DIRSERV, "Rejecting '%s' because of address '%s'",
               nickname, fmt_addr32(addr));
    if (msg)
      *msg = "Suspicious relay address range -- please contact us?";
    return FP_REJECT;
  }
  if (!authdir_policy_valid_address(addr, or_port)) {
    log_fn(severity, LD_DIRSERV,
           "Not marking '%s' valid because of address '%s'",
               nickname, fmt_addr32(addr));
    result |= FP_INVALID;
  }

  return result;
}