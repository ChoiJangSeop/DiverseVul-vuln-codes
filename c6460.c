unpack_fields(MYSQL_DATA *data,MA_MEM_ROOT *alloc,uint fields,
	      my_bool default_value, my_bool long_flag_protocol __attribute__((unused)))
{
  MYSQL_ROWS	*row;
  MYSQL_FIELD	*field,*result;
  char    *p;
  unsigned int i, field_count= sizeof(rset_field_offsets)/sizeof(size_t)/2;

  field=result=(MYSQL_FIELD*) ma_alloc_root(alloc,sizeof(MYSQL_FIELD)*fields);
  if (!result)
    return(0);

  for (row=data->data; row ; row = row->next,field++)
  {
    for (i=0; i < field_count; i++)
    {
      switch(row->data[i][0]) {
      case 0:
       *(char **)(((char *)field) + rset_field_offsets[i*2])= ma_strdup_root(alloc, "");
       *(unsigned int *)(((char *)field) + rset_field_offsets[i*2+1])= 0;
       break;
     default:
       *(char **)(((char *)field) + rset_field_offsets[i*2])=
         ma_strdup_root(alloc, (char *)row->data[i]);
       *(unsigned int *)(((char *)field) + rset_field_offsets[i*2+1])=
         (uint)(row->data[i+1] - row->data[i] - 1);
       break;
      }
    }

    p= (char *)row->data[6];
    /* filler */
    field->charsetnr= uint2korr(p);
    p+= 2;
    field->length= (uint) uint4korr(p);
    p+= 4;
    field->type=   (enum enum_field_types)uint1korr(p);
    p++;
    field->flags= uint2korr(p);
    p+= 2;
    field->decimals= (uint) p[0];
    p++;

    /* filler */
    p+= 2;

    if (INTERNAL_NUM_FIELD(field))
      field->flags|= NUM_FLAG;

    if (default_value && row->data[7])
      field->def=ma_strdup_root(alloc,(char*) row->data[7]);
    else
      field->def=0;
    field->max_length= 0;
  }
  free_rows(data);				/* Free old data */
  return(result);
}