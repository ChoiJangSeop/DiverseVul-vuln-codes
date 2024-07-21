void create_partition_name(char *out, const char *in1,
                           const char *in2, uint name_variant,
                           bool translate)
{
  char transl_part_name[FN_REFLEN];
  const char *transl_part;

  if (translate)
  {
    tablename_to_filename(in2, transl_part_name, FN_REFLEN);
    transl_part= transl_part_name;
  }
  else
    transl_part= in2;
  if (name_variant == NORMAL_PART_NAME)
    strxmov(out, in1, "#P#", transl_part, NullS);
  else if (name_variant == TEMP_PART_NAME)
    strxmov(out, in1, "#P#", transl_part, "#TMP#", NullS);
  else if (name_variant == RENAMED_PART_NAME)
    strxmov(out, in1, "#P#", transl_part, "#REN#", NullS);
}