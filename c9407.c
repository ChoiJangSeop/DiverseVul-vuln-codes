REF_Initialise(void)
{
  FILE *in;
  double file_freq_ppm, file_skew_ppm;
  double our_frequency_ppm;
  int tai_offset;

  mode = REF_ModeNormal;
  are_we_synchronised = 0;
  our_leap_status = LEAP_Unsynchronised;
  our_leap_sec = 0;
  our_tai_offset = 0;
  initialised = 1;
  our_root_dispersion = 1.0;
  our_root_delay = 1.0;
  our_frequency_ppm = 0.0;
  our_skew = 1.0; /* i.e. rather bad */
  our_residual_freq = 0.0;
  drift_file_age = 0.0;

  /* Now see if we can get the drift file opened */
  drift_file = CNF_GetDriftFile();
  if (drift_file) {
    in = fopen(drift_file, "r");
    if (in) {
      if (fscanf(in, "%lf%lf", &file_freq_ppm, &file_skew_ppm) == 2) {
        /* We have read valid data */
        our_frequency_ppm = file_freq_ppm;
        our_skew = 1.0e-6 * file_skew_ppm;
        if (our_skew < MIN_SKEW)
          our_skew = MIN_SKEW;
        LOG(LOGS_INFO, "Frequency %.3f +/- %.3f ppm read from %s",
            file_freq_ppm, file_skew_ppm, drift_file);
        LCL_SetAbsoluteFrequency(our_frequency_ppm);
      } else {
        LOG(LOGS_WARN, "Could not read valid frequency and skew from driftfile %s",
            drift_file);
      }
      fclose(in);
    }
  }
    
  if (our_frequency_ppm == 0.0) {
    our_frequency_ppm = LCL_ReadAbsoluteFrequency();
    if (our_frequency_ppm != 0.0) {
      LOG(LOGS_INFO, "Initial frequency %.3f ppm", our_frequency_ppm);
    }
  }

  logfileid = CNF_GetLogTracking() ? LOG_FileOpen("tracking",
      "   Date (UTC) Time     IP Address   St   Freq ppm   Skew ppm     Offset L Co  Offset sd Rem. corr. Root delay Root disp. Max. error")
    : -1;

  max_update_skew = fabs(CNF_GetMaxUpdateSkew()) * 1.0e-6;

  correction_time_ratio = CNF_GetCorrectionTimeRatio();

  enable_local_stratum = CNF_AllowLocalReference(&local_stratum, &local_orphan, &local_distance);
  UTI_ZeroTimespec(&local_ref_time);

  leap_timeout_id = 0;
  leap_in_progress = 0;
  leap_mode = CNF_GetLeapSecMode();
  /* Switch to step mode if the system driver doesn't support leap */
  if (leap_mode == REF_LeapModeSystem && !LCL_CanSystemLeap())
    leap_mode = REF_LeapModeStep;

  leap_tzname = CNF_GetLeapSecTimezone();
  if (leap_tzname) {
    /* Check that the timezone has good data for Jun 30 2012 and Dec 31 2012 */
    if (get_tz_leap(1341014400, &tai_offset) == LEAP_InsertSecond && tai_offset == 34 &&
        get_tz_leap(1356912000, &tai_offset) == LEAP_Normal && tai_offset == 35) {
      LOG(LOGS_INFO, "Using %s timezone to obtain leap second data", leap_tzname);
    } else {
      LOG(LOGS_WARN, "Timezone %s failed leap second check, ignoring", leap_tzname);
      leap_tzname = NULL;
    }
  }

  CNF_GetMakeStep(&make_step_limit, &make_step_threshold);
  CNF_GetMaxChange(&max_offset_delay, &max_offset_ignore, &max_offset);
  CNF_GetMailOnChange(&do_mail_change, &mail_change_threshold, &mail_change_user);
  log_change_threshold = CNF_GetLogChange();

  CNF_GetFallbackDrifts(&fb_drift_min, &fb_drift_max);

  if (fb_drift_max >= fb_drift_min && fb_drift_min > 0) {
    fb_drifts = MallocArray(struct fb_drift, fb_drift_max - fb_drift_min + 1);
    memset(fb_drifts, 0, sizeof (struct fb_drift) * (fb_drift_max - fb_drift_min + 1));
    next_fb_drift = 0;
    fb_drift_timeout_id = 0;
  }

  UTI_ZeroTimespec(&our_ref_time);
  UTI_ZeroTimespec(&last_ref_update);
  last_ref_update_interval = 0.0;

  LCL_AddParameterChangeHandler(handle_slew, NULL);

  /* Make first entry in tracking log */
  REF_SetUnsynchronised();
}