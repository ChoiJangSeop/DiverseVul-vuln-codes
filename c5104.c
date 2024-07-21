save_abbr (timezone_t tz, struct tm *tm)
{
#if HAVE_TM_ZONE || HAVE_TZNAME
  char const *zone = NULL;
  char *zone_copy = (char *) "";

# if HAVE_TZNAME
  int tzname_index = -1;
# endif

# if HAVE_TM_ZONE
  zone = tm->tm_zone;
# endif

# if HAVE_TZNAME
  if (! (zone && *zone) && 0 <= tm->tm_isdst)
    {
      tzname_index = tm->tm_isdst != 0;
      zone = tzname[tzname_index];
    }
# endif

  /* No need to replace null zones, or zones within the struct tm.  */
  if (!zone || ((char *) tm <= zone && zone < (char *) (tm + 1)))
    return true;

  if (*zone)
    {
      zone_copy = tz->abbrs;

      while (strcmp (zone_copy, zone) != 0)
        {
          if (! (*zone_copy || (zone_copy == tz->abbrs && tz->tz_is_set)))
            {
              size_t zone_size = strlen (zone) + 1;
              if (zone_size < tz->abbrs + ABBR_SIZE_MIN - zone_copy)
                extend_abbrs (zone_copy, zone, zone_size);
              else
                {
                  tz = tz->next = tzalloc (zone);
                  if (!tz)
                    return false;
                  tz->tz_is_set = 0;
                  zone_copy = tz->abbrs;
                }
              break;
            }

          zone_copy += strlen (zone_copy) + 1;
          if (!*zone_copy && tz->next)
            {
              tz = tz->next;
              zone_copy = tz->abbrs;
            }
        }
    }

  /* Replace the zone name so that its lifetime matches that of TZ.  */
# if HAVE_TM_ZONE
  tm->tm_zone = zone_copy;
# else
  if (0 <= tzname_index)
    tz->tzname_copy[tzname_index] = zone_copy;
# endif
#endif

  return true;
}