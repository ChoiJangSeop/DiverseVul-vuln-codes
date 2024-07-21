ecma_date_to_string_format (ecma_number_t datetime_number, /**< datetime */
                            const char *format_p) /**< format buffer */
{
  const uint32_t date_buffer_length = 37;
  JERRY_VLA (lit_utf8_byte_t, date_buffer, date_buffer_length);

  lit_utf8_byte_t *dest_p = date_buffer;

  while (*format_p != LIT_CHAR_NULL)
  {
    if (*format_p != LIT_CHAR_DOLLAR_SIGN)
    {
      *dest_p++ = (lit_utf8_byte_t) *format_p++;
      continue;
    }

    format_p++;

    const char *str_p = NULL;
    int32_t number = 0;
    int32_t number_length = 0;

    switch (*format_p)
    {
      case LIT_CHAR_UPPERCASE_Y: /* Year. */
      {
        number = ecma_date_year_from_time (datetime_number);

        if (number >= 100000 || number <= -100000)
        {
          number_length = 6;
        }
        else if (number >= 10000 || number <= -10000)
        {
          number_length = 5;
        }
        else
        {
          number_length = 4;
        }
        break;
      }
      case LIT_CHAR_LOWERCASE_Y: /* ISO Year: -000001, 0000, 0001, 9999, +012345 */
      {
        number = ecma_date_year_from_time (datetime_number);
        if (0 <= number && number <= 9999)
        {
          number_length = 4;
        }
        else
        {
          number_length = 6;
        }
        break;
      }
      case LIT_CHAR_UPPERCASE_M: /* Month. */
      {
        int32_t month = ecma_date_month_from_time (datetime_number);

        JERRY_ASSERT (month >= 0 && month <= 11);

        str_p = month_names_p[month];
        break;
      }
      case LIT_CHAR_UPPERCASE_O: /* Month as number. */
      {
        /* The 'ecma_date_month_from_time' (ECMA 262 v5, 15.9.1.4) returns a
         * number from 0 to 11, but we have to print the month from 1 to 12
         * for ISO 8601 standard (ECMA 262 v5, 15.9.1.15). */
        number = ecma_date_month_from_time (datetime_number) + 1;
        number_length = 2;
        break;
      }
      case LIT_CHAR_UPPERCASE_D: /* Day. */
      {
        number = ecma_date_date_from_time (datetime_number);
        number_length = 2;
        break;
      }
      case LIT_CHAR_UPPERCASE_W: /* Day of week. */
      {
        int32_t day = ecma_date_week_day (datetime_number);

        JERRY_ASSERT (day >= 0 && day <= 6);

        str_p = day_names_p[day];
        break;
      }
      case LIT_CHAR_LOWERCASE_H: /* Hour. */
      {
        number = ecma_date_hour_from_time (datetime_number);
        number_length = 2;
        break;
      }
      case LIT_CHAR_LOWERCASE_M: /* Minutes. */
      {
        number = ecma_date_min_from_time (datetime_number);
        number_length = 2;
        break;
      }
      case LIT_CHAR_LOWERCASE_S: /* Seconds. */
      {
        number = ecma_date_sec_from_time (datetime_number);
        number_length = 2;
        break;
      }
      case LIT_CHAR_LOWERCASE_I: /* Milliseconds. */
      {
        number = ecma_date_ms_from_time (datetime_number);
        number_length = 3;
        break;
      }
      case LIT_CHAR_LOWERCASE_Z: /* Time zone hours part. */
      {
        int32_t time_zone = (int32_t) ecma_date_local_time_zone_adjustment (datetime_number);

        if (time_zone >= 0)
        {
          *dest_p++ = LIT_CHAR_PLUS;
        }
        else
        {
          *dest_p++ = LIT_CHAR_MINUS;
          time_zone = -time_zone;
        }

        number = time_zone / ECMA_DATE_MS_PER_HOUR;
        number_length = 2;
        break;
      }
      default:
      {
        JERRY_ASSERT (*format_p == LIT_CHAR_UPPERCASE_Z); /* Time zone minutes part. */

        int32_t time_zone = (int32_t) ecma_date_local_time_zone_adjustment (datetime_number);

        if (time_zone < 0)
        {
          time_zone = -time_zone;
        }

        number = (time_zone % ECMA_DATE_MS_PER_HOUR) / ECMA_DATE_MS_PER_MINUTE;
        number_length = 2;
        break;
      }
    }

    format_p++;

    if (str_p != NULL)
    {
      /* Print string values: month or day name which is always 3 characters */
      memcpy (dest_p, str_p, 3);
      dest_p += 3;
      continue;
    }

    /* Print right aligned number values. */
    JERRY_ASSERT (number_length > 0);

    if (number < 0)
    {
      number = -number;
      *dest_p++ = '-';
    }
    else if (*(format_p - 1) == LIT_CHAR_LOWERCASE_Y && number_length == 6)
    {
      /* positive sign is compulsory for extended years */
      *dest_p++ = '+';
    }

    dest_p += number_length;
    lit_utf8_byte_t *buffer_p = dest_p;

    do
    {
      buffer_p--;
      *buffer_p = (lit_utf8_byte_t) ((number % 10) + (int32_t) LIT_CHAR_0);
      number /= 10;
    }
    while (--number_length);
  }

  JERRY_ASSERT (dest_p <= date_buffer + date_buffer_length);

  return ecma_make_string_value (ecma_new_ecma_string_from_utf8 (date_buffer,
                                                                 (lit_utf8_size_t) (dest_p - date_buffer)));
} /* ecma_date_to_string_format */