blockconvLow(l_uint32  *data,
             l_int32    w,
             l_int32    h,
             l_int32    wpl,
             l_uint32  *dataa,
             l_int32    wpla,
             l_int32    wc,
             l_int32    hc)
{
l_int32    i, j, imax, imin, jmax, jmin;
l_int32    wn, hn, fwc, fhc, wmwc, hmhc;
l_float32  norm, normh, normw;
l_uint32   val;
l_uint32  *linemina, *linemaxa, *line;

    PROCNAME("blockconvLow");

    wmwc = w - wc;
    hmhc = h - hc;
    if (wmwc <= 0 || hmhc <= 0) {
        L_ERROR("wc >= w || hc >=h\n", procName);
        return;
    }
    fwc = 2 * wc + 1;
    fhc = 2 * hc + 1;
    norm = 1.0 / ((l_float32)(fwc) * fhc);

        /*------------------------------------------------------------*
         *  Compute, using b.c. only to set limits on the accum image *
         *------------------------------------------------------------*/
    for (i = 0; i < h; i++) {
        imin = L_MAX(i - 1 - hc, 0);
        imax = L_MIN(i + hc, h - 1);
        line = data + wpl * i;
        linemina = dataa + wpla * imin;
        linemaxa = dataa + wpla * imax;
        for (j = 0; j < w; j++) {
            jmin = L_MAX(j - 1 - wc, 0);
            jmax = L_MIN(j + wc, w - 1);
            val = linemaxa[jmax] - linemaxa[jmin]
                  + linemina[jmin] - linemina[jmax];
            val = (l_uint8)(norm * val + 0.5);  /* see comment above */
            SET_DATA_BYTE(line, j, val);
        }
    }

        /*------------------------------------------------------------*
         *             Fix normalization for boundary pixels          *
         *------------------------------------------------------------*/
    for (i = 0; i <= hc; i++) {    /* first hc + 1 lines */
        hn = hc + i;
        normh = (l_float32)fhc / (l_float32)hn;   /* > 1 */
        line = data + wpl * i;
        for (j = 0; j <= wc; j++) {
            wn = wc + j;
            normw = (l_float32)fwc / (l_float32)wn;   /* > 1 */
            val = GET_DATA_BYTE(line, j);
            val = (l_uint8)L_MIN(val * normh * normw, 255);
            SET_DATA_BYTE(line, j, val);
        }
        for (j = wc + 1; j < wmwc; j++) {
            val = GET_DATA_BYTE(line, j);
            val = (l_uint8)L_MIN(val * normh, 255);
            SET_DATA_BYTE(line, j, val);
        }
        for (j = wmwc; j < w; j++) {
            wn = wc + w - j;
            normw = (l_float32)fwc / (l_float32)wn;   /* > 1 */
            val = GET_DATA_BYTE(line, j);
            val = (l_uint8)L_MIN(val * normh * normw, 255);
            SET_DATA_BYTE(line, j, val);
        }
    }

    for (i = hmhc; i < h; i++) {  /* last hc lines */
        hn = hc + h - i;
        normh = (l_float32)fhc / (l_float32)hn;   /* > 1 */
        line = data + wpl * i;
        for (j = 0; j <= wc; j++) {
            wn = wc + j;
            normw = (l_float32)fwc / (l_float32)wn;   /* > 1 */
            val = GET_DATA_BYTE(line, j);
            val = (l_uint8)L_MIN(val * normh * normw, 255);
            SET_DATA_BYTE(line, j, val);
        }
        for (j = wc + 1; j < wmwc; j++) {
            val = GET_DATA_BYTE(line, j);
            val = (l_uint8)L_MIN(val * normh, 255);
            SET_DATA_BYTE(line, j, val);
        }
        for (j = wmwc; j < w; j++) {
            wn = wc + w - j;
            normw = (l_float32)fwc / (l_float32)wn;   /* > 1 */
            val = GET_DATA_BYTE(line, j);
            val = (l_uint8)L_MIN(val * normh * normw, 255);
            SET_DATA_BYTE(line, j, val);
        }
    }

    for (i = hc + 1; i < hmhc; i++) {    /* intermediate lines */
        line = data + wpl * i;
        for (j = 0; j <= wc; j++) {   /* first wc + 1 columns */
            wn = wc + j;
            normw = (l_float32)fwc / (l_float32)wn;   /* > 1 */
            val = GET_DATA_BYTE(line, j);
            val = (l_uint8)L_MIN(val * normw, 255);
            SET_DATA_BYTE(line, j, val);
        }
        for (j = wmwc; j < w; j++) {   /* last wc columns */
            wn = wc + w - j;
            normw = (l_float32)fwc / (l_float32)wn;   /* > 1 */
            val = GET_DATA_BYTE(line, j);
            val = (l_uint8)L_MIN(val * normw, 255);
            SET_DATA_BYTE(line, j, val);
        }
    }
}