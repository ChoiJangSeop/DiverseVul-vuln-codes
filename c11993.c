void PersianCalendar::handleComputeFields(int32_t julianDay, UErrorCode &/*status*/) {
    int32_t year, month, dayOfMonth, dayOfYear;

    int32_t daysSinceEpoch = julianDay - PERSIAN_EPOCH;
    year = 1 + ClockMath::floorDivide(33 * daysSinceEpoch + 3, 12053);

    int32_t farvardin1 = 365 * (year - 1) + ClockMath::floorDivide(8 * year + 21, 33);
    dayOfYear = (daysSinceEpoch - farvardin1); // 0-based
    if (dayOfYear < 216) { // Compute 0-based month
        month = dayOfYear / 31;
    } else {
        month = (dayOfYear - 6) / 30;
    }
    dayOfMonth = dayOfYear - kPersianNumDays[month] + 1;
    ++dayOfYear; // Make it 1-based now

    internalSet(UCAL_ERA, 0);
    internalSet(UCAL_YEAR, year);
    internalSet(UCAL_EXTENDED_YEAR, year);
    internalSet(UCAL_MONTH, month);
    internalSet(UCAL_DAY_OF_MONTH, dayOfMonth);
    internalSet(UCAL_DAY_OF_YEAR, dayOfYear);
}    