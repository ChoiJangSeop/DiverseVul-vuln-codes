const char *Sys_TempPath( void )
{
	static TCHAR path[ MAX_PATH ];
	DWORD length;

	length = GetTempPath( sizeof( path ), path );

	if( length > sizeof( path ) || length == 0 )
		return Sys_DefaultHomePath( );
	else
		return path;
}