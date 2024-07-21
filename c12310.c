SecureElementStatus_t SecureElementProcessJoinAccept( JoinReqIdentifier_t joinReqType, uint8_t* joinEui,
                                                      uint16_t devNonce, uint8_t* encJoinAccept,
                                                      uint8_t encJoinAcceptSize, uint8_t* decJoinAccept,
                                                      uint8_t* versionMinor )
{
    SecureElementStatus_t status = SECURE_ELEMENT_ERROR;

    if( ( encJoinAccept == NULL ) || ( decJoinAccept == NULL ) || ( versionMinor == NULL ) )
    {
        return SECURE_ELEMENT_ERROR_NPE;
    }

    // Determine decryption key
    KeyIdentifier_t encKeyID = NWK_KEY;

    if( joinReqType != JOIN_REQ )
    {
        encKeyID = J_S_ENC_KEY;
    }

    //  - Header buffer to be used for MIC computation
    //        - LoRaWAN 1.0.x : micHeader = [MHDR(1)]
    //        - LoRaWAN 1.1.x : micHeader = [JoinReqType(1), JoinEUI(8), DevNonce(2), MHDR(1)]

    // Try first to process LoRaWAN 1.0.x JoinAccept
    uint8_t micHeader10[1] = { 0x20 };

    //   cmac = aes128_cmac(NwkKey, MHDR |  JoinNonce | NetID | DevAddr | DLSettings | RxDelay | CFList |
    //   CFListType)
    lr1110_crypto_process_join_accept(
        &LR1110, ( lr1110_crypto_status_t* ) &status, convert_key_id_from_se_to_lr1110( encKeyID ),
        convert_key_id_from_se_to_lr1110( NWK_KEY ), ( lr1110_crypto_lorawan_version_t ) 0, micHeader10,
        encJoinAccept + 1, encJoinAcceptSize - 1, decJoinAccept + 1 );

    if( status == SECURE_ELEMENT_SUCCESS )
    {
        *versionMinor = ( ( decJoinAccept[11] & 0x80 ) == 0x80 ) ? 1 : 0;
        if( *versionMinor == 0 )
        {
            // Network server is operating according to LoRaWAN 1.0.x
            return SECURE_ELEMENT_SUCCESS;
        }
    }

#if( USE_LRWAN_1_1_X_CRYPTO == 1 )
    // 1.0.x trial failed. Trying to process LoRaWAN 1.1.x JoinAccept
    uint8_t  micHeader11[JOIN_ACCEPT_MIC_COMPUTATION_OFFSET] = { 0 };
    uint16_t bufItr                                     = 0;

    //   cmac = aes128_cmac(JSIntKey, JoinReqType | JoinEUI | DevNonce | MHDR | JoinNonce | NetID | DevAddr |
    //   DLSettings | RxDelay | CFList | CFListType)
    micHeader11[bufItr++] = ( uint8_t ) joinReqType;

    memcpyr( micHeader11 + bufItr, joinEui, LORAMAC_JOIN_EUI_FIELD_SIZE );
    bufItr += LORAMAC_JOIN_EUI_FIELD_SIZE;

    micHeader11[bufItr++] = devNonce & 0xFF;
    micHeader11[bufItr++] = ( devNonce >> 8 ) & 0xFF;

    micHeader11[bufItr++] = 0x20;

    lr1110_crypto_process_join_accept(
        &LR1110, ( lr1110_crypto_status_t* ) &status, convert_key_id_from_se_to_lr1110( encKeyID ),
        convert_key_id_from_se_to_lr1110( J_S_INT_KEY ), ( lr1110_crypto_lorawan_version_t ) 1, micHeader11,
        encJoinAccept + 1, encJoinAcceptSize - 1, decJoinAccept + 1 );

    if( status == SECURE_ELEMENT_SUCCESS )
    {
        *versionMinor = ( ( decJoinAccept[11] & 0x80 ) == 0x80 ) ? 1 : 0;
        if( *versionMinor == 1 )
        {
            // Network server is operating according to LoRaWAN 1.1.x
            return SECURE_ELEMENT_SUCCESS;
        }
    }
#endif

    return status;
}