write_coefs_to_file(int valid,time_t ref_time,double offset,double rate)
{
  char *temp_coefs_file_name;
  FILE *out;
  int r1, r2;

  /* Create a temporary file with a '.tmp' extension. */

  temp_coefs_file_name = (char*) Malloc(strlen(coefs_file_name)+8);

  if(!temp_coefs_file_name) {
    return RTC_ST_BADFILE;
  }

  strcpy(temp_coefs_file_name,coefs_file_name);
  strcat(temp_coefs_file_name,".tmp");

  out = fopen(temp_coefs_file_name, "w");
  if (!out) {
    Free(temp_coefs_file_name);
    LOG(LOGS_WARN, "Could not open temporary RTC file %s.tmp for writing",
        coefs_file_name);
    return RTC_ST_BADFILE;
  }

  /* Gain rate is written out in ppm */
  r1 = fprintf(out, "%1d %ld %.6f %.3f\n",
               valid, ref_time, offset, 1.0e6 * rate);
  r2 = fclose(out);
  if (r1 < 0 || r2) {
    Free(temp_coefs_file_name);
    LOG(LOGS_WARN, "Could not write to temporary RTC file %s.tmp",
        coefs_file_name);
    return RTC_ST_BADFILE;
  }

  /* Rename the temporary file to the correct location (see rename(2) for details). */

  if (rename(temp_coefs_file_name,coefs_file_name)) {
    unlink(temp_coefs_file_name);
    Free(temp_coefs_file_name);
    LOG(LOGS_WARN, "Could not replace old RTC file %s.tmp with new one %s",
        coefs_file_name, coefs_file_name);
    return RTC_ST_BADFILE;
  }

  Free(temp_coefs_file_name);

  return RTC_ST_OK;
}