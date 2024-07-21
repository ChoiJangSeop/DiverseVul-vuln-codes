read_timeout(void *arg)
{
  FILE *f;
  double temp, comp;

  f = fopen(filename, "r");

  if (f && fscanf(f, "%lf", &temp) == 1) {
    comp = get_tempcomp(temp);

    if (fabs(comp) <= MAX_COMP) {
      comp = LCL_SetTempComp(comp);

      DEBUG_LOG("tempcomp updated to %f for %f", comp, temp);

      if (logfileid != -1) {
        struct timespec now;

        LCL_ReadCookedTime(&now, NULL);
        LOG_FileWrite(logfileid, "%s %11.4e %11.4e",
            UTI_TimeToLogForm(now.tv_sec), temp, comp);
      }
    } else {
      LOG(LOGS_WARN, "Temperature compensation of %.3f ppm exceeds sanity limit of %.1f",
          comp, MAX_COMP);
    }
  } else {
    LOG(LOGS_WARN, "Could not read temperature from %s", filename);
  }

  if (f)
    fclose(f);

  timeout_id = SCH_AddTimeoutByDelay(update_interval, read_timeout, NULL);
}