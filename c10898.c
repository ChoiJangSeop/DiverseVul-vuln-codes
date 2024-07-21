Init_date_core(void)
{
    #ifdef HAVE_RB_EXT_RACTOR_SAFE
	RB_EXT_RACTOR_SAFE(true);
    #endif
    id_cmp = rb_intern_const("<=>");
    id_le_p = rb_intern_const("<=");
    id_ge_p = rb_intern_const(">=");
    id_eqeq_p = rb_intern_const("==");

    half_days_in_day = rb_rational_new2(INT2FIX(1), INT2FIX(2));

#if (LONG_MAX / DAY_IN_SECONDS) > SECOND_IN_NANOSECONDS
    day_in_nanoseconds = LONG2NUM((long)DAY_IN_SECONDS *
				  SECOND_IN_NANOSECONDS);
#elif defined HAVE_LONG_LONG
    day_in_nanoseconds = LL2NUM((LONG_LONG)DAY_IN_SECONDS *
				SECOND_IN_NANOSECONDS);
#else
    day_in_nanoseconds = f_mul(INT2FIX(DAY_IN_SECONDS),
			       INT2FIX(SECOND_IN_NANOSECONDS));
#endif

    rb_gc_register_mark_object(half_days_in_day);
    rb_gc_register_mark_object(day_in_nanoseconds);

    positive_inf = +INFINITY;
    negative_inf = -INFINITY;

    /*
     * date and datetime class - Tadayoshi Funaba 1998-2011
     *
     * 'date' provides two classes: Date and DateTime.
     *
     * == Terms and Definitions
     *
     * Some terms and definitions are based on ISO 8601 and JIS X 0301.
     *
     * === Calendar Date
     *
     * The calendar date is a particular day of a calendar year,
     * identified by its ordinal number within a calendar month within
     * that year.
     *
     * In those classes, this is so-called "civil".
     *
     * === Ordinal Date
     *
     * The ordinal date is a particular day of a calendar year identified
     * by its ordinal number within the year.
     *
     * In those classes, this is so-called "ordinal".
     *
     * === Week Date
     *
     * The week date is a date identified by calendar week and day numbers.
     *
     * The calendar week is a seven day period within a calendar year,
     * starting on a Monday and identified by its ordinal number within
     * the year; the first calendar week of the year is the one that
     * includes the first Thursday of that year. In the Gregorian
     * calendar, this is equivalent to the week which includes January 4.
     *
     * In those classes, this is so-called "commercial".
     *
     * === Julian Day Number
     *
     * The Julian day number is in elapsed days since noon (Greenwich Mean
     * Time) on January 1, 4713 BCE (in the Julian calendar).
     *
     * In this document, the astronomical Julian day number is the same as
     * the original Julian day number. And the chronological Julian day
     * number is a variation of the Julian day number. Its days begin at
     * midnight on local time.
     *
     * In this document, when the term "Julian day number" simply appears,
     * it just refers to "chronological Julian day number", not the
     * original.
     *
     * In those classes, those are so-called "ajd" and "jd".
     *
     * === Modified Julian Day Number
     *
     * The modified Julian day number is in elapsed days since midnight
     * (Coordinated Universal Time) on November 17, 1858 CE (in the
     * Gregorian calendar).
     *
     * In this document, the astronomical modified Julian day number is
     * the same as the original modified Julian day number. And the
     * chronological modified Julian day number is a variation of the
     * modified Julian day number. Its days begin at midnight on local
     * time.
     *
     * In this document, when the term "modified Julian day number" simply
     * appears, it just refers to "chronological modified Julian day
     * number", not the original.
     *
     * In those classes, those are so-called "amjd" and "mjd".
     *
     * == Date
     *
     * A subclass of Object that includes the Comparable module and
     * easily handles date.
     *
     * A Date object is created with Date::new, Date::jd, Date::ordinal,
     * Date::commercial, Date::parse, Date::strptime, Date::today,
     * Time#to_date, etc.
     *
     *     require 'date'
     *
     *     Date.new(2001,2,3)
     *	    #=> #<Date: 2001-02-03 ...>
     *     Date.jd(2451944)
     *	    #=> #<Date: 2001-02-03 ...>
     *     Date.ordinal(2001,34)
     *	    #=> #<Date: 2001-02-03 ...>
     *     Date.commercial(2001,5,6)
     *	    #=> #<Date: 2001-02-03 ...>
     *     Date.parse('2001-02-03')
     *	    #=> #<Date: 2001-02-03 ...>
     *     Date.strptime('03-02-2001', '%d-%m-%Y')
     *	    #=> #<Date: 2001-02-03 ...>
     *     Time.new(2001,2,3).to_date
     *	    #=> #<Date: 2001-02-03 ...>
     *
     * All date objects are immutable; hence cannot modify themselves.
     *
     * The concept of a date object can be represented as a tuple
     * of the day count, the offset and the day of calendar reform.
     *
     * The day count denotes the absolute position of a temporal
     * dimension. The offset is relative adjustment, which determines
     * decoded local time with the day count. The day of calendar
     * reform denotes the start day of the new style. The old style
     * of the West is the Julian calendar which was adopted by
     * Caesar. The new style is the Gregorian calendar, which is the
     * current civil calendar of many countries.
     *
     * The day count is virtually the astronomical Julian day number.
     * The offset in this class is usually zero, and cannot be
     * specified directly.
     *
     * A Date object can be created with an optional argument,
     * the day of calendar reform as a Julian day number, which
     * should be 2298874 to 2426355 or negative/positive infinity.
     * The default value is +Date::ITALY+ (2299161=1582-10-15).
     * See also sample/cal.rb.
     *
     *     $ ruby sample/cal.rb -c it 10 1582
     *         October 1582
     *      S  M Tu  W Th  F  S
     *         1  2  3  4 15 16
     *     17 18 19 20 21 22 23
     *     24 25 26 27 28 29 30
     *     31
     *
     *     $ ruby sample/cal.rb -c gb  9 1752
     *        September 1752
     *      S  M Tu  W Th  F  S
     *            1  2 14 15 16
     *     17 18 19 20 21 22 23
     *     24 25 26 27 28 29 30
     *
     * A Date object has various methods. See each reference.
     *
     *     d = Date.parse('3rd Feb 2001')
     *					#=> #<Date: 2001-02-03 ...>
     *     d.year			#=> 2001
     *     d.mon			#=> 2
     *     d.mday			#=> 3
     *     d.wday			#=> 6
     *     d += 1			#=> #<Date: 2001-02-04 ...>
     *     d.strftime('%a %d %b %Y')	#=> "Sun 04 Feb 2001"
     *
     */
    cDate = rb_define_class("Date", rb_cObject);
    eDateError = rb_define_class_under(cDate, "Error", rb_eArgError);

    rb_include_module(cDate, rb_mComparable);

    /* An array of strings of full month names in English.  The first
     * element is nil.
     */
    rb_define_const(cDate, "MONTHNAMES", mk_ary_of_str(13, monthnames));

    /* An array of strings of abbreviated month names in English.  The
     * first element is nil.
     */
    rb_define_const(cDate, "ABBR_MONTHNAMES",
		    mk_ary_of_str(13, abbr_monthnames));

    /* An array of strings of the full names of days of the week in English.
     * The first is "Sunday".
     */
    rb_define_const(cDate, "DAYNAMES", mk_ary_of_str(7, daynames));

    /* An array of strings of abbreviated day names in English.  The
     * first is "Sun".
     */
    rb_define_const(cDate, "ABBR_DAYNAMES", mk_ary_of_str(7, abbr_daynames));

    /* The Julian day number of the day of calendar reform for Italy
     * and some catholic countries.
     */
    rb_define_const(cDate, "ITALY", INT2FIX(ITALY));

    /* The Julian day number of the day of calendar reform for England
     * and her colonies.
     */
    rb_define_const(cDate, "ENGLAND", INT2FIX(ENGLAND));

    /* The Julian day number of the day of calendar reform for the
     * proleptic Julian calendar.
     */
    rb_define_const(cDate, "JULIAN", DBL2NUM(JULIAN));

    /* The Julian day number of the day of calendar reform for the
     * proleptic Gregorian calendar.
     */
    rb_define_const(cDate, "GREGORIAN", DBL2NUM(GREGORIAN));

    rb_define_alloc_func(cDate, d_lite_s_alloc_simple);

#ifndef NDEBUG
    rb_define_private_method(CLASS_OF(cDate), "_valid_jd?",
			     date_s__valid_jd_p, -1);
    rb_define_private_method(CLASS_OF(cDate), "_valid_ordinal?",
			     date_s__valid_ordinal_p, -1);
    rb_define_private_method(CLASS_OF(cDate), "_valid_civil?",
			     date_s__valid_civil_p, -1);
    rb_define_private_method(CLASS_OF(cDate), "_valid_date?",
			     date_s__valid_civil_p, -1);
    rb_define_private_method(CLASS_OF(cDate), "_valid_commercial?",
			     date_s__valid_commercial_p, -1);
    rb_define_private_method(CLASS_OF(cDate), "_valid_weeknum?",
			     date_s__valid_weeknum_p, -1);
    rb_define_private_method(CLASS_OF(cDate), "_valid_nth_kday?",
			     date_s__valid_nth_kday_p, -1);
#endif

    rb_define_singleton_method(cDate, "valid_jd?", date_s_valid_jd_p, -1);
    rb_define_singleton_method(cDate, "valid_ordinal?",
			       date_s_valid_ordinal_p, -1);
    rb_define_singleton_method(cDate, "valid_civil?", date_s_valid_civil_p, -1);
    rb_define_singleton_method(cDate, "valid_date?", date_s_valid_civil_p, -1);
    rb_define_singleton_method(cDate, "valid_commercial?",
			       date_s_valid_commercial_p, -1);

#ifndef NDEBUG
    rb_define_private_method(CLASS_OF(cDate), "valid_weeknum?",
			     date_s_valid_weeknum_p, -1);
    rb_define_private_method(CLASS_OF(cDate), "valid_nth_kday?",
			     date_s_valid_nth_kday_p, -1);
    rb_define_private_method(CLASS_OF(cDate), "zone_to_diff",
			     date_s_zone_to_diff, 1);
#endif

    rb_define_singleton_method(cDate, "julian_leap?", date_s_julian_leap_p, 1);
    rb_define_singleton_method(cDate, "gregorian_leap?",
			       date_s_gregorian_leap_p, 1);
    rb_define_singleton_method(cDate, "leap?",
			       date_s_gregorian_leap_p, 1);

#ifndef NDEBUG
    rb_define_singleton_method(cDate, "new!", date_s_new_bang, -1);
    rb_define_alias(rb_singleton_class(cDate), "new_l!", "new");
#endif

    rb_define_singleton_method(cDate, "jd", date_s_jd, -1);
    rb_define_singleton_method(cDate, "ordinal", date_s_ordinal, -1);
    rb_define_singleton_method(cDate, "civil", date_s_civil, -1);
    rb_define_singleton_method(cDate, "commercial", date_s_commercial, -1);

#ifndef NDEBUG
    rb_define_singleton_method(cDate, "weeknum", date_s_weeknum, -1);
    rb_define_singleton_method(cDate, "nth_kday", date_s_nth_kday, -1);
#endif

    rb_define_singleton_method(cDate, "today", date_s_today, -1);
    rb_define_singleton_method(cDate, "_strptime", date_s__strptime, -1);
    rb_define_singleton_method(cDate, "strptime", date_s_strptime, -1);
    rb_define_singleton_method(cDate, "_parse", date_s__parse, -1);
    rb_define_singleton_method(cDate, "parse", date_s_parse, -1);
    rb_define_singleton_method(cDate, "_iso8601", date_s__iso8601, 1);
    rb_define_singleton_method(cDate, "iso8601", date_s_iso8601, -1);
    rb_define_singleton_method(cDate, "_rfc3339", date_s__rfc3339, 1);
    rb_define_singleton_method(cDate, "rfc3339", date_s_rfc3339, -1);
    rb_define_singleton_method(cDate, "_xmlschema", date_s__xmlschema, 1);
    rb_define_singleton_method(cDate, "xmlschema", date_s_xmlschema, -1);
    rb_define_singleton_method(cDate, "_rfc2822", date_s__rfc2822, 1);
    rb_define_singleton_method(cDate, "_rfc822", date_s__rfc2822, 1);
    rb_define_singleton_method(cDate, "rfc2822", date_s_rfc2822, -1);
    rb_define_singleton_method(cDate, "rfc822", date_s_rfc2822, -1);
    rb_define_singleton_method(cDate, "_httpdate", date_s__httpdate, 1);
    rb_define_singleton_method(cDate, "httpdate", date_s_httpdate, -1);
    rb_define_singleton_method(cDate, "_jisx0301", date_s__jisx0301, 1);
    rb_define_singleton_method(cDate, "jisx0301", date_s_jisx0301, -1);

    rb_define_method(cDate, "initialize", date_initialize, -1);
    rb_define_method(cDate, "initialize_copy", d_lite_initialize_copy, 1);

#ifndef NDEBUG
    rb_define_method(cDate, "fill", d_lite_fill, 0);
#endif

    rb_define_method(cDate, "ajd", d_lite_ajd, 0);
    rb_define_method(cDate, "amjd", d_lite_amjd, 0);
    rb_define_method(cDate, "jd", d_lite_jd, 0);
    rb_define_method(cDate, "mjd", d_lite_mjd, 0);
    rb_define_method(cDate, "ld", d_lite_ld, 0);

    rb_define_method(cDate, "year", d_lite_year, 0);
    rb_define_method(cDate, "yday", d_lite_yday, 0);
    rb_define_method(cDate, "mon", d_lite_mon, 0);
    rb_define_method(cDate, "month", d_lite_mon, 0);
    rb_define_method(cDate, "mday", d_lite_mday, 0);
    rb_define_method(cDate, "day", d_lite_mday, 0);
    rb_define_method(cDate, "day_fraction", d_lite_day_fraction, 0);

    rb_define_method(cDate, "cwyear", d_lite_cwyear, 0);
    rb_define_method(cDate, "cweek", d_lite_cweek, 0);
    rb_define_method(cDate, "cwday", d_lite_cwday, 0);

#ifndef NDEBUG
    rb_define_private_method(cDate, "wnum0", d_lite_wnum0, 0);
    rb_define_private_method(cDate, "wnum1", d_lite_wnum1, 0);
#endif

    rb_define_method(cDate, "wday", d_lite_wday, 0);

    rb_define_method(cDate, "sunday?", d_lite_sunday_p, 0);
    rb_define_method(cDate, "monday?", d_lite_monday_p, 0);
    rb_define_method(cDate, "tuesday?", d_lite_tuesday_p, 0);
    rb_define_method(cDate, "wednesday?", d_lite_wednesday_p, 0);
    rb_define_method(cDate, "thursday?", d_lite_thursday_p, 0);
    rb_define_method(cDate, "friday?", d_lite_friday_p, 0);
    rb_define_method(cDate, "saturday?", d_lite_saturday_p, 0);

#ifndef NDEBUG
    rb_define_method(cDate, "nth_kday?", d_lite_nth_kday_p, 2);
#endif

    rb_define_private_method(cDate, "hour", d_lite_zero, 0);
    rb_define_private_method(cDate, "min", d_lite_zero, 0);
    rb_define_private_method(cDate, "minute", d_lite_zero, 0);
    rb_define_private_method(cDate, "sec", d_lite_zero, 0);
    rb_define_private_method(cDate, "second", d_lite_zero, 0);

    rb_define_method(cDate, "julian?", d_lite_julian_p, 0);
    rb_define_method(cDate, "gregorian?", d_lite_gregorian_p, 0);
    rb_define_method(cDate, "leap?", d_lite_leap_p, 0);

    rb_define_method(cDate, "start", d_lite_start, 0);
    rb_define_method(cDate, "new_start", d_lite_new_start, -1);
    rb_define_method(cDate, "italy", d_lite_italy, 0);
    rb_define_method(cDate, "england", d_lite_england, 0);
    rb_define_method(cDate, "julian", d_lite_julian, 0);
    rb_define_method(cDate, "gregorian", d_lite_gregorian, 0);

    rb_define_method(cDate, "+", d_lite_plus, 1);
    rb_define_method(cDate, "-", d_lite_minus, 1);

    rb_define_method(cDate, "next_day", d_lite_next_day, -1);
    rb_define_method(cDate, "prev_day", d_lite_prev_day, -1);
    rb_define_method(cDate, "next", d_lite_next, 0);
    rb_define_method(cDate, "succ", d_lite_next, 0);

    rb_define_method(cDate, ">>", d_lite_rshift, 1);
    rb_define_method(cDate, "<<", d_lite_lshift, 1);

    rb_define_method(cDate, "next_month", d_lite_next_month, -1);
    rb_define_method(cDate, "prev_month", d_lite_prev_month, -1);
    rb_define_method(cDate, "next_year", d_lite_next_year, -1);
    rb_define_method(cDate, "prev_year", d_lite_prev_year, -1);

    rb_define_method(cDate, "step", d_lite_step, -1);
    rb_define_method(cDate, "upto", d_lite_upto, 1);
    rb_define_method(cDate, "downto", d_lite_downto, 1);

    rb_define_method(cDate, "<=>", d_lite_cmp, 1);
    rb_define_method(cDate, "===", d_lite_equal, 1);
    rb_define_method(cDate, "eql?", d_lite_eql_p, 1);
    rb_define_method(cDate, "hash", d_lite_hash, 0);

    rb_define_method(cDate, "to_s", d_lite_to_s, 0);
#ifndef NDEBUG
    rb_define_method(cDate, "inspect_raw", d_lite_inspect_raw, 0);
#endif
    rb_define_method(cDate, "inspect", d_lite_inspect, 0);

    rb_define_method(cDate, "strftime", d_lite_strftime, -1);

    rb_define_method(cDate, "asctime", d_lite_asctime, 0);
    rb_define_method(cDate, "ctime", d_lite_asctime, 0);
    rb_define_method(cDate, "iso8601", d_lite_iso8601, 0);
    rb_define_method(cDate, "xmlschema", d_lite_iso8601, 0);
    rb_define_method(cDate, "rfc3339", d_lite_rfc3339, 0);
    rb_define_method(cDate, "rfc2822", d_lite_rfc2822, 0);
    rb_define_method(cDate, "rfc822", d_lite_rfc2822, 0);
    rb_define_method(cDate, "httpdate", d_lite_httpdate, 0);
    rb_define_method(cDate, "jisx0301", d_lite_jisx0301, 0);

#ifndef NDEBUG
    rb_define_method(cDate, "marshal_dump_old", d_lite_marshal_dump_old, 0);
#endif
    rb_define_method(cDate, "marshal_dump", d_lite_marshal_dump, 0);
    rb_define_method(cDate, "marshal_load", d_lite_marshal_load, 1);
    rb_define_singleton_method(cDate, "_load", date_s__load, 1);

    /*
     * == DateTime
     *
     * A subclass of Date that easily handles date, hour, minute, second,
     * and offset.
     *
     * DateTime class is considered deprecated. Use Time class.
     *
     * DateTime does not consider any leap seconds, does not track
     * any summer time rules.
     *
     * A DateTime object is created with DateTime::new, DateTime::jd,
     * DateTime::ordinal, DateTime::commercial, DateTime::parse,
     * DateTime::strptime, DateTime::now, Time#to_datetime, etc.
     *
     *     require 'date'
     *
     *     DateTime.new(2001,2,3,4,5,6)
     *                         #=> #<DateTime: 2001-02-03T04:05:06+00:00 ...>
     *
     * The last element of day, hour, minute, or second can be a
     * fractional number. The fractional number's precision is assumed
     * at most nanosecond.
     *
     *     DateTime.new(2001,2,3.5)
     *                         #=> #<DateTime: 2001-02-03T12:00:00+00:00 ...>
     *
     * An optional argument, the offset, indicates the difference
     * between the local time and UTC. For example, <tt>Rational(3,24)</tt>
     * represents ahead of 3 hours of UTC, <tt>Rational(-5,24)</tt> represents
     * behind of 5 hours of UTC. The offset should be -1 to +1, and
     * its precision is assumed at most second. The default value is
     * zero (equals to UTC).
     *
     *     DateTime.new(2001,2,3,4,5,6,Rational(3,24))
     *                         #=> #<DateTime: 2001-02-03T04:05:06+03:00 ...>
     *
     * The offset also accepts string form:
     *
     *     DateTime.new(2001,2,3,4,5,6,'+03:00')
     *                         #=> #<DateTime: 2001-02-03T04:05:06+03:00 ...>
     *
     * An optional argument, the day of calendar reform (+start+), denotes
     * a Julian day number, which should be 2298874 to 2426355 or
     * negative/positive infinity.
     * The default value is +Date::ITALY+ (2299161=1582-10-15).
     *
     * A DateTime object has various methods. See each reference.
     *
     *     d = DateTime.parse('3rd Feb 2001 04:05:06+03:30')
     *                         #=> #<DateTime: 2001-02-03T04:05:06+03:30 ...>
     *     d.hour              #=> 4
     *     d.min               #=> 5
     *     d.sec               #=> 6
     *     d.offset            #=> (7/48)
     *     d.zone              #=> "+03:30"
     *     d += Rational('1.5')
     *                         #=> #<DateTime: 2001-02-04%16:05:06+03:30 ...>
     *     d = d.new_offset('+09:00')
     *                         #=> #<DateTime: 2001-02-04%21:35:06+09:00 ...>
     *     d.strftime('%I:%M:%S %p')
     *                         #=> "09:35:06 PM"
     *     d > DateTime.new(1999)
     *                         #=> true
     *
     * === When should you use DateTime and when should you use Time?
     *
     * It's a common misconception that
     * {William Shakespeare}[https://en.wikipedia.org/wiki/William_Shakespeare]
     * and
     * {Miguel de Cervantes}[https://en.wikipedia.org/wiki/Miguel_de_Cervantes]
     * died on the same day in history -
     * so much so that UNESCO named April 23 as
     * {World Book Day because of this fact}[https://en.wikipedia.org/wiki/World_Book_Day].
     * However, because England hadn't yet adopted the
     * {Gregorian Calendar Reform}[https://en.wikipedia.org/wiki/Gregorian_calendar#Gregorian_reform]
     * (and wouldn't until {1752}[https://en.wikipedia.org/wiki/Calendar_(New_Style)_Act_1750])
     * their deaths are actually 10 days apart.
     * Since Ruby's Time class implements a
     * {proleptic Gregorian calendar}[https://en.wikipedia.org/wiki/Proleptic_Gregorian_calendar]
     * and has no concept of calendar reform there's no way
     * to express this with Time objects. This is where DateTime steps in:
     *
     *     shakespeare = DateTime.iso8601('1616-04-23', Date::ENGLAND)
     *      #=> Tue, 23 Apr 1616 00:00:00 +0000
     *     cervantes = DateTime.iso8601('1616-04-23', Date::ITALY)
     *      #=> Sat, 23 Apr 1616 00:00:00 +0000
     *
     * Already you can see something is weird - the days of the week
     * are different. Taking this further:
     *
     *     cervantes == shakespeare
     *      #=> false
     *     (shakespeare - cervantes).to_i
     *      #=> 10
     *
     * This shows that in fact they died 10 days apart (in reality
     * 11 days since Cervantes died a day earlier but was buried on
     * the 23rd). We can see the actual date of Shakespeare's death by
     * using the #gregorian method to convert it:
     *
     *     shakespeare.gregorian
     *      #=> Tue, 03 May 1616 00:00:00 +0000
     *
     * So there's an argument that all the celebrations that take
     * place on the 23rd April in Stratford-upon-Avon are actually
     * the wrong date since England is now using the Gregorian calendar.
     * You can see why when we transition across the reform
     * date boundary:
     *
     *     # start off with the anniversary of Shakespeare's birth in 1751
     *     shakespeare = DateTime.iso8601('1751-04-23', Date::ENGLAND)
     *      #=> Tue, 23 Apr 1751 00:00:00 +0000
     *
     *     # add 366 days since 1752 is a leap year and April 23 is after February 29
     *     shakespeare + 366
     *      #=> Thu, 23 Apr 1752 00:00:00 +0000
     *
     *     # add another 365 days to take us to the anniversary in 1753
     *     shakespeare + 366 + 365
     *      #=> Fri, 04 May 1753 00:00:00 +0000
     *
     * As you can see, if we're accurately tracking the number of
     * {solar years}[https://en.wikipedia.org/wiki/Tropical_year]
     * since Shakespeare's birthday then the correct anniversary date
     * would be the 4th May and not the 23rd April.
     *
     * So when should you use DateTime in Ruby and when should
     * you use Time? Almost certainly you'll want to use Time
     * since your app is probably dealing with current dates and
     * times. However, if you need to deal with dates and times in a
     * historical context you'll want to use DateTime to avoid
     * making the same mistakes as UNESCO. If you also have to deal
     * with timezones then best of luck - just bear in mind that
     * you'll probably be dealing with
     * {local solar times}[https://en.wikipedia.org/wiki/Solar_time],
     * since it wasn't until the 19th century that the introduction
     * of the railways necessitated the need for
     * {Standard Time}[https://en.wikipedia.org/wiki/Standard_time#Great_Britain]
     * and eventually timezones.
     */

    cDateTime = rb_define_class("DateTime", cDate);
    rb_define_alloc_func(cDateTime, d_lite_s_alloc_complex);

    rb_define_singleton_method(cDateTime, "jd", datetime_s_jd, -1);
    rb_define_singleton_method(cDateTime, "ordinal", datetime_s_ordinal, -1);
    rb_define_singleton_method(cDateTime, "civil", datetime_s_civil, -1);
    rb_define_singleton_method(cDateTime, "new", datetime_s_civil, -1);
    rb_define_singleton_method(cDateTime, "commercial",
			       datetime_s_commercial, -1);

#ifndef NDEBUG
    rb_define_singleton_method(cDateTime, "weeknum",
			       datetime_s_weeknum, -1);
    rb_define_singleton_method(cDateTime, "nth_kday",
			       datetime_s_nth_kday, -1);
#endif

    rb_undef_method(CLASS_OF(cDateTime), "today");

    rb_define_singleton_method(cDateTime, "now", datetime_s_now, -1);
    rb_define_singleton_method(cDateTime, "_strptime",
			       datetime_s__strptime, -1);
    rb_define_singleton_method(cDateTime, "strptime",
			       datetime_s_strptime, -1);
    rb_define_singleton_method(cDateTime, "parse",
			       datetime_s_parse, -1);
    rb_define_singleton_method(cDateTime, "iso8601",
			       datetime_s_iso8601, -1);
    rb_define_singleton_method(cDateTime, "rfc3339",
			       datetime_s_rfc3339, -1);
    rb_define_singleton_method(cDateTime, "xmlschema",
			       datetime_s_xmlschema, -1);
    rb_define_singleton_method(cDateTime, "rfc2822",
			       datetime_s_rfc2822, -1);
    rb_define_singleton_method(cDateTime, "rfc822",
			       datetime_s_rfc2822, -1);
    rb_define_singleton_method(cDateTime, "httpdate",
			       datetime_s_httpdate, -1);
    rb_define_singleton_method(cDateTime, "jisx0301",
			       datetime_s_jisx0301, -1);

    rb_define_method(cDateTime, "hour", d_lite_hour, 0);
    rb_define_method(cDateTime, "min", d_lite_min, 0);
    rb_define_method(cDateTime, "minute", d_lite_min, 0);
    rb_define_method(cDateTime, "sec", d_lite_sec, 0);
    rb_define_method(cDateTime, "second", d_lite_sec, 0);
    rb_define_method(cDateTime, "sec_fraction", d_lite_sec_fraction, 0);
    rb_define_method(cDateTime, "second_fraction", d_lite_sec_fraction, 0);
    rb_define_method(cDateTime, "offset", d_lite_offset, 0);
    rb_define_method(cDateTime, "zone", d_lite_zone, 0);
    rb_define_method(cDateTime, "new_offset", d_lite_new_offset, -1);

    rb_define_method(cDateTime, "to_s", dt_lite_to_s, 0);

    rb_define_method(cDateTime, "strftime", dt_lite_strftime, -1);

    rb_define_method(cDateTime, "iso8601", dt_lite_iso8601, -1);
    rb_define_method(cDateTime, "xmlschema", dt_lite_iso8601, -1);
    rb_define_method(cDateTime, "rfc3339", dt_lite_rfc3339, -1);
    rb_define_method(cDateTime, "jisx0301", dt_lite_jisx0301, -1);

    /* conversions */

    rb_define_method(rb_cTime, "to_time", time_to_time, 0);
    rb_define_method(rb_cTime, "to_date", time_to_date, 0);
    rb_define_method(rb_cTime, "to_datetime", time_to_datetime, 0);

    rb_define_method(cDate, "to_time", date_to_time, 0);
    rb_define_method(cDate, "to_date", date_to_date, 0);
    rb_define_method(cDate, "to_datetime", date_to_datetime, 0);

    rb_define_method(cDateTime, "to_time", datetime_to_time, 0);
    rb_define_method(cDateTime, "to_date", datetime_to_date, 0);
    rb_define_method(cDateTime, "to_datetime", datetime_to_datetime, 0);

#ifndef NDEBUG
    /* tests */

    rb_define_singleton_method(cDate, "test_civil", date_s_test_civil, 0);
    rb_define_singleton_method(cDate, "test_ordinal", date_s_test_ordinal, 0);
    rb_define_singleton_method(cDate, "test_commercial",
			       date_s_test_commercial, 0);
    rb_define_singleton_method(cDate, "test_weeknum", date_s_test_weeknum, 0);
    rb_define_singleton_method(cDate, "test_nth_kday", date_s_test_nth_kday, 0);
    rb_define_singleton_method(cDate, "test_unit_conv",
			       date_s_test_unit_conv, 0);
    rb_define_singleton_method(cDate, "test_all", date_s_test_all, 0);
#endif
}