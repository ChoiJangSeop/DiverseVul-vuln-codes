GST_START_TEST (test_GstDateTime_iso8601)
{
  GstDateTime *dt, *dt2;
  gchar *str, *str2;
  GDateTime *gdt, *gdt2;

  dt = gst_date_time_new_now_utc ();
  fail_unless (gst_date_time_has_year (dt));
  fail_unless (gst_date_time_has_month (dt));
  fail_unless (gst_date_time_has_day (dt));
  fail_unless (gst_date_time_has_time (dt));
  fail_unless (gst_date_time_has_second (dt));
  str = gst_date_time_to_iso8601_string (dt);
  fail_unless (str != NULL);
  fail_unless_equals_int (strlen (str), strlen ("2012-06-26T22:46:43Z"));
  fail_unless (g_str_has_suffix (str, "Z"));
  dt2 = gst_date_time_new_from_iso8601_string (str);
  fail_unless (gst_date_time_get_year (dt) == gst_date_time_get_year (dt2));
  fail_unless (gst_date_time_get_month (dt) == gst_date_time_get_month (dt2));
  fail_unless (gst_date_time_get_day (dt) == gst_date_time_get_day (dt2));
  fail_unless (gst_date_time_get_hour (dt) == gst_date_time_get_hour (dt2));
  fail_unless (gst_date_time_get_minute (dt) == gst_date_time_get_minute (dt2));
  fail_unless (gst_date_time_get_second (dt) == gst_date_time_get_second (dt2));
  /* This will succeed because we're not comparing microseconds when
   * checking for equality */
  fail_unless (date_times_are_equal (dt, dt2));
  str2 = gst_date_time_to_iso8601_string (dt2);
  fail_unless_equals_string (str, str2);
  g_free (str2);
  gst_date_time_unref (dt2);
  g_free (str);
  gst_date_time_unref (dt);

  /* ---- year only ---- */

  dt = gst_date_time_new_y (2010);
  fail_unless (gst_date_time_has_year (dt));
  fail_unless (!gst_date_time_has_month (dt));
  fail_unless (!gst_date_time_has_day (dt));
  fail_unless (!gst_date_time_has_time (dt));
  fail_unless (!gst_date_time_has_second (dt));
  str = gst_date_time_to_iso8601_string (dt);
  fail_unless (str != NULL);
  fail_unless_equals_string (str, "2010");
  dt2 = gst_date_time_new_from_iso8601_string (str);
  fail_unless (gst_date_time_get_year (dt) == gst_date_time_get_year (dt2));
  fail_unless (date_times_are_equal (dt, dt2));
  str2 = gst_date_time_to_iso8601_string (dt2);
  fail_unless_equals_string (str, str2);
  g_free (str2);
  gst_date_time_unref (dt2);
  g_free (str);
  gst_date_time_unref (dt);

  /* ---- year and month ---- */

  dt = gst_date_time_new_ym (2010, 10);
  fail_unless (gst_date_time_has_year (dt));
  fail_unless (gst_date_time_has_month (dt));
  fail_unless (!gst_date_time_has_day (dt));
  fail_unless (!gst_date_time_has_time (dt));
  fail_unless (!gst_date_time_has_second (dt));
  str = gst_date_time_to_iso8601_string (dt);
  fail_unless (str != NULL);
  fail_unless_equals_string (str, "2010-10");
  dt2 = gst_date_time_new_from_iso8601_string (str);
  fail_unless (gst_date_time_get_year (dt) == gst_date_time_get_year (dt2));
  fail_unless (gst_date_time_get_month (dt) == gst_date_time_get_month (dt2));
  fail_unless (date_times_are_equal (dt, dt2));
  str2 = gst_date_time_to_iso8601_string (dt2);
  fail_unless_equals_string (str, str2);
  g_free (str2);
  gst_date_time_unref (dt2);
  g_free (str);
  gst_date_time_unref (dt);

  /* ---- year and month ---- */

  dt = gst_date_time_new_ymd (2010, 10, 30);
  fail_unless (gst_date_time_has_year (dt));
  fail_unless (gst_date_time_has_month (dt));
  fail_unless (gst_date_time_has_day (dt));
  fail_unless (!gst_date_time_has_time (dt));
  fail_unless (!gst_date_time_has_second (dt));
  str = gst_date_time_to_iso8601_string (dt);
  fail_unless (str != NULL);
  fail_unless_equals_string (str, "2010-10-30");
  dt2 = gst_date_time_new_from_iso8601_string (str);
  fail_unless (gst_date_time_get_year (dt) == gst_date_time_get_year (dt2));
  fail_unless (gst_date_time_get_month (dt) == gst_date_time_get_month (dt2));
  fail_unless (gst_date_time_get_day (dt) == gst_date_time_get_day (dt2));
  fail_unless (date_times_are_equal (dt, dt2));
  str2 = gst_date_time_to_iso8601_string (dt2);
  fail_unless_equals_string (str, str2);
  g_free (str2);
  gst_date_time_unref (dt2);
  g_free (str);
  gst_date_time_unref (dt);

  /* ---- date and time, but no seconds ---- */

  dt = gst_date_time_new (-4.5, 2010, 10, 30, 15, 50, -1);
  fail_unless (gst_date_time_has_year (dt));
  fail_unless (gst_date_time_has_month (dt));
  fail_unless (gst_date_time_has_day (dt));
  fail_unless (gst_date_time_has_time (dt));
  fail_unless (!gst_date_time_has_second (dt));
  str = gst_date_time_to_iso8601_string (dt);
  fail_unless (str != NULL);
  fail_unless_equals_string (str, "2010-10-30T15:50-0430");
  dt2 = gst_date_time_new_from_iso8601_string (str);
  fail_unless (gst_date_time_get_year (dt) == gst_date_time_get_year (dt2));
  fail_unless (gst_date_time_get_month (dt) == gst_date_time_get_month (dt2));
  fail_unless (gst_date_time_get_day (dt) == gst_date_time_get_day (dt2));
  fail_unless (gst_date_time_get_hour (dt) == gst_date_time_get_hour (dt2));
  fail_unless (gst_date_time_get_minute (dt) == gst_date_time_get_minute (dt2));
  fail_unless (date_times_are_equal (dt, dt2));
  str2 = gst_date_time_to_iso8601_string (dt2);
  fail_unless_equals_string (str, str2);
  g_free (str2);
  gst_date_time_unref (dt2);
  g_free (str);
  gst_date_time_unref (dt);

  /* ---- date and time, but no seconds (UTC) ---- */

  dt = gst_date_time_new (0, 2010, 10, 30, 15, 50, -1);
  fail_unless (gst_date_time_has_year (dt));
  fail_unless (gst_date_time_has_month (dt));
  fail_unless (gst_date_time_has_day (dt));
  fail_unless (gst_date_time_has_time (dt));
  fail_unless (!gst_date_time_has_second (dt));
  str = gst_date_time_to_iso8601_string (dt);
  fail_unless (str != NULL);
  fail_unless_equals_string (str, "2010-10-30T15:50Z");
  dt2 = gst_date_time_new_from_iso8601_string (str);
  fail_unless (gst_date_time_get_year (dt) == gst_date_time_get_year (dt2));
  fail_unless (gst_date_time_get_month (dt) == gst_date_time_get_month (dt2));
  fail_unless (gst_date_time_get_day (dt) == gst_date_time_get_day (dt2));
  fail_unless (gst_date_time_get_hour (dt) == gst_date_time_get_hour (dt2));
  fail_unless (gst_date_time_get_minute (dt) == gst_date_time_get_minute (dt2));
  fail_unless (date_times_are_equal (dt, dt2));
  str2 = gst_date_time_to_iso8601_string (dt2);
  fail_unless_equals_string (str, str2);
  g_free (str2);
  gst_date_time_unref (dt2);
  g_free (str);
  gst_date_time_unref (dt);

  /* ---- date and time, with seconds ---- */

  dt = gst_date_time_new (-4.5, 2010, 10, 30, 15, 50, 0);
  fail_unless (gst_date_time_has_year (dt));
  fail_unless (gst_date_time_has_month (dt));
  fail_unless (gst_date_time_has_day (dt));
  fail_unless (gst_date_time_has_time (dt));
  fail_unless (gst_date_time_has_second (dt));
  str = gst_date_time_to_iso8601_string (dt);
  fail_unless (str != NULL);
  fail_unless_equals_string (str, "2010-10-30T15:50:00-0430");
  dt2 = gst_date_time_new_from_iso8601_string (str);
  fail_unless (gst_date_time_get_year (dt) == gst_date_time_get_year (dt2));
  fail_unless (gst_date_time_get_month (dt) == gst_date_time_get_month (dt2));
  fail_unless (gst_date_time_get_day (dt) == gst_date_time_get_day (dt2));
  fail_unless (gst_date_time_get_hour (dt) == gst_date_time_get_hour (dt2));
  fail_unless (gst_date_time_get_minute (dt) == gst_date_time_get_minute (dt2));
  fail_unless (date_times_are_equal (dt, dt2));
  str2 = gst_date_time_to_iso8601_string (dt2);
  fail_unless_equals_string (str, str2);
  g_free (str2);
  gst_date_time_unref (dt2);
  g_free (str);
  gst_date_time_unref (dt);

  /* ---- date and time, with seconds (UTC) ---- */

  dt = gst_date_time_new (0, 2010, 10, 30, 15, 50, 0);
  fail_unless (gst_date_time_has_year (dt));
  fail_unless (gst_date_time_has_month (dt));
  fail_unless (gst_date_time_has_day (dt));
  fail_unless (gst_date_time_has_time (dt));
  fail_unless (gst_date_time_has_second (dt));
  str = gst_date_time_to_iso8601_string (dt);
  fail_unless (str != NULL);
  fail_unless_equals_string (str, "2010-10-30T15:50:00Z");
  dt2 = gst_date_time_new_from_iso8601_string (str);
  fail_unless (gst_date_time_get_year (dt) == gst_date_time_get_year (dt2));
  fail_unless (gst_date_time_get_month (dt) == gst_date_time_get_month (dt2));
  fail_unless (gst_date_time_get_day (dt) == gst_date_time_get_day (dt2));
  fail_unless (gst_date_time_get_hour (dt) == gst_date_time_get_hour (dt2));
  fail_unless (gst_date_time_get_minute (dt) == gst_date_time_get_minute (dt2));
  fail_unless (date_times_are_equal (dt, dt2));
  str2 = gst_date_time_to_iso8601_string (dt2);
  fail_unless_equals_string (str, str2);
  g_free (str2);
  gst_date_time_unref (dt2);
  g_free (str);
  gst_date_time_unref (dt);

  /* ---- date and time, but without the 'T' and without timezone */
  dt = gst_date_time_new_from_iso8601_string ("2010-10-30 15:50");
  fail_unless (gst_date_time_get_year (dt) == 2010);
  fail_unless (gst_date_time_get_month (dt) == 10);
  fail_unless (gst_date_time_get_day (dt) == 30);
  fail_unless (gst_date_time_get_hour (dt) == 15);
  fail_unless (gst_date_time_get_minute (dt) == 50);
  fail_unless (!gst_date_time_has_second (dt));
  gst_date_time_unref (dt);

  /* ---- date and time+secs, but without the 'T' and without timezone */
  dt = gst_date_time_new_from_iso8601_string ("2010-10-30 15:50:33");
  fail_unless (gst_date_time_get_year (dt) == 2010);
  fail_unless (gst_date_time_get_month (dt) == 10);
  fail_unless (gst_date_time_get_day (dt) == 30);
  fail_unless (gst_date_time_get_hour (dt) == 15);
  fail_unless (gst_date_time_get_minute (dt) == 50);
  fail_unless (gst_date_time_get_second (dt) == 33);
  gst_date_time_unref (dt);

  /* ---- dates with 00s */
  dt = gst_date_time_new_from_iso8601_string ("2010-10-00");
  fail_unless (gst_date_time_get_year (dt) == 2010);
  fail_unless (gst_date_time_get_month (dt) == 10);
  fail_unless (!gst_date_time_has_day (dt));
  fail_unless (!gst_date_time_has_time (dt));
  gst_date_time_unref (dt);

  dt = gst_date_time_new_from_iso8601_string ("2010-00-00");
  fail_unless (gst_date_time_get_year (dt) == 2010);
  fail_unless (!gst_date_time_has_month (dt));
  fail_unless (!gst_date_time_has_day (dt));
  fail_unless (!gst_date_time_has_time (dt));
  gst_date_time_unref (dt);

  dt = gst_date_time_new_from_iso8601_string ("2010-00-30");
  fail_unless (gst_date_time_get_year (dt) == 2010);
  fail_unless (!gst_date_time_has_month (dt));
  fail_unless (!gst_date_time_has_day (dt));
  fail_unless (!gst_date_time_has_time (dt));
  gst_date_time_unref (dt);

  /* completely invalid */
  dt = gst_date_time_new_from_iso8601_string ("0000-00-00");
  fail_unless (dt == NULL);

  /* partially invalid - here we'll just extract the year */
  dt = gst_date_time_new_from_iso8601_string ("2010/05/30");
  fail_unless (gst_date_time_get_year (dt) == 2010);
  fail_unless (!gst_date_time_has_month (dt));
  fail_unless (!gst_date_time_has_day (dt));
  fail_unless (!gst_date_time_has_time (dt));
  gst_date_time_unref (dt);


  /* only time provided - we assume today's date */
  gdt = g_date_time_new_now_utc ();

  dt = gst_date_time_new_from_iso8601_string ("15:50:33");
  fail_unless (gst_date_time_get_year (dt) == g_date_time_get_year (gdt));
  fail_unless (gst_date_time_get_month (dt) == g_date_time_get_month (gdt));
  fail_unless (gst_date_time_get_day (dt) ==
      g_date_time_get_day_of_month (gdt));
  fail_unless (gst_date_time_get_hour (dt) == 15);
  fail_unless (gst_date_time_get_minute (dt) == 50);
  fail_unless (gst_date_time_get_second (dt) == 33);
  gst_date_time_unref (dt);

  dt = gst_date_time_new_from_iso8601_string ("15:50:33Z");
  fail_unless (gst_date_time_get_year (dt) == g_date_time_get_year (gdt));
  fail_unless (gst_date_time_get_month (dt) == g_date_time_get_month (gdt));
  fail_unless (gst_date_time_get_day (dt) ==
      g_date_time_get_day_of_month (gdt));
  fail_unless (gst_date_time_get_hour (dt) == 15);
  fail_unless (gst_date_time_get_minute (dt) == 50);
  fail_unless (gst_date_time_get_second (dt) == 33);
  gst_date_time_unref (dt);

  dt = gst_date_time_new_from_iso8601_string ("15:50");
  fail_unless (gst_date_time_get_year (dt) == g_date_time_get_year (gdt));
  fail_unless (gst_date_time_get_month (dt) == g_date_time_get_month (gdt));
  fail_unless (gst_date_time_get_day (dt) ==
      g_date_time_get_day_of_month (gdt));
  fail_unless (gst_date_time_get_hour (dt) == 15);
  fail_unless (gst_date_time_get_minute (dt) == 50);
  fail_unless (!gst_date_time_has_second (dt));
  gst_date_time_unref (dt);

  dt = gst_date_time_new_from_iso8601_string ("15:50Z");
  fail_unless (gst_date_time_get_year (dt) == g_date_time_get_year (gdt));
  fail_unless (gst_date_time_get_month (dt) == g_date_time_get_month (gdt));
  fail_unless (gst_date_time_get_day (dt) ==
      g_date_time_get_day_of_month (gdt));
  fail_unless (gst_date_time_get_hour (dt) == 15);
  fail_unless (gst_date_time_get_minute (dt) == 50);
  fail_unless (!gst_date_time_has_second (dt));
  gst_date_time_unref (dt);

  gdt2 = g_date_time_add_minutes (gdt, -270);
  g_date_time_unref (gdt);

  dt = gst_date_time_new_from_iso8601_string ("15:50:33-0430");
  fail_unless (gst_date_time_get_year (dt) == g_date_time_get_year (gdt2));
  fail_unless (gst_date_time_get_month (dt) == g_date_time_get_month (gdt2));
  fail_unless (gst_date_time_get_day (dt) ==
      g_date_time_get_day_of_month (gdt2));
  fail_unless (gst_date_time_get_hour (dt) == 15);
  fail_unless (gst_date_time_get_minute (dt) == 50);
  fail_unless (gst_date_time_get_second (dt) == 33);
  gst_date_time_unref (dt);

  dt = gst_date_time_new_from_iso8601_string ("15:50-0430");
  fail_unless (gst_date_time_get_year (dt) == g_date_time_get_year (gdt2));
  fail_unless (gst_date_time_get_month (dt) == g_date_time_get_month (gdt2));
  fail_unless (gst_date_time_get_day (dt) ==
      g_date_time_get_day_of_month (gdt2));
  fail_unless (gst_date_time_get_hour (dt) == 15);
  fail_unless (gst_date_time_get_minute (dt) == 50);
  fail_unless (!gst_date_time_has_second (dt));
  gst_date_time_unref (dt);

  g_date_time_unref (gdt2);
}