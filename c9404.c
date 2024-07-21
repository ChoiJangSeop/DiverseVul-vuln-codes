FILE *open_dumpfile(SRC_Instance inst, const char *mode)
{
  FILE *f;
  char filename[PATH_MAX], *dumpdir;

  dumpdir = CNF_GetDumpDir();
  if (dumpdir[0] == '\0') {
    LOG(LOGS_WARN, "dumpdir not specified");
    return NULL;
  }

  /* Include IP address in the name for NTP sources, or reference ID in hex */
  if ((inst->type == SRC_NTP &&
       snprintf(filename, sizeof (filename), "%s/%s.dat", dumpdir,
                source_to_string(inst)) >= sizeof (filename)) ||
      (inst->type != SRC_NTP &&
       snprintf(filename, sizeof (filename), "%s/refid:%08"PRIx32".dat",
                dumpdir, inst->ref_id) >= sizeof (filename))) {
    LOG(LOGS_WARN, "dumpdir too long");
    return NULL;
  }

  f = fopen(filename, mode);
  if (!f && mode[0] != 'r')
    LOG(LOGS_WARN, "Could not open dump file for %s",
        source_to_string(inst));

  return f;
}