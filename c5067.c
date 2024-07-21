static void process_event_record(MYSQL_THD thd, LEX_CSTRING event_name,
                                 const char *data, size_t data_length)
{
  const char *record_str = (const char *)THDVAR(thd, event_record_def);
  LEX_CSTRING record_begin = get_token(&record_str);
  LEX_CSTRING record_end = get_token(&record_str);

  if (record_str == NULL)
  {
    return;
  }

  if (record_end.length == 0)
  {
    /* We are already in the consuming phase. Add a new event name into
       a record variable */

    const char *buffer= THDVAR(thd, event_record);

    /* Add event. */
    add_event(thd, buffer, event_name, data, data_length);


    if (!my_charset_latin1.coll->strnncoll(&my_charset_latin1,
                                           (const uchar *)record_begin.str,
                                           record_begin.length,
                                           (const uchar *)event_name.str,
                                           event_name.length, FALSE))
    {
      /* Do not expect any more events. */
      THDVAR(thd, event_record_def)= 0;
    }
  }
  else
  {
    /* We have not started recording of events yet. */
    if (my_charset_latin1.coll->strnncoll(&my_charset_latin1,
                                          (const uchar *)record_begin.str,
                                          record_begin.length,
                                          (const uchar *)event_name.str,
                                          event_name.length, FALSE))
    {
      /* Event not matching. */
      return;
    }

    THDVAR(thd, event_record)= 0;

    add_event(thd, "", event_name, data, data_length);

    /* Add event. */

    record_str = (const char *)THDVAR(thd, event_record_def);

    /* Remove starting event. */
    memmove((char *)record_str, (void *)record_end.str, record_end.length + 1);
  }
}