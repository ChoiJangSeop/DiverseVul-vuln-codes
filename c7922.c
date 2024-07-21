string_interpret_escape(const uschar **pp)
{
#ifdef COMPILE_UTILITY
const uschar *hex_digits= CUS"0123456789abcdef";
#endif
int ch;
const uschar *p = *pp;
ch = *(++p);
if (isdigit(ch) && ch != '8' && ch != '9')
  {
  ch -= '0';
  if (isdigit(p[1]) && p[1] != '8' && p[1] != '9')
    {
    ch = ch * 8 + *(++p) - '0';
    if (isdigit(p[1]) && p[1] != '8' && p[1] != '9')
      ch = ch * 8 + *(++p) - '0';
    }
  }
else switch(ch)
  {
  case 'b':  ch = '\b'; break;
  case 'f':  ch = '\f'; break;
  case 'n':  ch = '\n'; break;
  case 'r':  ch = '\r'; break;
  case 't':  ch = '\t'; break;
  case 'v':  ch = '\v'; break;
  case 'x':
  ch = 0;
  if (isxdigit(p[1]))
    {
    ch = ch * 16 +
      Ustrchr(hex_digits, tolower(*(++p))) - hex_digits;
    if (isxdigit(p[1])) ch = ch * 16 +
      Ustrchr(hex_digits, tolower(*(++p))) - hex_digits;
    }
  break;
  }
*pp = p;
return ch;
}