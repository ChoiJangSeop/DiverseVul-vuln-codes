static PHP_GINIT_FUNCTION(pcntl)
{ 
	memset(pcntl_globals, 0, sizeof(*pcntl_globals));
}