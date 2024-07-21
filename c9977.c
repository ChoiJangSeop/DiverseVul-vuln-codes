authzValidate(
	Syntax *syntax,
	struct berval *in )
{
	struct berval	bv;
	int		rc = LDAP_INVALID_SYNTAX;
	LDAPURLDesc	*ludp = NULL;
	int		scope = -1;

	/*
	 * 1) <DN>
	 * 2) dn[.{exact|children|subtree|onelevel}]:{*|<DN>}
	 * 3) dn.regex:<pattern>
	 * 4) u[.mech[/realm]]:<ID>
	 * 5) group[/<groupClass>[/<memberAttr>]]:<DN>
	 * 6) <URL>
	 */

	assert( in != NULL );
	assert( !BER_BVISNULL( in ) );

	Debug( LDAP_DEBUG_TRACE,
		"authzValidate: parsing %s\n", in->bv_val, 0, 0 );

	/*
	 * 2) dn[.{exact|children|subtree|onelevel}]:{*|<DN>}
	 * 3) dn.regex:<pattern>
	 *
	 * <DN> must pass DN normalization
	 */
	if ( !strncasecmp( in->bv_val, "dn", STRLENOF( "dn" ) ) ) {
		bv.bv_val = in->bv_val + STRLENOF( "dn" );

		if ( bv.bv_val[ 0 ] == '.' ) {
			bv.bv_val++;

			if ( !strncasecmp( bv.bv_val, "exact:", STRLENOF( "exact:" ) ) ) {
				bv.bv_val += STRLENOF( "exact:" );
				scope = LDAP_X_SCOPE_EXACT;

			} else if ( !strncasecmp( bv.bv_val, "regex:", STRLENOF( "regex:" ) ) ) {
				bv.bv_val += STRLENOF( "regex:" );
				scope = LDAP_X_SCOPE_REGEX;

			} else if ( !strncasecmp( bv.bv_val, "children:", STRLENOF( "children:" ) ) ) {
				bv.bv_val += STRLENOF( "children:" );
				scope = LDAP_X_SCOPE_CHILDREN;

			} else if ( !strncasecmp( bv.bv_val, "subtree:", STRLENOF( "subtree:" ) ) ) {
				bv.bv_val += STRLENOF( "subtree:" );
				scope = LDAP_X_SCOPE_SUBTREE;

			} else if ( !strncasecmp( bv.bv_val, "onelevel:", STRLENOF( "onelevel:" ) ) ) {
				bv.bv_val += STRLENOF( "onelevel:" );
				scope = LDAP_X_SCOPE_ONELEVEL;

			} else {
				return LDAP_INVALID_SYNTAX;
			}

		} else {
			if ( bv.bv_val[ 0 ] != ':' ) {
				return LDAP_INVALID_SYNTAX;
			}
			scope = LDAP_X_SCOPE_EXACT;
			bv.bv_val++;
		}

		bv.bv_val += strspn( bv.bv_val, " " );
		/* jump here in case no type specification was present
		 * and uri was not an URI... HEADS-UP: assuming EXACT */
is_dn:		bv.bv_len = in->bv_len - ( bv.bv_val - in->bv_val );

		/* a single '*' means any DN without using regexes */
		if ( ber_bvccmp( &bv, '*' ) ) {
			/* LDAP_X_SCOPE_USERS */
			return LDAP_SUCCESS;
		}

		switch ( scope ) {
		case LDAP_X_SCOPE_EXACT:
		case LDAP_X_SCOPE_CHILDREN:
		case LDAP_X_SCOPE_SUBTREE:
		case LDAP_X_SCOPE_ONELEVEL:
			return dnValidate( NULL, &bv );

		case LDAP_X_SCOPE_REGEX:
			return LDAP_SUCCESS;
		}

		return rc;

	/*
	 * 4) u[.mech[/realm]]:<ID>
	 */
	} else if ( ( in->bv_val[ 0 ] == 'u' || in->bv_val[ 0 ] == 'U' )
			&& ( in->bv_val[ 1 ] == ':' 
				|| in->bv_val[ 1 ] == '/' 
				|| in->bv_val[ 1 ] == '.' ) )
	{
		char		buf[ SLAP_LDAPDN_MAXLEN ];
		struct berval	id,
				user = BER_BVNULL,
				realm = BER_BVNULL,
				mech = BER_BVNULL;

		if ( sizeof( buf ) <= in->bv_len ) {
			return LDAP_INVALID_SYNTAX;
		}

		id.bv_len = in->bv_len;
		id.bv_val = buf;
		strncpy( buf, in->bv_val, sizeof( buf ) );

		rc = slap_parse_user( &id, &user, &realm, &mech );
		if ( rc != LDAP_SUCCESS ) {
			return LDAP_INVALID_SYNTAX;
		}

		return rc;

	/*
	 * 5) group[/groupClass[/memberAttr]]:<DN>
	 *
	 * <groupClass> defaults to "groupOfNames"
	 * <memberAttr> defaults to "member"
	 * 
	 * <DN> must pass DN normalization
	 */
	} else if ( strncasecmp( in->bv_val, "group", STRLENOF( "group" ) ) == 0 )
	{
		struct berval	group_dn = BER_BVNULL,
				group_oc = BER_BVNULL,
				member_at = BER_BVNULL;

		bv.bv_val = in->bv_val + STRLENOF( "group" );
		bv.bv_len = in->bv_len - STRLENOF( "group" );
		group_dn.bv_val = ber_bvchr( &bv, ':' );
		if ( group_dn.bv_val == NULL ) {
			/* last chance: assume it's a(n exact) DN ... */
			bv.bv_val = in->bv_val;
			scope = LDAP_X_SCOPE_EXACT;
			goto is_dn;
		}
		
		/*
		 * FIXME: we assume that "member" and "groupOfNames"
		 * are present in schema...
		 */
		if ( bv.bv_val[ 0 ] == '/' ) {
			group_oc.bv_val = &bv.bv_val[ 1 ];
			group_oc.bv_len = group_dn.bv_val - group_oc.bv_val;

			member_at.bv_val = ber_bvchr( &group_oc, '/' );
			if ( member_at.bv_val ) {
				AttributeDescription	*ad = NULL;
				const char		*text = NULL;

				group_oc.bv_len = member_at.bv_val - group_oc.bv_val;
				member_at.bv_val++;
				member_at.bv_len = group_dn.bv_val - member_at.bv_val;
				rc = slap_bv2ad( &member_at, &ad, &text );
				if ( rc != LDAP_SUCCESS ) {
					return rc;
				}
			}

			if ( oc_bvfind( &group_oc ) == NULL ) {
				return LDAP_INVALID_SYNTAX;
			}
		}

		group_dn.bv_val++;
		group_dn.bv_len = in->bv_len - ( group_dn.bv_val - in->bv_val );

		rc = dnValidate( NULL, &group_dn );
		if ( rc != LDAP_SUCCESS ) {
			return rc;
		}

		return rc;
	}

	/*
	 * ldap:///<base>??<scope>?<filter>
	 * <scope> ::= {base|one|subtree}
	 *
	 * <scope> defaults to "base"
	 * <base> must pass DN normalization
	 * <filter> must pass str2filter()
	 */
	rc = ldap_url_parse( in->bv_val, &ludp );
	switch ( rc ) {
	case LDAP_URL_SUCCESS:
		/* FIXME: the check is pedantic, but I think it's necessary,
		 * because people tend to use things like ldaps:// which
		 * gives the idea SSL is being used.  Maybe we could
		 * accept ldapi:// as well, but the point is that we use
		 * an URL as an easy means to define bits of a search with
		 * little parsing.
		 */
		if ( strcasecmp( ludp->lud_scheme, "ldap" ) != 0 ) {
			/*
			 * must be ldap:///
			 */
			rc = LDAP_INVALID_SYNTAX;
			goto done;
		}
		break;

	case LDAP_URL_ERR_BADSCHEME:
		/*
		 * last chance: assume it's a(n exact) DN ...
		 *
		 * NOTE: must pass DN normalization
		 */
		ldap_free_urldesc( ludp );
		bv.bv_val = in->bv_val;
		scope = LDAP_X_SCOPE_EXACT;
		goto is_dn;

	default:
		rc = LDAP_INVALID_SYNTAX;
		goto done;
	}

	if ( ( ludp->lud_host && *ludp->lud_host )
		|| ludp->lud_attrs || ludp->lud_exts )
	{
		/* host part must be empty */
		/* attrs and extensions parts must be empty */
		rc = LDAP_INVALID_SYNTAX;
		goto done;
	}

	/* Grab the filter */
	if ( ludp->lud_filter ) {
		Filter	*f = str2filter( ludp->lud_filter );
		if ( f == NULL ) {
			rc = LDAP_INVALID_SYNTAX;
			goto done;
		}
		filter_free( f );
	}

	/* Grab the searchbase */
	assert( ludp->lud_dn != NULL );
	ber_str2bv( ludp->lud_dn, 0, 0, &bv );
	rc = dnValidate( NULL, &bv );

done:
	ldap_free_urldesc( ludp );
	return( rc );
}