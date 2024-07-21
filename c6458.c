int mthd_my_read_one_row(MYSQL *mysql,uint fields,MYSQL_ROW row, ulong *lengths)
{
  uint field;
  ulong pkt_len,len;
  uchar *pos,*prev_pos, *end_pos;

  if ((pkt_len=(uint) ma_net_safe_read(mysql)) == packet_error)
    return -1;

  if (pkt_len <= 8 && mysql->net.read_pos[0] == 254)
  {
    mysql->warning_count= uint2korr(mysql->net.read_pos + 1);
    mysql->server_status= uint2korr(mysql->net.read_pos + 3);
    return 1;				/* End of data */
  }
  prev_pos= 0;				/* allowed to write at packet[-1] */
  pos=mysql->net.read_pos;
  end_pos=pos+pkt_len;
  for (field=0 ; field < fields ; field++)
  {
    if ((len=(ulong) net_field_length(&pos)) == NULL_LENGTH)
    {						/* null field */
      row[field] = 0;
      *lengths++=0;
    }
    else
    {
      if (len > (ulong) (end_pos - pos))
      {
        mysql->net.last_errno=CR_UNKNOWN_ERROR;
        strcpy(mysql->net.last_error,ER(mysql->net.last_errno));
        return -1;
      }
      row[field] = (char*) pos;
      pos+=len;
      *lengths++=len;
    }
    if (prev_pos)
      *prev_pos=0;				/* Terminate prev field */
    prev_pos=pos;
  }
  row[field]=(char*) prev_pos+1;		/* End of last field */
  *prev_pos=0;					/* Terminate last field */
  return 0;
}