ippSetValueTag(
    ipp_t          *ipp,		/* I  - IPP message */
    ipp_attribute_t **attr,		/* IO - IPP attribute */
    ipp_tag_t       value_tag)		/* I  - Value tag */
{
  int		i;			/* Looping var */
  _ipp_value_t	*value;			/* Current value */
  int		integer;		/* Current integer value */
  cups_lang_t	*language;		/* Current language */
  char		code[32];		/* Language code */
  ipp_tag_t	temp_tag;		/* Temporary value tag */


 /*
  * Range check input...
  */

  if (!ipp || !attr || !*attr)
    return (0);

 /*
  * If there is no change, return immediately...
  */

  if (value_tag == (*attr)->value_tag)
    return (1);

 /*
  * Otherwise implement changes as needed...
  */

  temp_tag = (ipp_tag_t)((int)((*attr)->value_tag) & IPP_TAG_CUPS_MASK);

  switch (value_tag)
  {
    case IPP_TAG_UNSUPPORTED_VALUE :
    case IPP_TAG_DEFAULT :
    case IPP_TAG_UNKNOWN :
    case IPP_TAG_NOVALUE :
    case IPP_TAG_NOTSETTABLE :
    case IPP_TAG_DELETEATTR :
    case IPP_TAG_ADMINDEFINE :
       /*
        * Free any existing values...
        */

        if ((*attr)->num_values > 0)
          ipp_free_values(*attr, 0, (*attr)->num_values);

       /*
        * Set out-of-band value...
        */

        (*attr)->value_tag = value_tag;
        break;

    case IPP_TAG_RANGE :
        if (temp_tag != IPP_TAG_INTEGER)
          return (0);

        for (i = (*attr)->num_values, value = (*attr)->values;
             i > 0;
             i --, value ++)
        {
          integer            = value->integer;
          value->range.lower = value->range.upper = integer;
        }

        (*attr)->value_tag = IPP_TAG_RANGE;
        break;

    case IPP_TAG_NAME :
        if (temp_tag != IPP_TAG_KEYWORD && temp_tag != IPP_TAG_URI &&
            temp_tag != IPP_TAG_URISCHEME && temp_tag != IPP_TAG_LANGUAGE &&
            temp_tag != IPP_TAG_MIMETYPE)
          return (0);

        (*attr)->value_tag = (ipp_tag_t)(IPP_TAG_NAME | ((*attr)->value_tag & IPP_TAG_CUPS_CONST));
        break;

    case IPP_TAG_NAMELANG :
    case IPP_TAG_TEXTLANG :
        if (value_tag == IPP_TAG_NAMELANG &&
            (temp_tag != IPP_TAG_NAME && temp_tag != IPP_TAG_KEYWORD &&
             temp_tag != IPP_TAG_URI && temp_tag != IPP_TAG_URISCHEME &&
             temp_tag != IPP_TAG_LANGUAGE && temp_tag != IPP_TAG_MIMETYPE))
          return (0);

        if (value_tag == IPP_TAG_TEXTLANG && temp_tag != IPP_TAG_TEXT)
          return (0);

        if (ipp->attrs && ipp->attrs->next && ipp->attrs->next->name &&
            !strcmp(ipp->attrs->next->name, "attributes-natural-language"))
        {
         /*
          * Use the language code from the IPP message...
          */

	  (*attr)->values[0].string.language =
	      _cupsStrAlloc(ipp->attrs->next->values[0].string.text);
        }
        else
        {
         /*
          * Otherwise, use the language code corresponding to the locale...
          */

	  language = cupsLangDefault();
	  (*attr)->values[0].string.language = _cupsStrAlloc(ipp_lang_code(language->language,
									code,
									sizeof(code)));
        }

        for (i = (*attr)->num_values - 1, value = (*attr)->values + 1;
             i > 0;
             i --, value ++)
          value->string.language = (*attr)->values[0].string.language;

        if ((int)(*attr)->value_tag & IPP_TAG_CUPS_CONST)
        {
         /*
          * Make copies of all values...
          */

	  for (i = (*attr)->num_values, value = (*attr)->values;
	       i > 0;
	       i --, value ++)
	    value->string.text = _cupsStrAlloc(value->string.text);
        }

        (*attr)->value_tag = IPP_TAG_NAMELANG;
        break;

    case IPP_TAG_KEYWORD :
        if (temp_tag == IPP_TAG_NAME || temp_tag == IPP_TAG_NAMELANG)
          break;			/* Silently "allow" name -> keyword */

    default :
        return (0);
  }

  return (1);
}