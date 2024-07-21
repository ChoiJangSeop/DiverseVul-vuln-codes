static char *Sys_PIDFileName( void )
{
	return va( "%s/%s", Sys_TempPath( ), PID_FILENAME );
}