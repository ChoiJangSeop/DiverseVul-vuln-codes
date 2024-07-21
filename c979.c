tor_timegm(struct tm *tm)
{
  /* This is a pretty ironclad timegm implementation, snarfed from Python2.2.
   * It's way more brute-force than fiddling with tzset().
   */
  time_t year, days, hours, minutes, seconds;
  int i;
  year = tm->tm_year + 1900;
  if (year < 1970 || tm->tm_mon < 0 || tm->tm_mon > 11) {
    log_warn(LD_BUG, "Out-of-range argument to tor_timegm");
    return -1;
  }
  tor_assert(year < INT_MAX);
  days = 365 * (year-1970) + n_leapdays(1970,(int)year);
  for (i = 0; i < tm->tm_mon; ++i)
    days += days_per_month[i];
  if (tm->tm_mon > 1 && IS_LEAPYEAR(year))
    ++days;
  days += tm->tm_mday - 1;
  hours = days*24 + tm->tm_hour;

  minutes = hours*60 + tm->tm_min;
  seconds = minutes*60 + tm->tm_sec;
  return seconds;
}