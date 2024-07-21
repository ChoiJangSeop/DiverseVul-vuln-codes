backsql_process_filter_attr( backsql_srch_info *bsi, Filter *f, backsql_at_map_rec *at )
{
	backsql_info		*bi = (backsql_info *)bsi->bsi_op->o_bd->be_private;
	int			casefold = 0;
	struct berval		*filter_value = NULL;
	MatchingRule		*matching_rule = NULL;
	struct berval		ordering = BER_BVC("<=");

	Debug( LDAP_DEBUG_TRACE, "==>backsql_process_filter_attr(%s)\n",
		at->bam_ad->ad_cname.bv_val );

	/*
	 * need to add this attribute to list of attrs to load,
	 * so that we can do test_filter() later
	 */
	backsql_attrlist_add( bsi, at->bam_ad );

	backsql_merge_from_tbls( bsi, &at->bam_from_tbls );

	if ( !BER_BVISNULL( &at->bam_join_where )
			&& strstr( bsi->bsi_join_where.bb_val.bv_val,
				at->bam_join_where.bv_val ) == NULL )
	{
	       	backsql_strfcat_x( &bsi->bsi_join_where,
				bsi->bsi_op->o_tmpmemctx,
				"lb",
				(ber_len_t)STRLENOF( " AND " ), " AND ",
				&at->bam_join_where );
	}

	if ( f->f_choice & SLAPD_FILTER_UNDEFINED ) {
		backsql_strfcat_x( &bsi->bsi_flt_where,
			bsi->bsi_op->o_tmpmemctx,
			"l",
			(ber_len_t)STRLENOF( "1=0" ), "1=0" );
		return 1;
	}

	switch ( f->f_choice ) {
	case LDAP_FILTER_EQUALITY:
		filter_value = &f->f_av_value;
		matching_rule = at->bam_ad->ad_type->sat_equality;

		goto equality_match;

		/* fail over into next case */
		
	case LDAP_FILTER_EXT:
		filter_value = &f->f_mra->ma_value;
		matching_rule = f->f_mr_rule;

equality_match:;
		/* always uppercase strings by now */
#ifdef BACKSQL_UPPERCASE_FILTER
		if ( SLAP_MR_ASSOCIATED( matching_rule,
					bi->sql_caseIgnoreMatch ) )
#endif /* BACKSQL_UPPERCASE_FILTER */
		{
			casefold = 1;
		}

		/* FIXME: directoryString filtering should use a similar
		 * approach to deal with non-prettified values like
		 * " A  non    prettified   value  ", by using a LIKE
		 * filter with all whitespaces collapsed to a single '%' */
		if ( SLAP_MR_ASSOCIATED( matching_rule,
					bi->sql_telephoneNumberMatch ) )
		{
			struct berval	bv;
			ber_len_t	i;

			/*
			 * to check for matching telephone numbers
			 * with intermized chars, e.g. val='1234'
			 * use
			 * 
			 * val LIKE '%1%2%3%4%'
			 */

			bv.bv_len = 2 * filter_value->bv_len - 1;
			bv.bv_val = ch_malloc( bv.bv_len + 1 );

			bv.bv_val[ 0 ] = filter_value->bv_val[ 0 ];
			for ( i = 1; i < filter_value->bv_len; i++ ) {
				bv.bv_val[ 2 * i - 1 ] = '%';
				bv.bv_val[ 2 * i ] = filter_value->bv_val[ i ];
			}
			bv.bv_val[ 2 * i - 1 ] = '\0';

			(void)backsql_process_filter_like( bsi, at, casefold, &bv );
			ch_free( bv.bv_val );

			break;
		}

		/* NOTE: this is required by objectClass inheritance 
		 * and auxiliary objectClass use in filters for slightly
		 * more efficient candidate selection. */
		/* FIXME: a bit too many specializations to deal with
		 * very specific cases... */
		if ( at->bam_ad == slap_schema.si_ad_objectClass
				|| at->bam_ad == slap_schema.si_ad_structuralObjectClass )
		{
			backsql_strfcat_x( &bsi->bsi_flt_where,
					bsi->bsi_op->o_tmpmemctx,
					"lbl",
					(ber_len_t)STRLENOF( "(ldap_entries.id=ldap_entry_objclasses.entry_id AND ldap_entry_objclasses.oc_name='" /* ') */ ),
						"(ldap_entries.id=ldap_entry_objclasses.entry_id AND ldap_entry_objclasses.oc_name='" /* ') */,
					filter_value,
					(ber_len_t)STRLENOF( /* (' */ "')" ),
						/* (' */ "')" );
			break;
		}

		/*
		 * maybe we should check type of at->sel_expr here somehow,
		 * to know whether upper_func is applicable, but for now
		 * upper_func stuff is made for Oracle, where UPPER is
		 * safely applicable to NUMBER etc.
		 */
		(void)backsql_process_filter_eq( bsi, at, casefold, filter_value );
		break;

	case LDAP_FILTER_GE:
		ordering.bv_val = ">=";

		/* fall thru to next case */
		
	case LDAP_FILTER_LE:
		filter_value = &f->f_av_value;
		
		/* always uppercase strings by now */
#ifdef BACKSQL_UPPERCASE_FILTER
		if ( at->bam_ad->ad_type->sat_ordering &&
				SLAP_MR_ASSOCIATED( at->bam_ad->ad_type->sat_ordering,
					bi->sql_caseIgnoreMatch ) )
#endif /* BACKSQL_UPPERCASE_FILTER */
		{
			casefold = 1;
		}

		/*
		 * FIXME: should we uppercase the operands?
		 */
		if ( casefold && BACKSQL_AT_CANUPPERCASE( at ) ) {
			ber_len_t	start;

			backsql_strfcat_x( &bsi->bsi_flt_where,
					bsi->bsi_op->o_tmpmemctx,
					"cbbc",
					'(', /* ) */
					&at->bam_sel_expr_u, 
					&ordering,
					'\'' );

			start = bsi->bsi_flt_where.bb_val.bv_len;

			backsql_strfcat_x( &bsi->bsi_flt_where,
					bsi->bsi_op->o_tmpmemctx,
					"bl",
					filter_value, 
					(ber_len_t)STRLENOF( /* (' */ "')" ),
						/* (' */ "')" );

			ldap_pvt_str2upper( &bsi->bsi_flt_where.bb_val.bv_val[ start ] );
		
		} else {
			backsql_strfcat_x( &bsi->bsi_flt_where,
					bsi->bsi_op->o_tmpmemctx,
					"cbbcbl",
					'(' /* ) */ ,
					&at->bam_sel_expr,
					&ordering,
					'\'',
					&f->f_av_value,
					(ber_len_t)STRLENOF( /* (' */ "')" ),
						/* ( */ "')" );
		}
		break;

	case LDAP_FILTER_PRESENT:
		backsql_strfcat_x( &bsi->bsi_flt_where,
				bsi->bsi_op->o_tmpmemctx,
				"lbl",
				(ber_len_t)STRLENOF( "NOT (" /* ) */),
					"NOT (", /* ) */
				&at->bam_sel_expr, 
				(ber_len_t)STRLENOF( /* ( */ " IS NULL)" ),
					/* ( */ " IS NULL)" );
		break;

	case LDAP_FILTER_SUBSTRINGS:
		backsql_process_sub_filter( bsi, f, at );
		break;

	case LDAP_FILTER_APPROX:
		/* we do our best */

		/*
		 * maybe we should check type of at->sel_expr here somehow,
		 * to know whether upper_func is applicable, but for now
		 * upper_func stuff is made for Oracle, where UPPER is
		 * safely applicable to NUMBER etc.
		 */
		(void)backsql_process_filter_like( bsi, at, 1, &f->f_av_value );
		break;

	default:
		/* unhandled filter type; should not happen */
		assert( 0 );
		backsql_strfcat_x( &bsi->bsi_flt_where,
				bsi->bsi_op->o_tmpmemctx,
				"l",
				(ber_len_t)STRLENOF( "8=8" ), "8=8" );
		break;

	}

	Debug( LDAP_DEBUG_TRACE, "<==backsql_process_filter_attr(%s)\n",
		at->bam_ad->ad_cname.bv_val );

	return 1;
}