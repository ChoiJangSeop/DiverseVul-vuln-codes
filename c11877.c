check_uuid(const char *uuid)
{
   const char *p;

   for (p = uuid; p[0]; p++)
     if ((!isalnum(*p)) && (*p != '-')) return EINA_FALSE;
   return EINA_TRUE;
}