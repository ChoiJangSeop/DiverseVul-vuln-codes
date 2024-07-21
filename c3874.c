xsltNumberFormatRoman(xmlBufferPtr buffer,
		      double number,
		      int is_upper)
{
    /*
     * Based on an example by Jim Walsh
     */
    while (number >= 1000.0) {
	xmlBufferCCat(buffer, (is_upper) ? "M" : "m");
	number -= 1000.0;
    }
    if (number >= 900.0) {
	xmlBufferCCat(buffer, (is_upper) ? "CM" : "cm");
	number -= 900.0;
    }
    while (number >= 500.0) {
	xmlBufferCCat(buffer, (is_upper) ? "D" : "d");
	number -= 500.0;
    }
    if (number >= 400.0) {
	xmlBufferCCat(buffer, (is_upper) ? "CD" : "cd");
	number -= 400.0;
    }
    while (number >= 100.0) {
	xmlBufferCCat(buffer, (is_upper) ? "C" : "c");
	number -= 100.0;
    }
    if (number >= 90.0) {
	xmlBufferCCat(buffer, (is_upper) ? "XC" : "xc");
	number -= 90.0;
    }
    while (number >= 50.0) {
	xmlBufferCCat(buffer, (is_upper) ? "L" : "l");
	number -= 50.0;
    }
    if (number >= 40.0) {
	xmlBufferCCat(buffer, (is_upper) ? "XL" : "xl");
	number -= 40.0;
    }
    while (number >= 10.0) {
	xmlBufferCCat(buffer, (is_upper) ? "X" : "x");
	number -= 10.0;
    }
    if (number >= 9.0) {
	xmlBufferCCat(buffer, (is_upper) ? "IX" : "ix");
	number -= 9.0;
    }
    while (number >= 5.0) {
	xmlBufferCCat(buffer, (is_upper) ? "V" : "v");
	number -= 5.0;
    }
    if (number >= 4.0) {
	xmlBufferCCat(buffer, (is_upper) ? "IV" : "iv");
	number -= 4.0;
    }
    while (number >= 1.0) {
	xmlBufferCCat(buffer, (is_upper) ? "I" : "i");
	number--;
    }
}