Panic(const char *fmtstr, ...)
{
   /* Ignored */
   sLog(log_warning, "Panic callback invoked. \n");
   exit(1);
}