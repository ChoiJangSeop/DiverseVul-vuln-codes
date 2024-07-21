update_drift_file(double freq_ppm, double skew)
{
  char *temp_drift_file;
  FILE *out;
  int r1, r2;

  /* Create a temporary file with a '.tmp' extension. */

  temp_drift_file = (char*) Malloc(strlen(drift_file)+8);

  if(!temp_drift_file) {
    return;
  }

  strcpy(temp_drift_file,drift_file);
  strcat(temp_drift_file,".tmp");

  out = fopen(temp_drift_file, "w");
  if (!out) {
    Free(temp_drift_file);
    LOG(LOGS_WARN, "Could not open temporary driftfile %s.tmp for writing",
        drift_file);
    return;
  }

  /* Write the frequency and skew parameters in ppm */
  r1 = fprintf(out, "%20.6f %20.6f\n", freq_ppm, 1.0e6 * skew);
  r2 = fclose(out);
  if (r1 < 0 || r2) {
    Free(temp_drift_file);
    LOG(LOGS_WARN, "Could not write to temporary driftfile %s.tmp",
        drift_file);
    return;
  }

  /* Rename the temporary file to the correct location (see rename(2) for details). */

  if (rename(temp_drift_file,drift_file)) {
    unlink(temp_drift_file);
    Free(temp_drift_file);
    LOG(LOGS_WARN, "Could not replace old driftfile %s with new one %s.tmp",
        drift_file,drift_file);
    return;
  }

  Free(temp_drift_file);
}