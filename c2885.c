Debug(const char *fmtstr,
      va_list args)
{
   /* Ignored */
#ifdef VMX86_DEBUG
   sLog(log_debug, "Debug callback invoked. \n");
#endif
}