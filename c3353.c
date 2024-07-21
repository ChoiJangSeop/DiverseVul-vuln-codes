mbedtls_asn1_named_data *mbedtls_asn1_store_named_data( mbedtls_asn1_named_data **head,
                                        const char *oid, size_t oid_len,
                                        const unsigned char *val,
                                        size_t val_len )
{
    mbedtls_asn1_named_data *cur;

    if( ( cur = mbedtls_asn1_find_named_data( *head, oid, oid_len ) ) == NULL )
    {
        // Add new entry if not present yet based on OID
        //
        if( ( cur = mbedtls_calloc( 1, sizeof(mbedtls_asn1_named_data) ) ) == NULL )
            return( NULL );

        cur->oid.len = oid_len;
        cur->oid.p = mbedtls_calloc( 1, oid_len );
        if( cur->oid.p == NULL )
        {
            mbedtls_free( cur );
            return( NULL );
        }

        memcpy( cur->oid.p, oid, oid_len );

        cur->val.len = val_len;
        cur->val.p = mbedtls_calloc( 1, val_len );
        if( cur->val.p == NULL )
        {
            mbedtls_free( cur->oid.p );
            mbedtls_free( cur );
            return( NULL );
        }

        cur->next = *head;
        *head = cur;
    }
    else if( cur->val.len < val_len )
    {
        // Enlarge existing value buffer if needed
        //
        mbedtls_free( cur->val.p );
        cur->val.p = NULL;

        cur->val.len = val_len;
        cur->val.p = mbedtls_calloc( 1, val_len );
        if( cur->val.p == NULL )
        {
            mbedtls_free( cur->oid.p );
            mbedtls_free( cur );
            return( NULL );
        }
    }

    if( val != NULL )
        memcpy( cur->val.p, val, val_len );

    return( cur );
}