process_button(struct parsed_tag *tag)
{
    Str tmp = NULL;
    char *p, *q, *r, *qq = "";
    int qlen, v;

    if (cur_form_id < 0) {
       char *s = "<form_int method=internal action=none>";
       tmp = process_form(parse_tag(&s, TRUE));
    }
    if (tmp == NULL)
       tmp = Strnew();

    p = "submit";
    parsedtag_get_value(tag, ATTR_TYPE, &p);
    q = NULL;
    parsedtag_get_value(tag, ATTR_VALUE, &q);
    r = "";
    parsedtag_get_value(tag, ATTR_NAME, &r);

    v = formtype(p);
    if (v == FORM_UNKNOWN)
       return NULL;

    if (!q) {
       switch (v) {
       case FORM_INPUT_SUBMIT:
       case FORM_INPUT_BUTTON:
           q = "SUBMIT";
           break;
       case FORM_INPUT_RESET:
           q = "RESET";
           break;
       }
    }
    if (q) {
       qq = html_quote(q);
       qlen = strlen(q);
    }

    /*    Strcat_charp(tmp, "<pre_int>"); */
    Strcat(tmp, Sprintf("<input_alt hseq=\"%d\" fid=\"%d\" type=\"%s\" "
                       "name=\"%s\" value=\"%s\">",
                       cur_hseq++, cur_form_id, html_quote(p),
                       html_quote(r), qq));
    return tmp;
}