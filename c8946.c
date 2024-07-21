get_filter_list( Operation *op, BerElement *ber,
	Filter **f,
	const char **text )
{
	Filter		**new;
	int		err;
	ber_tag_t	tag;
	ber_len_t	len;
	char		*last;

	Debug( LDAP_DEBUG_FILTER, "begin get_filter_list\n", 0, 0, 0 );
	new = f;
	for ( tag = ber_first_element( ber, &len, &last );
		tag != LBER_DEFAULT;
		tag = ber_next_element( ber, &len, last ) )
	{
		err = get_filter( op, ber, new, text );
		if ( err != LDAP_SUCCESS )
			return( err );
		new = &(*new)->f_next;
	}
	*new = NULL;

	Debug( LDAP_DEBUG_FILTER, "end get_filter_list\n", 0, 0, 0 );
	return( LDAP_SUCCESS );
}