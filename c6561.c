static char *InterpretDelegateProperties(ImageInfo *image_info,
  Image *image,const char *embed_text,ExceptionInfo *exception)
{
#define ExtendInterpretText(string_length) \
DisableMSCWarning(4127) \
{ \
  size_t length=(string_length); \
  if ((size_t) (q-interpret_text+length+1) >= extent) \
    { \
      extent+=length; \
      interpret_text=(char *) ResizeQuantumMemory(interpret_text,extent+ \
        MaxTextExtent,sizeof(*interpret_text)); \
      if (interpret_text == (char *) NULL) \
        return((char *) NULL); \
      q=interpret_text+strlen(interpret_text); \
   } \
} \
RestoreMSCWarning

#define AppendKeyValue2Text(key,value)\
DisableMSCWarning(4127) \
{ \
  size_t length=strlen(key)+strlen(value)+2; \
  if ((size_t) (q-interpret_text+length+1) >= extent) \
    { \
      extent+=length; \
      interpret_text=(char *) ResizeQuantumMemory(interpret_text,extent+ \
        MaxTextExtent,sizeof(*interpret_text)); \
      if (interpret_text == (char *) NULL) \
        return((char *) NULL); \
      q=interpret_text+strlen(interpret_text); \
     } \
   q+=FormatLocaleString(q,extent,"%s=%s\n",(key),(value)); \
} \
RestoreMSCWarning

#define AppendString2Text(string) \
DisableMSCWarning(4127) \
{ \
  size_t length=strlen((string)); \
  if ((size_t) (q-interpret_text+length+1) >= extent) \
    { \
      extent+=length; \
      interpret_text=(char *) ResizeQuantumMemory(interpret_text,extent+ \
        MaxTextExtent,sizeof(*interpret_text)); \
      if (interpret_text == (char *) NULL) \
        return((char *) NULL); \
      q=interpret_text+strlen(interpret_text); \
    } \
  (void) CopyMagickString(q,(string),extent); \
  q+=length; \
} \
RestoreMSCWarning

  char
    *interpret_text,
    *string;

  register char
    *q;  /* current position in interpret_text */

  register const char
    *p;  /* position in embed_text string being expanded */

  size_t
    extent;  /* allocated length of interpret_text */

  MagickBooleanType
    number;

  assert(image == NULL || image->signature == MagickCoreSignature);
  assert(image_info == NULL || image_info->signature == MagickCoreSignature);
  if ((image != (Image *) NULL) && (image->debug != MagickFalse))
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  else
   if ((image_info != (ImageInfo *) NULL) && (image_info->debug != MagickFalse))
     (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s","no-image");
  if (embed_text == (const char *) NULL)
    return(ConstantString(""));
  p=embed_text;
  while ((isspace((int) ((unsigned char) *p)) != 0) && (*p != '\0'))
    p++;
  if (*p == '\0')
    return(ConstantString(""));
  /*
    Translate any embedded format characters.
  */
  interpret_text=AcquireString(embed_text);  /* new string with extra space */
  extent=MagickPathExtent;  /* allocated space in string */
  number=MagickFalse;  /* is last char a number? */
  for (q=interpret_text; *p!='\0';
    number=isdigit((int) ((unsigned char) *p)) ? MagickTrue : MagickFalse,p++)
  {
    /*
      Interpret escape characters (e.g. Filename: %M).
    */
    *q='\0';
    ExtendInterpretText(MagickPathExtent);
    switch (*p)
    {
      case '\\':
      {
        switch (*(p+1))
        {
          case '\0':
            continue;
          case 'r':  /* convert to RETURN */
          {
            *q++='\r';
            p++;
            continue;
          }
          case 'n':  /* convert to NEWLINE */
          {
            *q++='\n';
            p++;
            continue;
          }
          case '\n':  /* EOL removal UNIX,MacOSX */
          {
            p++;
            continue;
          }
          case '\r':  /* EOL removal DOS,Windows */
          {
            p++;
            if (*p == '\n') /* return-newline EOL */
              p++;
            continue;
          }
          default:
          {
            p++;
            *q++=(*p);
          }
        }
        continue;
      }
      case '&':
      {
        if (LocaleNCompare("&lt;",p,4) == 0)
          {
            *q++='<';
            p+=3;
          }
        else
          if (LocaleNCompare("&gt;",p,4) == 0)
            {
              *q++='>';
              p+=3;
            }
          else
            if (LocaleNCompare("&amp;",p,5) == 0)
              {
                *q++='&';
                p+=4;
              }
            else
              *q++=(*p);
        continue;
      }
      case '%':
        break;  /* continue to next set of handlers */
      default:
      {
        *q++=(*p);  /* any thing else is 'as normal' */
        continue;
      }
    }
    p++; /* advance beyond the percent */
    /*
      Doubled Percent - or percent at end of string.
    */
    if ((*p == '\0') || (*p == '\'') || (*p == '"'))
      p--;
    if (*p == '%')
      {
        *q++='%';
        continue;
      }
    /*
      Single letter escapes %c.
    */
    if (number != MagickFalse)
      {
        /*
          But only if not preceeded by a number!
        */
        *q++='%'; /* do NOT substitute the percent */
        p--;      /* back up one */
        continue;
      }
    string=GetMagickPropertyLetter(image_info,image,*p, exception);
    if (string != (char *) NULL)
      {
        AppendString2Text(string);
        string=DestroyString(string);
        continue;
      }
    (void) ThrowMagickException(exception,GetMagickModule(),OptionWarning,
      "UnknownImageProperty","\"%%%c\"",*p);
  }
  *q='\0';
  return(interpret_text);
}