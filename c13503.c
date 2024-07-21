backsql_process_sub_filter( backsql_srch_info *bsi, Filter *f,
	backsql_at_map_rec *at )
{
	backsql_info		*bi = (backsql_info *)bsi->bsi_op->o_bd->be_private;
	int			i;
	int			casefold = 0;

	if ( !f ) {
		return 0;
	}

	/* always uppercase strings by now */
#ifdef BACKSQL_UPPERCASE_FILTER
	if ( f->f_sub_desc->ad_type->sat_substr &&
			SLAP_MR_ASSOCIATED( f->f_sub_desc->ad_type->sat_substr,
				bi->sql_caseIgnoreMatch ) )
#endif /* BACKSQL_UPPERCASE_FILTER */
	{
		casefold = 1;
	}

	if ( f->f_sub_desc->ad_type->sat_substr &&
			SLAP_MR_ASSOCIATED( f->f_sub_desc->ad_type->sat_substr,
				bi->sql_telephoneNumberMatch ) )
	{

		struct berval	bv;
		ber_len_t	i, s, a;

		/*
		 * to check for matching telephone numbers
		 * with intermixed chars, e.g. val='1234'
		 * use
		 * 
		 * val LIKE '%1%2%3%4%'
		 */

		BER_BVZERO( &bv );
		if ( f->f_sub_initial.bv_val ) {
			bv.bv_len += f->f_sub_initial.bv_len;
		}
		if ( f->f_sub_any != NULL ) {
			for ( a = 0; f->f_sub_any[ a ].bv_val != NULL; a++ ) {
				bv.bv_len += f->f_sub_any[ a ].bv_len;
			}
		}
		if ( f->f_sub_final.bv_val ) {
			bv.bv_len += f->f_sub_final.bv_len;
		}
		bv.bv_len = 2 * bv.bv_len - 1;
		bv.bv_val = ch_malloc( bv.bv_len + 1 );

		s = 0;
		if ( !BER_BVISNULL( &f->f_sub_initial ) ) {
			bv.bv_val[ s ] = f->f_sub_initial.bv_val[ 0 ];
			for ( i = 1; i < f->f_sub_initial.bv_len; i++ ) {
				bv.bv_val[ s + 2 * i - 1 ] = '%';
				bv.bv_val[ s + 2 * i ] = f->f_sub_initial.bv_val[ i ];
			}
			bv.bv_val[ s + 2 * i - 1 ] = '%';
			s += 2 * i;
		}

		if ( f->f_sub_any != NULL ) {
			for ( a = 0; !BER_BVISNULL( &f->f_sub_any[ a ] ); a++ ) {
				bv.bv_val[ s ] = f->f_sub_any[ a ].bv_val[ 0 ];
				for ( i = 1; i < f->f_sub_any[ a ].bv_len; i++ ) {
					bv.bv_val[ s + 2 * i - 1 ] = '%';
					bv.bv_val[ s + 2 * i ] = f->f_sub_any[ a ].bv_val[ i ];
				}
				bv.bv_val[ s + 2 * i - 1 ] = '%';
				s += 2 * i;
			}
		}

		if ( !BER_BVISNULL( &f->f_sub_final ) ) {
			bv.bv_val[ s ] = f->f_sub_final.bv_val[ 0 ];
			for ( i = 1; i < f->f_sub_final.bv_len; i++ ) {
				bv.bv_val[ s + 2 * i - 1 ] = '%';
				bv.bv_val[ s + 2 * i ] = f->f_sub_final.bv_val[ i ];
			}
				bv.bv_val[ s + 2 * i - 1 ] = '%';
			s += 2 * i;
		}

		bv.bv_val[ s - 1 ] = '\0';

		(void)backsql_process_filter_like( bsi, at, casefold, &bv );
		ch_free( bv.bv_val );

		return 1;
	}

	/*
	 * When dealing with case-sensitive strings 
	 * we may omit normalization; however, normalized
	 * SQL filters are more liberal.
	 */

	backsql_strfcat_x( &bsi->bsi_flt_where,
			bsi->bsi_op->o_tmpmemctx, "c", '(' /* ) */  );

	/* TimesTen */
	Debug( LDAP_DEBUG_TRACE, "backsql_process_sub_filter(%s):\n",
		at->bam_ad->ad_cname.bv_val );
	Debug(LDAP_DEBUG_TRACE, "   expr: '%s%s%s'\n", at->bam_sel_expr.bv_val,
		at->bam_sel_expr_u.bv_val ? "' '" : "",
		at->bam_sel_expr_u.bv_val ? at->bam_sel_expr_u.bv_val : "" );
	if ( casefold && BACKSQL_AT_CANUPPERCASE( at ) ) {
		/*
		 * If a pre-upper-cased version of the column 
		 * or a precompiled upper function exists, use it
		 */
		backsql_strfcat_x( &bsi->bsi_flt_where, 
				bsi->bsi_op->o_tmpmemctx,
				"bl",
				&at->bam_sel_expr_u,
				(ber_len_t)STRLENOF( " LIKE '" ),
					" LIKE '" );

	} else {
		backsql_strfcat_x( &bsi->bsi_flt_where,
				bsi->bsi_op->o_tmpmemctx,
				"bl",
				&at->bam_sel_expr,
				(ber_len_t)STRLENOF( " LIKE '" ), " LIKE '" );
	}
 
	if ( !BER_BVISNULL( &f->f_sub_initial ) ) {
		ber_len_t	start;

#ifdef BACKSQL_TRACE
		Debug( LDAP_DEBUG_TRACE, 
			"==>backsql_process_sub_filter(%s): "
			"sub_initial=\"%s\"\n", at->bam_ad->ad_cname.bv_val,
			f->f_sub_initial.bv_val );
#endif /* BACKSQL_TRACE */

		start = bsi->bsi_flt_where.bb_val.bv_len;
		backsql_strfcat_x( &bsi->bsi_flt_where,
				bsi->bsi_op->o_tmpmemctx,
				"b",
				&f->f_sub_initial );
		if ( casefold && BACKSQL_AT_CANUPPERCASE( at ) ) {
			ldap_pvt_str2upper( &bsi->bsi_flt_where.bb_val.bv_val[ start ] );
		}
	}

	backsql_strfcat_x( &bsi->bsi_flt_where,
			bsi->bsi_op->o_tmpmemctx,
			"c", '%' );

	if ( f->f_sub_any != NULL ) {
		for ( i = 0; !BER_BVISNULL( &f->f_sub_any[ i ] ); i++ ) {
			ber_len_t	start;

#ifdef BACKSQL_TRACE
			Debug( LDAP_DEBUG_TRACE, 
				"==>backsql_process_sub_filter(%s): "
				"sub_any[%d]=\"%s\"\n", at->bam_ad->ad_cname.bv_val, 
				i, f->f_sub_any[ i ].bv_val );
#endif /* BACKSQL_TRACE */

			start = bsi->bsi_flt_where.bb_val.bv_len;
			backsql_strfcat_x( &bsi->bsi_flt_where,
					bsi->bsi_op->o_tmpmemctx,
					"bc",
					&f->f_sub_any[ i ],
					'%' );
			if ( casefold && BACKSQL_AT_CANUPPERCASE( at ) ) {
				/*
				 * Note: toupper('%') = '%'
				 */
				ldap_pvt_str2upper( &bsi->bsi_flt_where.bb_val.bv_val[ start ] );
			}
		}
	}

	if ( !BER_BVISNULL( &f->f_sub_final ) ) {
		ber_len_t	start;

#ifdef BACKSQL_TRACE
		Debug( LDAP_DEBUG_TRACE, 
			"==>backsql_process_sub_filter(%s): "
			"sub_final=\"%s\"\n", at->bam_ad->ad_cname.bv_val,
			f->f_sub_final.bv_val );
#endif /* BACKSQL_TRACE */

		start = bsi->bsi_flt_where.bb_val.bv_len;
    		backsql_strfcat_x( &bsi->bsi_flt_where,
				bsi->bsi_op->o_tmpmemctx,
				"b",
				&f->f_sub_final );
  		if ( casefold && BACKSQL_AT_CANUPPERCASE( at ) ) {
			ldap_pvt_str2upper( &bsi->bsi_flt_where.bb_val.bv_val[ start ] );
		}
	}

	backsql_strfcat_x( &bsi->bsi_flt_where,
			bsi->bsi_op->o_tmpmemctx,
			"l", 
			(ber_len_t)STRLENOF( /* (' */ "')" ), /* (' */ "')" );
 
	return 1;
}