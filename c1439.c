cmsFloat64Number ParseFloatNumber(const char *Buffer)
{
    cmsFloat64Number dnum = 0.0;
    int sign = 1;

    if (*Buffer == '-' || *Buffer == '+') {

         sign = (*Buffer == '-') ? -1 : 1;
         Buffer++;
    }


    while (*Buffer && isdigit((int) *Buffer)) {

        dnum = dnum * 10.0 + (*Buffer - '0');
        if (*Buffer) Buffer++;
    }

    if (*Buffer == '.') {

        cmsFloat64Number frac = 0.0;      // fraction
        int prec = 0;                     // precission

        if (*Buffer) Buffer++;

        while (*Buffer && isdigit((int) *Buffer)) {

            frac = frac * 10.0 + (*Buffer - '0');
            prec++;
            if (*Buffer) Buffer++;
        }

        dnum = dnum + (frac / xpow10(prec));
    }

    // Exponent, example 34.00E+20
    if (*Buffer && toupper(*Buffer) == 'E') {

        int e;
        int sgn;

        if (*Buffer) Buffer++;
        sgn = 1;

        if (*Buffer == '-') {

            sgn = -1;
            if (*Buffer) Buffer++;
        }
        else
            if (*Buffer == '+') {

                sgn = +1;
                if (*Buffer) Buffer++;
            }

            e = 0;
            while (*Buffer && isdigit((int) *Buffer)) {

                if ((cmsFloat64Number) e * 10L < INT_MAX)
                    e = e * 10 + (*Buffer - '0');

                if (*Buffer) Buffer++;
            }

            e = sgn*e;
            dnum = dnum * xpow10(e);
    }

    return sign * dnum;
}