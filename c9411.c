read_coefs_from_file(void)
{
  FILE *in;

  if (!tried_to_load_coefs) {

    valid_coefs_from_file = 0; /* only gets set true if we succeed */

    tried_to_load_coefs = 1;

    if (coefs_file_name && (in = fopen(coefs_file_name, "r"))) {
      if (fscanf(in, "%d%ld%lf%lf",
                 &valid_coefs_from_file,
                 &file_ref_time,
                 &file_ref_offset,
                 &file_rate_ppm) == 4) {
      } else {
        LOG(LOGS_WARN, "Could not read coefficients from %s", coefs_file_name);
      }
      fclose(in);
    }
  }
}