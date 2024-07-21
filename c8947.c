get_filter(
	Operation *op,
	BerElement *ber,
	Filter **filt,
	const char **text )
{
	ber_tag_t	tag;
	ber_len_t	len;
	int		err;
	Filter		f;

	Debug( LDAP_DEBUG_FILTER, "begin get_filter\n", 0, 0, 0 );
	/*
	 * A filter looks like this coming in:
	 *	Filter ::= CHOICE {
	 *		and		[0]	SET OF Filter,
	 *		or		[1]	SET OF Filter,
	 *		not		[2]	Filter,
	 *		equalityMatch	[3]	AttributeValueAssertion,
	 *		substrings	[4]	SubstringFilter,
	 *		greaterOrEqual	[5]	AttributeValueAssertion,
	 *		lessOrEqual	[6]	AttributeValueAssertion,
	 *		present		[7]	AttributeType,
	 *		approxMatch	[8]	AttributeValueAssertion,
	 *		extensibleMatch [9]	MatchingRuleAssertion
	 *	}
	 *
	 *	SubstringFilter ::= SEQUENCE {
	 *		type		   AttributeType,
	 *		SEQUENCE OF CHOICE {
	 *			initial		 [0] IA5String,
	 *			any		 [1] IA5String,
	 *			final		 [2] IA5String
	 *		}
	 *	}
	 *
	 *	MatchingRuleAssertion ::= SEQUENCE {
	 *		matchingRule	[1] MatchingRuleId OPTIONAL,
	 *		type		[2] AttributeDescription OPTIONAL,
	 *		matchValue	[3] AssertionValue,
	 *		dnAttributes	[4] BOOLEAN DEFAULT FALSE
	 *	}
	 *
	 */

	tag = ber_peek_tag( ber, &len );

	if( tag == LBER_ERROR ) {
		*text = "error decoding filter";
		return SLAPD_DISCONNECT;
	}

	err = LDAP_SUCCESS;

	f.f_next = NULL;
	f.f_choice = tag; 

	switch ( f.f_choice ) {
	case LDAP_FILTER_EQUALITY:
		Debug( LDAP_DEBUG_FILTER, "EQUALITY\n", 0, 0, 0 );
		err = get_ava( op, ber, &f, SLAP_MR_EQUALITY, text );
		if ( err != LDAP_SUCCESS ) {
			break;
		}

		assert( f.f_ava != NULL );
		break;

	case LDAP_FILTER_SUBSTRINGS:
		Debug( LDAP_DEBUG_FILTER, "SUBSTRINGS\n", 0, 0, 0 );
		err = get_ssa( op, ber, &f, text );
		if( err != LDAP_SUCCESS ) {
			break;
		}
		assert( f.f_sub != NULL );
		break;

	case LDAP_FILTER_GE:
		Debug( LDAP_DEBUG_FILTER, "GE\n", 0, 0, 0 );
		err = get_ava( op, ber, &f, SLAP_MR_ORDERING, text );
		if ( err != LDAP_SUCCESS ) {
			break;
		}
		assert( f.f_ava != NULL );
		break;

	case LDAP_FILTER_LE:
		Debug( LDAP_DEBUG_FILTER, "LE\n", 0, 0, 0 );
		err = get_ava( op, ber, &f, SLAP_MR_ORDERING, text );
		if ( err != LDAP_SUCCESS ) {
			break;
		}
		assert( f.f_ava != NULL );
		break;

	case LDAP_FILTER_PRESENT: {
		struct berval type;

		Debug( LDAP_DEBUG_FILTER, "PRESENT\n", 0, 0, 0 );
		if ( ber_scanf( ber, "m", &type ) == LBER_ERROR ) {
			err = SLAPD_DISCONNECT;
			*text = "error decoding filter";
			break;
		}

		f.f_desc = NULL;
		err = slap_bv2ad( &type, &f.f_desc, text );

		if( err != LDAP_SUCCESS ) {
			f.f_choice |= SLAPD_FILTER_UNDEFINED;
			err = slap_bv2undef_ad( &type, &f.f_desc, text,
				SLAP_AD_PROXIED|SLAP_AD_NOINSERT );

			if ( err != LDAP_SUCCESS ) {
				/* unrecognized attribute description or other error */
				Debug( LDAP_DEBUG_ANY, 
					"get_filter: conn %lu unknown attribute "
					"type=%s (%d)\n",
					op->o_connid, type.bv_val, err );

				err = LDAP_SUCCESS;
				f.f_desc = slap_bv2tmp_ad( &type, op->o_tmpmemctx );
			}
			*text = NULL;
		}

		assert( f.f_desc != NULL );
		} break;

	case LDAP_FILTER_APPROX:
		Debug( LDAP_DEBUG_FILTER, "APPROX\n", 0, 0, 0 );
		err = get_ava( op, ber, &f, SLAP_MR_EQUALITY_APPROX, text );
		if ( err != LDAP_SUCCESS ) {
			break;
		}
		assert( f.f_ava != NULL );
		break;

	case LDAP_FILTER_AND:
		Debug( LDAP_DEBUG_FILTER, "AND\n", 0, 0, 0 );
		err = get_filter_list( op, ber, &f.f_and, text );
		if ( err != LDAP_SUCCESS ) {
			break;
		}
		if ( f.f_and == NULL ) {
			f.f_choice = SLAPD_FILTER_COMPUTED;
			f.f_result = LDAP_COMPARE_TRUE;
		}
		/* no assert - list could be empty */
		break;

	case LDAP_FILTER_OR:
		Debug( LDAP_DEBUG_FILTER, "OR\n", 0, 0, 0 );
		err = get_filter_list( op, ber, &f.f_or, text );
		if ( err != LDAP_SUCCESS ) {
			break;
		}
		if ( f.f_or == NULL ) {
			f.f_choice = SLAPD_FILTER_COMPUTED;
			f.f_result = LDAP_COMPARE_FALSE;
		}
		/* no assert - list could be empty */
		break;

	case LDAP_FILTER_NOT:
		Debug( LDAP_DEBUG_FILTER, "NOT\n", 0, 0, 0 );
		(void) ber_skip_tag( ber, &len );
		err = get_filter( op, ber, &f.f_not, text );
		if ( err != LDAP_SUCCESS ) {
			break;
		}

		assert( f.f_not != NULL );
		if ( f.f_not->f_choice == SLAPD_FILTER_COMPUTED ) {
			int fresult = f.f_not->f_result;
			f.f_choice = SLAPD_FILTER_COMPUTED;
			op->o_tmpfree( f.f_not, op->o_tmpmemctx );
			f.f_not = NULL;

			switch( fresult ) {
			case LDAP_COMPARE_TRUE:
				f.f_result = LDAP_COMPARE_FALSE;
				break;
			case LDAP_COMPARE_FALSE:
				f.f_result = LDAP_COMPARE_TRUE;
				break;
			default: ;
				/* (!Undefined) is Undefined */
			}
		}
		break;

	case LDAP_FILTER_EXT:
		Debug( LDAP_DEBUG_FILTER, "EXTENSIBLE\n", 0, 0, 0 );

		err = get_mra( op, ber, &f, text );
		if ( err != LDAP_SUCCESS ) {
			break;
		}

		assert( f.f_mra != NULL );
		break;

	default:
		(void) ber_scanf( ber, "x" ); /* skip the element */
		Debug( LDAP_DEBUG_ANY, "get_filter: unknown filter type=%lu\n",
			f.f_choice, 0, 0 );
		f.f_choice = SLAPD_FILTER_COMPUTED;
		f.f_result = SLAPD_COMPARE_UNDEFINED;
		break;
	}

	if( err != LDAP_SUCCESS && err != SLAPD_DISCONNECT ) {
		/* ignore error */
		*text = NULL;
		f.f_choice = SLAPD_FILTER_COMPUTED;
		f.f_result = SLAPD_COMPARE_UNDEFINED;
		err = LDAP_SUCCESS;
	}

	if ( err == LDAP_SUCCESS ) {
		*filt = op->o_tmpalloc( sizeof(f), op->o_tmpmemctx );
		**filt = f;
	}

	Debug( LDAP_DEBUG_FILTER, "end get_filter %d\n", err, 0, 0 );

	return( err );
}