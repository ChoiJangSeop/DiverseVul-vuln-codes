fribidi_log2vis (
  /* input */
  const FriBidiChar *str,
  FriBidiStrIndex len,
  /* input and output */
  FriBidiParType *pbase_dir,
  /* output */
  FriBidiChar *visual_str,
  FriBidiStrIndex *positions_L_to_V,
  FriBidiStrIndex *positions_V_to_L,
  FriBidiLevel *embedding_levels
)
{
  register FriBidiStrIndex i;
  FriBidiLevel max_level = 0;
  fribidi_boolean private_V_to_L = false;
  fribidi_boolean private_embedding_levels = false;
  fribidi_boolean status = false;
  FriBidiArabicProp *ar_props = NULL;
  FriBidiCharType *bidi_types = NULL;
  FriBidiBracketType *bracket_types = NULL;

  if UNLIKELY
    (len == 0)
    {
      status = true;
      goto out;
    }

  DBG ("in fribidi_log2vis");

  fribidi_assert (str);
  fribidi_assert (pbase_dir);

  bidi_types = fribidi_malloc (len * sizeof bidi_types[0]);
  if (!bidi_types)
    goto out;

  fribidi_get_bidi_types (str, len, bidi_types);

  bracket_types = fribidi_malloc (len * sizeof bracket_types[0]);
  if (!bracket_types)
    goto out;

  fribidi_get_bracket_types (str, len, bidi_types,
                             /* output */
                             bracket_types);
  if (!embedding_levels)
    {
      embedding_levels = fribidi_malloc (len * sizeof embedding_levels[0]);
      if (!embedding_levels)
	goto out;
      private_embedding_levels = true;
    }

  max_level = fribidi_get_par_embedding_levels_ex (bidi_types,
                                                   bracket_types,
                                                   len,
                                                   pbase_dir,
                                                   embedding_levels) - 1;
  if UNLIKELY
    (max_level < 0) goto out;

  /* If l2v is to be calculated we must have v2l as well. If it is not
     given by the caller, we have to make a private instance of it. */
  if (positions_L_to_V && !positions_V_to_L)
    {
      positions_V_to_L =
	(FriBidiStrIndex *) fribidi_malloc (sizeof (FriBidiStrIndex) * len);
      if (!positions_V_to_L)
	goto out;
      private_V_to_L = true;
    }

  /* Set up the ordering array to identity order */
  if (positions_V_to_L)
    {
      for (i = 0; i < len; i++)
	positions_V_to_L[i] = i;
    }


  if (visual_str)
    {
      /* Using memcpy instead
      for (i = len - 1; i >= 0; i--)
	visual_str[i] = str[i];
      */
      memcpy (visual_str, str, len * sizeof (*visual_str));

      /* Arabic joining */
      ar_props = fribidi_malloc (len * sizeof ar_props[0]);
      fribidi_get_joining_types (str, len, ar_props);
      fribidi_join_arabic (bidi_types, len, embedding_levels, ar_props);

      fribidi_shape (flags, embedding_levels, len, ar_props, visual_str);
    }

  /* line breaking goes here, but we assume one line in this function */

  /* and this should be called once per line, but again, we assume one
   * line in this deprecated function */
  status =
    fribidi_reorder_line (flags, bidi_types, len, 0, *pbase_dir,
			  embedding_levels, visual_str,
			  positions_V_to_L);

  /* Convert the v2l list to l2v */
  if (positions_L_to_V)
    {
      for (i = 0; i < len; i++)
	positions_L_to_V[i] = -1;
      for (i = 0; i < len; i++)
	positions_L_to_V[positions_V_to_L[i]] = i;
    }

out:

  if (private_V_to_L)
    fribidi_free (positions_V_to_L);

  if (private_embedding_levels)
    fribidi_free (embedding_levels);

  if (ar_props)
    fribidi_free (ar_props);

  if (bidi_types)
    fribidi_free (bidi_types);

  if (bracket_types)
    fribidi_free (bracket_types);

  return status ? max_level + 1 : 0;
}