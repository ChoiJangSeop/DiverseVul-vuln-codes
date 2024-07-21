parse_option(const char *origin, option_string **stmt, char delm)
{
  char *retstr;
  char *ptr= (char *)origin;
  option_string **sptr= stmt;
  option_string *tmp;
  size_t length= strlen(origin);
  uint count= 0; /* We know that there is always one */

  for (tmp= *sptr= (option_string *)my_malloc(sizeof(option_string),
                                          MYF(MY_ZEROFILL|MY_FAE|MY_WME));
       (retstr= strchr(ptr, delm)); 
       tmp->next=  (option_string *)my_malloc(sizeof(option_string),
                                          MYF(MY_ZEROFILL|MY_FAE|MY_WME)),
       tmp= tmp->next)
  {
    char buffer[HUGE_STRING_LENGTH];
    char *buffer_ptr;

    count++;
    strncpy(buffer, ptr, (size_t)(retstr - ptr));
    if ((buffer_ptr= strchr(buffer, ':')))
    {
      char *option_ptr;

      tmp->length= (size_t)(buffer_ptr - buffer);
      tmp->string= my_strndup(ptr, (uint)tmp->length, MYF(MY_FAE));

      option_ptr= ptr + 1 + tmp->length;

      /* Move past the : and the first string */
      tmp->option_length= (size_t)(retstr - option_ptr);
      tmp->option= my_strndup(option_ptr, (uint)tmp->option_length,
                              MYF(MY_FAE));
    }
    else
    {
      tmp->string= my_strndup(ptr, (size_t)(retstr - ptr), MYF(MY_FAE));
      tmp->length= (size_t)(retstr - ptr);
    }

    ptr+= retstr - ptr + 1;
    if (isspace(*ptr))
      ptr++;
    count++;
  }

  if (ptr != origin+length)
  {
    char *origin_ptr;

    if ((origin_ptr= strchr(ptr, ':')))
    {
      char *option_ptr;

      tmp->length= (size_t)(origin_ptr - ptr);
      tmp->string= my_strndup(origin, tmp->length, MYF(MY_FAE));

      option_ptr= (char *)ptr + 1 + tmp->length;

      /* Move past the : and the first string */
      tmp->option_length= (size_t)((ptr + length) - option_ptr);
      tmp->option= my_strndup(option_ptr, tmp->option_length,
                              MYF(MY_FAE));
    }
    else
    {
      tmp->length= (size_t)((ptr + length) - ptr);
      tmp->string= my_strndup(ptr, tmp->length, MYF(MY_FAE));
    }

    count++;
  }

  return count;
}