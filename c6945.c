static void remove_trailing_spaces(char *string, size_t len)
{
  if(len<1) return; // not needed, b/c sizeof of make/model is 64
  string[len-1]=0;
  if(len<3) return; // also not needed
  len = strnlen(string,len-1);
  for(int i=len-1; i>=0; i--)
  {
    if(isspace(string[i]))
      string[i]=0;
    else
      break;
  }
}