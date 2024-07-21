static void gdCtxPrintf(gdIOCtx * out, const char *format, ...)
{
	char buf[4096];
	int len;
	va_list args;

	va_start(args, format);
	len = vsnprintf(buf, sizeof(buf)-1, format, args);
	va_end(args);
	out->putBuf(out, buf, len);
}