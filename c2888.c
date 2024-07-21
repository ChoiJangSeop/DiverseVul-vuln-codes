Debug(const char *fmtstr,
      va_list args)
{
   /* Ignored */
#ifdef VMX86_DEBUG
   sLog(log_warning, "Debug callback invoked. \n");
#endif
}