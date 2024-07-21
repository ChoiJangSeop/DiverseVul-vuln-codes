static void ProcessRadioRxDone( void )
{
    LoRaMacHeader_t macHdr;
    ApplyCFListParams_t applyCFList;
    GetPhyParams_t getPhy;
    PhyParam_t phyParam;
    LoRaMacCryptoStatus_t macCryptoStatus = LORAMAC_CRYPTO_ERROR;

    LoRaMacMessageData_t macMsgData;
    LoRaMacMessageJoinAccept_t macMsgJoinAccept;
    uint8_t *payload = RxDoneParams.Payload;
    uint16_t size = RxDoneParams.Size;
    int16_t rssi = RxDoneParams.Rssi;
    int8_t snr = RxDoneParams.Snr;

    uint8_t pktHeaderLen = 0;

    uint32_t downLinkCounter = 0;
    uint32_t address = MacCtx.NvmCtx->DevAddr;
    uint8_t multicast = 0;
    AddressIdentifier_t addrID = UNICAST_DEV_ADDR;
    FCntIdentifier_t fCntID;

    MacCtx.McpsConfirm.AckReceived = false;
    MacCtx.McpsIndication.Rssi = rssi;
    MacCtx.McpsIndication.Snr = snr;
    MacCtx.McpsIndication.RxSlot = MacCtx.RxSlot;
    MacCtx.McpsIndication.Port = 0;
    MacCtx.McpsIndication.Multicast = 0;
    MacCtx.McpsIndication.FramePending = 0;
    MacCtx.McpsIndication.Buffer = NULL;
    MacCtx.McpsIndication.BufferSize = 0;
    MacCtx.McpsIndication.RxData = false;
    MacCtx.McpsIndication.AckReceived = false;
    MacCtx.McpsIndication.DownLinkCounter = 0;
    MacCtx.McpsIndication.McpsIndication = MCPS_UNCONFIRMED;
    MacCtx.McpsIndication.DevAddress = 0;
    MacCtx.McpsIndication.DeviceTimeAnsReceived = false;

    Radio.Sleep( );
    TimerStop( &MacCtx.RxWindowTimer2 );

    // This function must be called even if we are not in class b mode yet.
    if( LoRaMacClassBRxBeacon( payload, size ) == true )
    {
        MacCtx.MlmeIndication.BeaconInfo.Rssi = rssi;
        MacCtx.MlmeIndication.BeaconInfo.Snr = snr;
        return;
    }
    // Check if we expect a ping or a multicast slot.
    if( MacCtx.NvmCtx->DeviceClass == CLASS_B )
    {
        if( LoRaMacClassBIsPingExpected( ) == true )
        {
            LoRaMacClassBSetPingSlotState( PINGSLOT_STATE_CALC_PING_OFFSET );
            LoRaMacClassBPingSlotTimerEvent( NULL );
            MacCtx.McpsIndication.RxSlot = RX_SLOT_WIN_CLASS_B_PING_SLOT;
        }
        else if( LoRaMacClassBIsMulticastExpected( ) == true )
        {
            LoRaMacClassBSetMulticastSlotState( PINGSLOT_STATE_CALC_PING_OFFSET );
            LoRaMacClassBMulticastSlotTimerEvent( NULL );
            MacCtx.McpsIndication.RxSlot = RX_SLOT_WIN_CLASS_B_MULTICAST_SLOT;
        }
    }

    macHdr.Value = payload[pktHeaderLen++];

    switch( macHdr.Bits.MType )
    {
        case FRAME_TYPE_JOIN_ACCEPT:
            macMsgJoinAccept.Buffer = payload;
            macMsgJoinAccept.BufSize = size;

            // Abort in case if the device isn't joined yet and no rejoin request is ongoing.
            if( MacCtx.NvmCtx->NetworkActivation != ACTIVATION_TYPE_NONE )
            {
                MacCtx.McpsIndication.Status = LORAMAC_EVENT_INFO_STATUS_ERROR;
                PrepareRxDoneAbort( );
                return;
            }
            macCryptoStatus = LoRaMacCryptoHandleJoinAccept( JOIN_REQ, SecureElementGetJoinEui( ), &macMsgJoinAccept );

            if( LORAMAC_CRYPTO_SUCCESS == macCryptoStatus )
            {
                // Network ID
                MacCtx.NvmCtx->NetID = ( uint32_t ) macMsgJoinAccept.NetID[0];
                MacCtx.NvmCtx->NetID |= ( ( uint32_t ) macMsgJoinAccept.NetID[1] << 8 );
                MacCtx.NvmCtx->NetID |= ( ( uint32_t ) macMsgJoinAccept.NetID[2] << 16 );

                // Device Address
                MacCtx.NvmCtx->DevAddr = macMsgJoinAccept.DevAddr;

                // DLSettings
                MacCtx.NvmCtx->MacParams.Rx1DrOffset = macMsgJoinAccept.DLSettings.Bits.RX1DRoffset;
                MacCtx.NvmCtx->MacParams.Rx2Channel.Datarate = macMsgJoinAccept.DLSettings.Bits.RX2DataRate;
                MacCtx.NvmCtx->MacParams.RxCChannel.Datarate = macMsgJoinAccept.DLSettings.Bits.RX2DataRate;

                // RxDelay
                MacCtx.NvmCtx->MacParams.ReceiveDelay1 = macMsgJoinAccept.RxDelay;
                if( MacCtx.NvmCtx->MacParams.ReceiveDelay1 == 0 )
                {
                    MacCtx.NvmCtx->MacParams.ReceiveDelay1 = 1;
                }
                MacCtx.NvmCtx->MacParams.ReceiveDelay1 *= 1000;
                MacCtx.NvmCtx->MacParams.ReceiveDelay2 = MacCtx.NvmCtx->MacParams.ReceiveDelay1 + 1000;

                MacCtx.NvmCtx->Version.Fields.Minor = 0;

                // Apply CF list
                applyCFList.Payload = macMsgJoinAccept.CFList;
                // Size of the regular payload is 12. Plus 1 byte MHDR and 4 bytes MIC
                applyCFList.Size = size - 17;

                RegionApplyCFList( MacCtx.NvmCtx->Region, &applyCFList );

                MacCtx.NvmCtx->NetworkActivation = ACTIVATION_TYPE_OTAA;

                // MLME handling
                if( LoRaMacConfirmQueueIsCmdActive( MLME_JOIN ) == true )
                {
                    LoRaMacConfirmQueueSetStatus( LORAMAC_EVENT_INFO_STATUS_OK, MLME_JOIN );
                }
            }
            else
            {
                // MLME handling
                if( LoRaMacConfirmQueueIsCmdActive( MLME_JOIN ) == true )
                {
                    LoRaMacConfirmQueueSetStatus( LORAMAC_EVENT_INFO_STATUS_JOIN_FAIL, MLME_JOIN );
                }
            }
            break;
        case FRAME_TYPE_DATA_CONFIRMED_DOWN:
            MacCtx.McpsIndication.McpsIndication = MCPS_CONFIRMED;
            // Intentional fall through
        case FRAME_TYPE_DATA_UNCONFIRMED_DOWN:
            // Check if the received payload size is valid
            getPhy.UplinkDwellTime = MacCtx.NvmCtx->MacParams.DownlinkDwellTime;
            getPhy.Datarate = MacCtx.McpsIndication.RxDatarate;
            getPhy.Attribute = PHY_MAX_PAYLOAD;
            phyParam = RegionGetPhyParam( MacCtx.NvmCtx->Region, &getPhy );
            if( MAX( 0, ( int16_t )( ( int16_t ) size - ( int16_t ) LORA_MAC_FRMPAYLOAD_OVERHEAD ) ) > ( int16_t )phyParam.Value )
            {
                MacCtx.McpsIndication.Status = LORAMAC_EVENT_INFO_STATUS_ERROR;
                PrepareRxDoneAbort( );
                return;
            }
            macMsgData.Buffer = payload;
            macMsgData.BufSize = size;
            macMsgData.FRMPayload = MacCtx.RxPayload;
            macMsgData.FRMPayloadSize = LORAMAC_PHY_MAXPAYLOAD;

            if( LORAMAC_PARSER_SUCCESS != LoRaMacParserData( &macMsgData ) )
            {
                MacCtx.McpsIndication.Status = LORAMAC_EVENT_INFO_STATUS_ERROR;
                PrepareRxDoneAbort( );
                return;
            }

            // Store device address
            MacCtx.McpsIndication.DevAddress = macMsgData.FHDR.DevAddr;

            FType_t fType;
            if( LORAMAC_STATUS_OK != DetermineFrameType( &macMsgData, &fType ) )
            {
                MacCtx.McpsIndication.Status = LORAMAC_EVENT_INFO_STATUS_ERROR;
                PrepareRxDoneAbort( );
                return;
            }

            //Check if it is a multicast message
            multicast = 0;
            downLinkCounter = 0;
            for( uint8_t i = 0; i < LORAMAC_MAX_MC_CTX; i++ )
            {
                if( ( MacCtx.NvmCtx->MulticastChannelList[i].ChannelParams.Address == macMsgData.FHDR.DevAddr ) &&
                    ( MacCtx.NvmCtx->MulticastChannelList[i].ChannelParams.IsEnabled == true ) )
                {
                    multicast = 1;
                    addrID = MacCtx.NvmCtx->MulticastChannelList[i].ChannelParams.GroupID;
                    downLinkCounter = *( MacCtx.NvmCtx->MulticastChannelList[i].DownLinkCounter );
                    address = MacCtx.NvmCtx->MulticastChannelList[i].ChannelParams.Address;
                    if( MacCtx.NvmCtx->DeviceClass == CLASS_C )
                    {
                        MacCtx.McpsIndication.RxSlot = RX_SLOT_WIN_CLASS_C_MULTICAST;
                    }
                    break;
                }
            }

            // Filter messages according to multicast downlink exceptions
            if( ( multicast == 1 ) && ( ( fType != FRAME_TYPE_D ) ||
                                        ( macMsgData.FHDR.FCtrl.Bits.Ack == true ) ||
                                        ( macMsgData.FHDR.FCtrl.Bits.AdrAckReq == true ) ) )
            {
                MacCtx.McpsIndication.Status = LORAMAC_EVENT_INFO_STATUS_ERROR;
                PrepareRxDoneAbort( );
                return;
            }

            // Get maximum allowed counter difference
            getPhy.Attribute = PHY_MAX_FCNT_GAP;
            phyParam = RegionGetPhyParam( MacCtx.NvmCtx->Region, &getPhy );

            // Get downlink frame counter value
            macCryptoStatus = GetFCntDown( addrID, fType, &macMsgData, MacCtx.NvmCtx->Version, phyParam.Value, &fCntID, &downLinkCounter );
            if( macCryptoStatus != LORAMAC_CRYPTO_SUCCESS )
            {
                if( macCryptoStatus == LORAMAC_CRYPTO_FAIL_FCNT_DUPLICATED )
                {
                    // Catch the case of repeated downlink frame counter
                    MacCtx.McpsIndication.Status = LORAMAC_EVENT_INFO_STATUS_DOWNLINK_REPEATED;
                    if( ( MacCtx.NvmCtx->Version.Fields.Minor == 0 ) && ( macHdr.Bits.MType == FRAME_TYPE_DATA_CONFIRMED_DOWN ) && ( MacCtx.NvmCtx->LastRxMic == macMsgData.MIC ) )
                    {
                        MacCtx.NvmCtx->SrvAckRequested = true;
                    }
                }
                else if( macCryptoStatus == LORAMAC_CRYPTO_FAIL_MAX_GAP_FCNT )
                {
                    // Lost too many frames
                    MacCtx.McpsIndication.Status = LORAMAC_EVENT_INFO_STATUS_DOWNLINK_TOO_MANY_FRAMES_LOSS;
                }
                else
                {
                    // Other errors
                    MacCtx.McpsIndication.Status = LORAMAC_EVENT_INFO_STATUS_ERROR;
                }
                MacCtx.McpsIndication.DownLinkCounter = downLinkCounter;
                PrepareRxDoneAbort( );
                return;
            }

            macCryptoStatus = LoRaMacCryptoUnsecureMessage( addrID, address, fCntID, downLinkCounter, &macMsgData );
            if( macCryptoStatus != LORAMAC_CRYPTO_SUCCESS )
            {
                if( macCryptoStatus == LORAMAC_CRYPTO_FAIL_ADDRESS )
                {
                    // We are not the destination of this frame.
                    MacCtx.McpsIndication.Status = LORAMAC_EVENT_INFO_STATUS_ADDRESS_FAIL;
                }
                else
                {
                    // MIC calculation fail
                    MacCtx.McpsIndication.Status = LORAMAC_EVENT_INFO_STATUS_MIC_FAIL;
                }
                PrepareRxDoneAbort( );
                return;
            }

            // Frame is valid
            MacCtx.McpsIndication.Status = LORAMAC_EVENT_INFO_STATUS_OK;
            MacCtx.McpsIndication.Multicast = multicast;
            MacCtx.McpsIndication.FramePending = macMsgData.FHDR.FCtrl.Bits.FPending;
            MacCtx.McpsIndication.Buffer = NULL;
            MacCtx.McpsIndication.BufferSize = 0;
            MacCtx.McpsIndication.DownLinkCounter = downLinkCounter;
            MacCtx.McpsIndication.AckReceived = macMsgData.FHDR.FCtrl.Bits.Ack;

            MacCtx.McpsConfirm.Status = LORAMAC_EVENT_INFO_STATUS_OK;
            MacCtx.McpsConfirm.AckReceived = macMsgData.FHDR.FCtrl.Bits.Ack;

            // Reset ADR ACK Counter only, when RX1 or RX2 slot
            if( ( MacCtx.McpsIndication.RxSlot == RX_SLOT_WIN_1 ) ||
                ( MacCtx.McpsIndication.RxSlot == RX_SLOT_WIN_2 ) )
            {
                MacCtx.NvmCtx->AdrAckCounter = 0;
            }

            // MCPS Indication and ack requested handling
            if( multicast == 1 )
            {
                MacCtx.McpsIndication.McpsIndication = MCPS_MULTICAST;
            }
            else
            {
                if( macHdr.Bits.MType == FRAME_TYPE_DATA_CONFIRMED_DOWN )
                {
                    MacCtx.NvmCtx->SrvAckRequested = true;
                    if( MacCtx.NvmCtx->Version.Fields.Minor == 0 )
                    {
                        MacCtx.NvmCtx->LastRxMic = macMsgData.MIC;
                    }
                    MacCtx.McpsIndication.McpsIndication = MCPS_CONFIRMED;
                }
                else
                {
                    MacCtx.NvmCtx->SrvAckRequested = false;
                    MacCtx.McpsIndication.McpsIndication = MCPS_UNCONFIRMED;
                }
            }

            RemoveMacCommands( MacCtx.McpsIndication.RxSlot, macMsgData.FHDR.FCtrl, MacCtx.McpsConfirm.McpsRequest );

            switch( fType )
            {
                case FRAME_TYPE_A:
                {  /* +----------+------+-------+--------------+
                    * | FOptsLen | Fopt | FPort |  FRMPayload  |
                    * +----------+------+-------+--------------+
                    * |    > 0   |   X  |  > 0  |       X      |
                    * +----------+------+-------+--------------+
                    */

                    // Decode MAC commands in FOpts field
                    ProcessMacCommands( macMsgData.FHDR.FOpts, 0, macMsgData.FHDR.FCtrl.Bits.FOptsLen, snr, MacCtx.McpsIndication.RxSlot );
                    MacCtx.McpsIndication.Port = macMsgData.FPort;
                    MacCtx.McpsIndication.Buffer = macMsgData.FRMPayload;
                    MacCtx.McpsIndication.BufferSize = macMsgData.FRMPayloadSize;
                    MacCtx.McpsIndication.RxData = true;
                    break;
                }
                case FRAME_TYPE_B:
                {  /* +----------+------+-------+--------------+
                    * | FOptsLen | Fopt | FPort |  FRMPayload  |
                    * +----------+------+-------+--------------+
                    * |    > 0   |   X  |   -   |       -      |
                    * +----------+------+-------+--------------+
                    */

                    // Decode MAC commands in FOpts field
                    ProcessMacCommands( macMsgData.FHDR.FOpts, 0, macMsgData.FHDR.FCtrl.Bits.FOptsLen, snr, MacCtx.McpsIndication.RxSlot );
                    MacCtx.McpsIndication.Port = macMsgData.FPort;
                    break;
                }
                case FRAME_TYPE_C:
                {  /* +----------+------+-------+--------------+
                    * | FOptsLen | Fopt | FPort |  FRMPayload  |
                    * +----------+------+-------+--------------+
                    * |    = 0   |   -  |  = 0  | MAC commands |
                    * +----------+------+-------+--------------+
                    */

                    // Decode MAC commands in FRMPayload
                    ProcessMacCommands( macMsgData.FRMPayload, 0, macMsgData.FRMPayloadSize, snr, MacCtx.McpsIndication.RxSlot );
                    MacCtx.McpsIndication.Port = macMsgData.FPort;
                    break;
                }
                case FRAME_TYPE_D:
                {  /* +----------+------+-------+--------------+
                    * | FOptsLen | Fopt | FPort |  FRMPayload  |
                    * +----------+------+-------+--------------+
                    * |    = 0   |   -  |  > 0  |       X      |
                    * +----------+------+-------+--------------+
                    */

                    // No MAC commands just application payload
                    MacCtx.McpsIndication.Port = macMsgData.FPort;
                    MacCtx.McpsIndication.Buffer = macMsgData.FRMPayload;
                    MacCtx.McpsIndication.BufferSize = macMsgData.FRMPayloadSize;
                    MacCtx.McpsIndication.RxData = true;
                    break;
                }
                default:
                    MacCtx.McpsIndication.Status = LORAMAC_EVENT_INFO_STATUS_ERROR;
                    PrepareRxDoneAbort( );
                    break;
            }

            // Provide always an indication, skip the callback to the user application,
            // in case of a confirmed downlink retransmission.
            MacCtx.MacFlags.Bits.McpsInd = 1;

            break;
        case FRAME_TYPE_PROPRIETARY:
            memcpy1( MacCtx.RxPayload, &payload[pktHeaderLen], size - pktHeaderLen );

            MacCtx.McpsIndication.McpsIndication = MCPS_PROPRIETARY;
            MacCtx.McpsIndication.Status = LORAMAC_EVENT_INFO_STATUS_OK;
            MacCtx.McpsIndication.Buffer = MacCtx.RxPayload;
            MacCtx.McpsIndication.BufferSize = size - pktHeaderLen;

            MacCtx.MacFlags.Bits.McpsInd = 1;
            break;
        default:
            MacCtx.McpsIndication.Status = LORAMAC_EVENT_INFO_STATUS_ERROR;
            PrepareRxDoneAbort( );
            break;
    }

    // Verify if we need to disable the AckTimeoutTimer
    if( MacCtx.NodeAckRequested == true )
    {
        if( MacCtx.McpsConfirm.AckReceived == true )
        {
            OnAckTimeoutTimerEvent( NULL );
        }
    }
    else
    {
        if( MacCtx.NvmCtx->DeviceClass == CLASS_C )
        {
            OnAckTimeoutTimerEvent( NULL );
        }
    }
    MacCtx.MacFlags.Bits.MacDone = 1;

    UpdateRxSlotIdleState( );
}