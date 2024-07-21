void CalendarRegressionTest::TestT11632(void) {
    UErrorCode status = U_ZERO_ERROR;
    GregorianCalendar cal(TimeZone::createTimeZone("Pacific/Apia"), status);
    if(U_FAILURE(status)) {
        dataerrln("Error creating Calendar: %s", u_errorName(status));
        return;
    }
    failure(status, "Calendar::createInstance(status)");
    cal.clear();
    failure(status, "clear calendar");
    cal.set(UCAL_HOUR, 597);
    failure(status, "set hour value in calendar");
    SimpleDateFormat sdf(UnicodeString("y-MM-dd'T'HH:mm:ss"), status);
    failure(status, "initializing SimpleDateFormat");
    sdf.setCalendar(cal);
    UnicodeString dstr;
    UDate d = cal.getTime(status);
    if (!failure(status, "getTime for date")) {
        sdf.format(d, dstr);
        std::string utf8;
        dstr.toUTF8String(utf8);
        assertEquals("correct datetime displayed for hour value", UnicodeString("1970-01-25T21:00:00"), dstr);
        cal.clear();
        failure(status, "clear calendar");
        cal.set(UCAL_HOUR, 300);
        failure(status, "set hour value in calendar");
        sdf.setCalendar(cal);
        d = cal.getTime(status);
        if (!failure(status, "getTime for initial date")) {
            dstr.remove();
            sdf.format(d, dstr);
            dstr.toUTF8String(utf8);
            assertEquals("correct datetime displayed for hour value", UnicodeString("1970-01-13T12:00:00"), dstr);
        }
    }
} 