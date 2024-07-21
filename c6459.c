MYSQL_DATA *mthd_my_read_rows(MYSQL *mysql,MYSQL_FIELD *mysql_fields,
			     uint fields)
{
  uint	field;
  ulong pkt_len;
  ulong len;
  uchar *cp;
  char	*to, *end_to;
  MYSQL_DATA *result;
  MYSQL_ROWS **prev_ptr,*cur;
  NET *net = &mysql->net;

  if ((pkt_len= ma_net_safe_read(mysql)) == packet_error)
    return(0);
  if (!(result=(MYSQL_DATA*) calloc(1, sizeof(MYSQL_DATA))))
  {
    SET_CLIENT_ERROR(mysql, CR_OUT_OF_MEMORY, SQLSTATE_UNKNOWN, 0);
    return(0);
  }
  ma_init_alloc_root(&result->alloc,8192,0);	/* Assume rowlength < 8192 */
  result->alloc.min_malloc=sizeof(MYSQL_ROWS);
  prev_ptr= &result->data;
  result->rows=0;
  result->fields=fields;

  while (*(cp=net->read_pos) != 254 || pkt_len >= 8)
  {
    result->rows++;
    if (!(cur= (MYSQL_ROWS*) ma_alloc_root(&result->alloc,
					    sizeof(MYSQL_ROWS))) ||
	      !(cur->data= ((MYSQL_ROW)
		      ma_alloc_root(&result->alloc,
				     (fields+1)*sizeof(char *)+fields+pkt_len))))
    {
      free_rows(result);
      SET_CLIENT_ERROR(mysql, CR_OUT_OF_MEMORY, SQLSTATE_UNKNOWN, 0);
      return(0);
    }
    *prev_ptr=cur;
    prev_ptr= &cur->next;
    to= (char*) (cur->data+fields+1);
    end_to=to+fields+pkt_len-1;
    for (field=0 ; field < fields ; field++)
    {
      if ((len=(ulong) net_field_length(&cp)) == NULL_LENGTH)
      {						/* null field */
        cur->data[field] = 0;
      }
      else
      {
        cur->data[field] = to;
        if (len > (ulong) (end_to - to))
        {
          free_rows(result);
          SET_CLIENT_ERROR(mysql, CR_UNKNOWN_ERROR, SQLSTATE_UNKNOWN, 0);
          return(0);
        }
        memcpy(to,(char*) cp,len); to[len]=0;
        to+=len+1;
        cp+=len;
        if (mysql_fields)
        {
          if (mysql_fields[field].max_length < len)
            mysql_fields[field].max_length=len;
         }
      }
    }
    cur->data[field]=to;			/* End of last field */
    if ((pkt_len=ma_net_safe_read(mysql)) == packet_error)
    {
      free_rows(result);
      return(0);
    }
  }
  *prev_ptr=0;					/* last pointer is null */
  /* save status */
  if (pkt_len > 1)
  {
    cp++;
    mysql->warning_count= uint2korr(cp);
    cp+= 2;
    mysql->server_status= uint2korr(cp);
  }
  return(result);
}