EXPORTED time_t calc_compile_time(const char *time, const char *date)
{
    struct tm tm;
    char month[4];
    const char *monthname[] = {
	"Jan", "Feb", "Mar", "Apr", "May", "Jun",
	"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
    };

    memset(&tm, 0, sizeof(struct tm));
    tm.tm_isdst = -1;
    sscanf(time, "%02d:%02d:%02d", &tm.tm_hour, &tm.tm_min, &tm.tm_sec);
    sscanf(date, "%s %2d %4d", month, &tm.tm_mday, &tm.tm_year);
    tm.tm_year -= 1900;
    for (tm.tm_mon = 0; tm.tm_mon < 12; tm.tm_mon++) {
	if (!strcmp(month, monthname[tm.tm_mon])) break;
    }

    return mktime(&tm);
}