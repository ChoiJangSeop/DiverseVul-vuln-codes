static void nextstate(struct gps_packet_t *lexer, unsigned char c)
{
    static int n = 0;
#ifdef RTCM104V2_ENABLE
    enum isgpsstat_t isgpsstat;
#endif /* RTCM104V2_ENABLE */
#ifdef SUPERSTAR2_ENABLE
    static unsigned char ctmp;
#endif /* SUPERSTAR2_ENABLE */
/*@ +charint -casebreak @*/
    n++;
    switch (lexer->state) {
    case GROUND_STATE:
	n = 0;
	if (c == '#') {
	    lexer->state = COMMENT_BODY;
	    break;
	}
#ifdef NMEA_ENABLE
	if (c == '$') {
	    lexer->state = NMEA_DOLLAR;
	    break;
	}
	if (c == '!') {
	    lexer->state = NMEA_BANG;
	    break;
	}
#endif /* NMEA_ENABLE */
#if defined(TNT_ENABLE) || defined(GARMINTXT_ENABLE) || defined(ONCORE_ENABLE)
	if (c == '@') {
	    lexer->state = AT1_LEADER;
	    break;
	}
#endif
#ifdef SIRF_ENABLE
	if (c == 0xa0) {
	    lexer->state = SIRF_LEADER_1;
	    break;
	}
#endif /* SIRF_ENABLE */
#ifdef SUPERSTAR2_ENABLE
	if (c == SOH) {
	    lexer->state = SUPERSTAR2_LEADER;
	    break;
	}
#endif /* SUPERSTAR2_ENABLE */
#if defined(TSIP_ENABLE) || defined(EVERMORE_ENABLE) || defined(GARMIN_ENABLE)
	if (c == DLE) {
	    lexer->state = DLE_LEADER;
	    break;
	}
#endif /* TSIP_ENABLE || EVERMORE_ENABLE || GARMIN_ENABLE */
#ifdef TRIPMATE_ENABLE
	if (c == 'A') {
#ifdef RTCM104V2_ENABLE
	    if (rtcm2_decode(lexer, c) == ISGPS_MESSAGE) {
		lexer->state = RTCM2_RECOGNIZED;
		break;
	    }
#endif /* RTCM104V2_ENABLE */
	    lexer->state = ASTRAL_1;
	    break;
	}
#endif /* TRIPMATE_ENABLE */
#ifdef EARTHMATE_ENABLE
	if (c == 'E') {
#ifdef RTCM104V2_ENABLE
	    if (rtcm2_decode(lexer, c) == ISGPS_MESSAGE) {
		lexer->state = RTCM2_RECOGNIZED;
		break;
	    }
#endif /* RTCM104V2_ENABLE */
	    lexer->state = EARTHA_1;
	    break;
	}
#endif /* EARTHMATE_ENABLE */
#ifdef ZODIAC_ENABLE
	if (c == 0xff) {
	    lexer->state = ZODIAC_LEADER_1;
	    break;
	}
#endif /* ZODIAC_ENABLE */
#ifdef UBX_ENABLE
	if (c == 0xb5) {
	    lexer->state = UBX_LEADER_1;
	    break;
	}
#endif /* UBX_ENABLE */
#ifdef ITRAX_ENABLE
	if (c == '<') {
	    lexer->state = ITALK_LEADER_1;
	    break;
	}
#endif /* ITRAX_ENABLE */
#ifdef NAVCOM_ENABLE
	if (c == 0x02) {
	    lexer->state = NAVCOM_LEADER_1;
	    break;
	}
#endif /* NAVCOM_ENABLE */
#ifdef GEOSTAR_ENABLE
	if (c == 'P') {
	    lexer->state = GEOSTAR_LEADER_1;
	    break;
	}
#endif /* GEOSTAR_ENABLE */
#ifdef RTCM104V2_ENABLE
	if ((isgpsstat = rtcm2_decode(lexer, c)) == ISGPS_SYNC) {
	    lexer->state = RTCM2_SYNC_STATE;
	    break;
	} else if (isgpsstat == ISGPS_MESSAGE) {
	    lexer->state = RTCM2_RECOGNIZED;
	    break;
	}
#endif /* RTCM104V2_ENABLE */
#ifdef RTCM104V3_ENABLE
	if (c == 0xD3) {
	    lexer->state = RTCM3_LEADER_1;
	    break;
	}
#endif /* RTCM104V3_ENABLE */
#ifdef PASSTHROUGH_ENABLE
	if (c == '{') {
	    lexer->state = JSON_LEADER;
	    character_pushback(lexer);
	}
#endif /* PASSTHROUGH_ENABLE */
	break;
    case COMMENT_BODY:
	if (c == '\n')
	    lexer->state = COMMENT_RECOGNIZED;
	else if (!isprint(c)) {
	    lexer->state = GROUND_STATE;
	    character_pushback(lexer);
	}
	break;
#ifdef NMEA_ENABLE
    case NMEA_DOLLAR:
	if (c == 'G')
	    lexer->state = NMEA_PUB_LEAD;
	else if (c == 'P')	/* vendor sentence */
	    lexer->state = NMEA_VENDOR_LEAD;
	else if (c == 'I')	/* Seatalk */
	    lexer->state = SEATALK_LEAD_1;
	else if (c == 'W')	/* Weather instrument */
	    lexer->state = WEATHER_LEAD_1;
	else if (c == 'H')	/* Heading/compass */
	    lexer->state = HEADCOMP_LEAD_1;
	else if (c == 'T')	/* Turn indicator */
	    lexer->state = TURN_LEAD_1;
	else if (c == 'A')	/* SiRF Ack */
	    lexer->state = SIRF_ACK_LEAD_1;
	else if (c == 'E')	/* ECDIS */
	    lexer->state = ECDIS_LEAD_1;
	else if (c == 'S')
	    lexer->state = SOUNDER_LEAD_1;
#ifdef OCEANSERVER_ENABLE
	else if (c == 'C')
	    lexer->state = NMEA_LEADER_END;
#endif /* OCEANSERVER_ENABLE */
	else
	    lexer->state = GROUND_STATE;
	break;
    case NMEA_PUB_LEAD:
	/*
	 * $GP == GPS, $GL = GLONASS only, $GN = mixed GPS and GLONASS,
	 * according to NMEA (IEIC 61162-1) DRAFT 02/06/2009.
	 */
	if (c == 'P' || c == 'N' || c == 'L')
	    lexer->state = NMEA_LEADER_END;
	else
	    lexer->state = GROUND_STATE;
	break;
    case NMEA_VENDOR_LEAD:
	if (c == 'A')
	    lexer->state = NMEA_PASHR_A;
	else if (isalpha(c))
	    lexer->state = NMEA_LEADER_END;
	else
	    lexer->state = GROUND_STATE;
	break;
    /*
     * Without the following six states, DLE in a $PASHR can fool the
     * sniffer into thinking it sees a TSIP packet.  Hilarity ensues.
     */
    case NMEA_PASHR_A:
	if (c == 'S')
	    lexer->state = NMEA_PASHR_S;
	else if (isalpha(c))
	    lexer->state = NMEA_LEADER_END;
	else
	    lexer->state = GROUND_STATE;
	break;
    case NMEA_PASHR_S:
	if (c == 'H')
	    lexer->state = NMEA_PASHR_H;
	else if (isalpha(c))
	    lexer->state = NMEA_LEADER_END;
	else
	    lexer->state = GROUND_STATE;
	break;
    case NMEA_PASHR_H:
	if (c == 'R')
	    lexer->state = NMEA_BINARY_BODY;
	else if (isalpha(c))
	    lexer->state = NMEA_LEADER_END;
	else
	    lexer->state = GROUND_STATE;
	break;
    case NMEA_BINARY_BODY:
	if (c == '\r')
	    lexer->state = NMEA_BINARY_CR;
	break;
    case NMEA_BINARY_CR:
	if (c == '\n')
	    lexer->state = NMEA_BINARY_NL;
	else
	    lexer->state = NMEA_BINARY_BODY;
	break;
    case NMEA_BINARY_NL:
	if (c == '$') {
	    character_pushback(lexer);
	    lexer->state = NMEA_RECOGNIZED;
	} else
	    lexer->state = NMEA_BINARY_BODY;
	break;
    case NMEA_BANG:
	if (c == 'A')
	    lexer->state = AIS_LEAD_1;
	else if (c == 'B')
	    lexer->state = AIS_LEAD_ALT1;
	else
	    lexer->state = GROUND_STATE;
	break;
    case AIS_LEAD_1:
	if (c == 'I')
	    lexer->state = AIS_LEAD_2;
	else
	    lexer->state = GROUND_STATE;
	break;
    case AIS_LEAD_2:
	if (isalpha(c))
	    lexer->state = NMEA_LEADER_END;
	else
	    lexer->state = GROUND_STATE;
	break;
    case AIS_LEAD_ALT1:
	if (c == 'S')
	    lexer->state = AIS_LEAD_ALT2;
	else
	    lexer->state = GROUND_STATE;
	break;
    case AIS_LEAD_ALT2:
	if (isalpha(c))
	    lexer->state = NMEA_LEADER_END;
	else
	    lexer->state = GROUND_STATE;
	break;
#if defined(TNT_ENABLE) || defined(GARMINTXT_ENABLE) || defined(ONCORE_ENABLE)
    case AT1_LEADER:
	switch (c) {
#ifdef ONCORE_ENABLE
	case '@':
	    lexer->state = ONCORE_AT2;
	    break;
#endif /* ONCORE_ENABLE */
#ifdef TNT_ENABLE
	case '*':
	    /*
	     * TNT has similar structure to NMEA packet, '*' before
	     * optional checksum ends the packet. Since '*' cannot be
	     * received from GARMIN working in TEXT mode, use this
	     * difference to tell that this is not GARMIN TEXT packet,
	     * could be TNT.
	     */
	    lexer->state = NMEA_LEADER_END;
	    break;
#endif /* TNT_ENABLE */
#if defined(GARMINTXT_ENABLE)
	case '\r':
	    /* stay in this state, next character should be '\n' */
	    /* in the theory we can stop search here and don't wait for '\n' */
	    lexer->state = AT1_LEADER;
	    break;
	case '\n':
	    /* end of packet found */
	    lexer->state = GTXT_RECOGNIZED;
	    break;
#endif /* GARMINTXT_ENABLE */
	default:
	    if (!isprint(c))
		lexer->state = GROUND_STATE;
	}
	break;
#endif /* defined(TNT_ENABLE) || defined(GARMINTXT_ENABLE) || defined(ONCORE_ENABLE) */
    case NMEA_LEADER_END:
	if (c == '\r')
	    lexer->state = NMEA_CR;
	else if (c == '\n')
	    /* not strictly correct, but helps for interpreting logfiles */
	    lexer->state = NMEA_RECOGNIZED;
	else if (c == '$'){
	    /* faster recovery from missing sentence trailers */
	    lexer->state = NMEA_DOLLAR;
	    lexer->inbufptr += (n-1);
	} else if (!isprint(c))
	    lexer->state = GROUND_STATE;
	break;
    case NMEA_CR:
	if (c == '\n')
	    lexer->state = NMEA_RECOGNIZED;
	/*
	 * There's a GPS called a Jackson Labs Firefly-1a that emits \r\r\n
	 * at the end of each sentence.  Don't be confused by this.
	 */
	else if (c == '\r')
	    lexer->state = NMEA_CR;
	else
	    lexer->state = GROUND_STATE;
	break;
    case NMEA_RECOGNIZED:
	if (c == '#')
	    lexer->state = COMMENT_BODY;
	else if (c == '$')
	    lexer->state = NMEA_DOLLAR;
	else if (c == '!')
	    lexer->state = NMEA_BANG;
#ifdef UBX_ENABLE
	else if (c == 0xb5)	/* LEA-5H can and will output NMEA and UBX back to back */
	    lexer->state = UBX_LEADER_1;
#endif
	else
	    lexer->state = GROUND_STATE;
	break;
    case SEATALK_LEAD_1:
	if (c == 'I' || c == 'N')	/* II or IN are accepted */
	    lexer->state = NMEA_LEADER_END;
	else
	    lexer->state = GROUND_STATE;
	break;
    case WEATHER_LEAD_1:
	if (c == 'I')		/* Weather instrument leader accepted */
	    lexer->state = NMEA_LEADER_END;
	else
	    lexer->state = GROUND_STATE;
	break;
    case HEADCOMP_LEAD_1:
	if (c == 'C')		/* Heading/compass leader accepted */
	    lexer->state = NMEA_LEADER_END;
	else
	    lexer->state = GROUND_STATE;
	break;
    case TURN_LEAD_1:
	if (c == 'I')		/* Turn indicator leader accepted */
	    lexer->state = NMEA_LEADER_END;
	else
	    lexer->state = GROUND_STATE;
	break;
    case ECDIS_LEAD_1:
	if (c == 'C')		/* ECDIS leader accepted */
	    lexer->state = NMEA_LEADER_END;
	else
	    lexer->state = GROUND_STATE;
	break;
    case SOUNDER_LEAD_1:
	if (c == 'D')		/* Depth-sounder leader accepted */
	    lexer->state = NMEA_LEADER_END;
	else
	    lexer->state = GROUND_STATE;
	break;
#ifdef TRIPMATE_ENABLE
    case ASTRAL_1:
	if (c == 'S') {
#ifdef RTCM104V2_ENABLE
	    if ((isgpsstat = rtcm2_decode(lexer, c)) == ISGPS_SYNC) {
		lexer->state = RTCM2_SYNC_STATE;
		break;
	    } else if (isgpsstat == ISGPS_MESSAGE) {
		lexer->state = RTCM2_RECOGNIZED;
		break;
	    }
#endif /* RTCM104V2_ENABLE */
	    lexer->state = ASTRAL_2;
	} else
	    lexer->state = GROUND_STATE;
	break;
    case ASTRAL_2:
	if (c == 'T') {
#ifdef RTCM104V2_ENABLE
	    if ((isgpsstat = rtcm2_decode(lexer, c)) == ISGPS_SYNC) {
		lexer->state = RTCM2_SYNC_STATE;
		break;
	    } else if (isgpsstat == ISGPS_MESSAGE) {
		lexer->state = RTCM2_RECOGNIZED;
		break;
	    }
#endif /* RTCM104V2_ENABLE */
	    lexer->state = ASTRAL_3;
	} else
	    lexer->state = GROUND_STATE;
	break;
    case ASTRAL_3:
	if (c == 'R') {
#ifdef RTCM104V2_ENABLE
	    if ((isgpsstat = rtcm2_decode(lexer, c)) == ISGPS_SYNC) {
		lexer->state = RTCM2_SYNC_STATE;
		break;
	    } else if (isgpsstat == ISGPS_MESSAGE) {
		lexer->state = RTCM2_RECOGNIZED;
		break;
	    }
#endif /* RTCM104V2_ENABLE */
	    lexer->state = ASTRAL_5;
	} else
	    lexer->state = GROUND_STATE;
	break;
    case ASTRAL_4:
	if (c == 'A') {
#ifdef RTCM104V2_ENABLE
	    if ((isgpsstat = rtcm2_decode(lexer, c)) == ISGPS_SYNC) {
		lexer->state = RTCM2_SYNC_STATE;
		break;
	    } else if (isgpsstat == ISGPS_MESSAGE) {
		lexer->state = RTCM2_RECOGNIZED;
		break;
	    }
#endif /* RTCM104V2_ENABLE */
	    lexer->state = ASTRAL_2;
	} else
	    lexer->state = GROUND_STATE;
	break;
    case ASTRAL_5:
	if (c == 'L') {
#ifdef RTCM104V2_ENABLE
	    if ((isgpsstat = rtcm2_decode(lexer, c)) == ISGPS_SYNC) {
		lexer->state = RTCM2_SYNC_STATE;
		break;
	    } else if (isgpsstat == ISGPS_MESSAGE) {
		lexer->state = RTCM2_RECOGNIZED;
		break;
	    }
#endif /* RTCM104V2_ENABLE */
	    lexer->state = NMEA_RECOGNIZED;
	} else
	    lexer->state = GROUND_STATE;
	break;
#endif /* TRIPMATE_ENABLE */
#ifdef EARTHMATE_ENABLE
    case EARTHA_1:
	if (c == 'A') {
#ifdef RTCM104V2_ENABLE
	    if ((isgpsstat = rtcm2_decode(lexer, c)) == ISGPS_SYNC) {
		lexer->state = RTCM2_SYNC_STATE;
		break;
	    } else if (isgpsstat == ISGPS_MESSAGE) {
		lexer->state = RTCM2_RECOGNIZED;
		break;
	    }
#endif /* RTCM104V2_ENABLE */
	    lexer->state = EARTHA_2;
	} else
	    lexer->state = GROUND_STATE;
	break;
    case EARTHA_2:
	if (c == 'R') {
#ifdef RTCM104V2_ENABLE
	    if ((isgpsstat = rtcm2_decode(lexer, c)) == ISGPS_SYNC) {
		lexer->state = RTCM2_SYNC_STATE;
		break;
	    } else if (isgpsstat == ISGPS_MESSAGE) {
		lexer->state = RTCM2_RECOGNIZED;
		break;
	    }
#endif /* RTCM104V2_ENABLE */
	    lexer->state = EARTHA_3;
	} else
	    lexer->state = GROUND_STATE;
	break;
    case EARTHA_3:
	if (c == 'T') {
#ifdef RTCM104V2_ENABLE
	    if ((isgpsstat = rtcm2_decode(lexer, c)) == ISGPS_SYNC) {
		lexer->state = RTCM2_SYNC_STATE;
		break;
	    } else if (isgpsstat == ISGPS_MESSAGE) {
		lexer->state = RTCM2_RECOGNIZED;
		break;
	    }
#endif /* RTCM104V2_ENABLE */
	    lexer->state = EARTHA_4;
	} else
	    lexer->state = GROUND_STATE;
	break;
    case EARTHA_4:
	if (c == 'H') {
#ifdef RTCM104V2_ENABLE
	    if ((isgpsstat = rtcm2_decode(lexer, c)) == ISGPS_SYNC) {
		lexer->state = RTCM2_SYNC_STATE;
		break;
	    } else if (isgpsstat == ISGPS_MESSAGE) {
		lexer->state = RTCM2_RECOGNIZED;
		break;
	    }
#endif /* RTCM104V2_ENABLE */
	    lexer->state = EARTHA_5;
	} else
	    lexer->state = GROUND_STATE;
	break;
    case EARTHA_5:
	if (c == 'A') {
#ifdef RTCM104V2_ENABLE
	    if ((isgpsstat = rtcm2_decode(lexer, c)) == ISGPS_SYNC) {
		lexer->state = RTCM2_SYNC_STATE;
		break;
	    } else if (isgpsstat == ISGPS_MESSAGE) {
		lexer->state = RTCM2_RECOGNIZED;
		break;
	    }
#endif /* RTCM104V2_ENABLE */
	    lexer->state = NMEA_RECOGNIZED;
	} else
	    lexer->state = GROUND_STATE;
	break;
#endif /* EARTHMATE_ENABLE */
    case SIRF_ACK_LEAD_1:
	if (c == 'c')
	    lexer->state = SIRF_ACK_LEAD_2;
	else if (c == 'I')
	    lexer->state = AIS_LEAD_2;
	else
	    lexer->state = GROUND_STATE;
	break;
    case SIRF_ACK_LEAD_2:
	if (c == 'k')
	    lexer->state = NMEA_LEADER_END;
	else
	    lexer->state = GROUND_STATE;
	break;
#endif /* NMEA_ENABLE */
#ifdef SIRF_ENABLE
    case SIRF_LEADER_1:
	if (c == 0xa2)
	    lexer->state = SIRF_LEADER_2;
	else
	    lexer->state = GROUND_STATE;
	break;
    case SIRF_LEADER_2:
	lexer->length = (size_t) (c << 8);
	lexer->state = SIRF_LENGTH_1;
	break;
    case SIRF_LENGTH_1:
	lexer->length += c + 2;
	if (lexer->length <= MAX_PACKET_LENGTH)
	    lexer->state = SIRF_PAYLOAD;
	else
	    lexer->state = GROUND_STATE;
	break;
    case SIRF_PAYLOAD:
	if (--lexer->length == 0)
	    lexer->state = SIRF_DELIVERED;
	break;
    case SIRF_DELIVERED:
	if (c == 0xb0)
	    lexer->state = SIRF_TRAILER_1;
	else
	    lexer->state = GROUND_STATE;
	break;
    case SIRF_TRAILER_1:
	if (c == 0xb3)
	    lexer->state = SIRF_RECOGNIZED;
	else
	    lexer->state = GROUND_STATE;
	break;
    case SIRF_RECOGNIZED:
	if (c == 0xa0)
	    lexer->state = SIRF_LEADER_1;
	else
	    lexer->state = GROUND_STATE;
	break;
#endif /* SIRF_ENABLE */
#ifdef SUPERSTAR2_ENABLE
    case SUPERSTAR2_LEADER:
	ctmp = c;
	lexer->state = SUPERSTAR2_ID1;
	break;
    case SUPERSTAR2_ID1:
	if ((ctmp ^ 0xff) == c)
	    lexer->state = SUPERSTAR2_ID2;
	else
	    lexer->state = GROUND_STATE;
	break;
    case SUPERSTAR2_ID2:
	lexer->length = (size_t) c;	/* how many data bytes follow this byte */
	if (lexer->length)
	    lexer->state = SUPERSTAR2_PAYLOAD;
	else
	    lexer->state = SUPERSTAR2_CKSUM1;	/* no data, jump to checksum */
	break;
    case SUPERSTAR2_PAYLOAD:
	if (--lexer->length == 0)
	    lexer->state = SUPERSTAR2_CKSUM1;
	break;
    case SUPERSTAR2_CKSUM1:
	lexer->state = SUPERSTAR2_CKSUM2;
	break;
    case SUPERSTAR2_CKSUM2:
	lexer->state = SUPERSTAR2_RECOGNIZED;
	break;
    case SUPERSTAR2_RECOGNIZED:
	if (c == SOH)
	    lexer->state = SUPERSTAR2_LEADER;
	else
	    lexer->state = GROUND_STATE;
	break;
#endif /* SUPERSTAR2_ENABLE */
#ifdef ONCORE_ENABLE
    case ONCORE_AT2:
	if (isupper(c)) {
	    lexer->length = (size_t) c;
	    lexer->state = ONCORE_ID1;
	} else
	    lexer->state = GROUND_STATE;
	break;
    case ONCORE_ID1:
	if (isalpha(c)) {
	    lexer->length =
		oncore_payload_cksum_length((unsigned char)lexer->length, c);
	    if (lexer->length != 0) {
		lexer->state = ONCORE_PAYLOAD;
		break;
	    }
	}
	lexer->state = GROUND_STATE;
	break;
    case ONCORE_PAYLOAD:
	if (--lexer->length == 0)
	    lexer->state = ONCORE_CHECKSUM;
	break;
    case ONCORE_CHECKSUM:
	if (c != '\r')
	    lexer->state = GROUND_STATE;
	else
	    lexer->state = ONCORE_CR;
	break;
    case ONCORE_CR:
	if (c == '\n')
	    lexer->state = ONCORE_RECOGNIZED;
	else
	    lexer->state = ONCORE_PAYLOAD;
	break;
    case ONCORE_RECOGNIZED:
	if (c == '@')
	    lexer->state = AT1_LEADER;
	else
	    lexer->state = GROUND_STATE;
	break;
#endif /* ONCORE_ENABLE */
#if defined(TSIP_ENABLE) || defined(EVERMORE_ENABLE) || defined(GARMIN_ENABLE)
    case DLE_LEADER:
#ifdef EVERMORE_ENABLE
	if (c == STX) {
	    lexer->state = EVERMORE_LEADER_2;
	    break;
	}
#endif /* EVERMORE_ENABLE */
#if defined(TSIP_ENABLE) || defined(GARMIN_ENABLE) || defined(NAVCOM_ENABLE)
	/* garmin is special case of TSIP */
	/* check last because there's no checksum */
#if defined(TSIP_ENABLE)
	if (c >= 0x13) {
	    lexer->state = TSIP_PAYLOAD;
	    break;
	}
#endif /* TSIP_ENABLE */
	if (c == DLE) {
	    lexer->state = GROUND_STATE;
	    break;
	}
	// FALL-THRU!!!!! no break here
#endif /* TSIP_ENABLE */
#ifdef NAVCOM_ENABLE
    case NAVCOM_LEADER_1:
	if (c == 0x99)
	    lexer->state = NAVCOM_LEADER_2;
	else
	    lexer->state = GROUND_STATE;
	break;
    case NAVCOM_LEADER_2:
	if (c == 0x66)
	    lexer->state = NAVCOM_LEADER_3;
	else
	    lexer->state = GROUND_STATE;
	break;
    case NAVCOM_LEADER_3:
	lexer->state = NAVCOM_ID;
	break;
    case NAVCOM_ID:
	lexer->length = (size_t) c - 4;
	lexer->state = NAVCOM_LENGTH_1;
	break;
    case NAVCOM_LENGTH_1:
	lexer->length += (c << 8);
	lexer->state = NAVCOM_LENGTH_2;
	break;
    case NAVCOM_LENGTH_2:
	if (--lexer->length == 0)
	    lexer->state = NAVCOM_PAYLOAD;
	break;
    case NAVCOM_PAYLOAD:
    {
	unsigned char csum = lexer->inbuffer[3];
	for (n = 4;
	     (unsigned char *)(lexer->inbuffer + n) < lexer->inbufptr - 1;
	     n++)
	    csum ^= lexer->inbuffer[n];
	if (csum != c) {
	    gpsd_report(LOG_IO,
			"Navcom packet type 0x%hhx bad checksum 0x%hhx, expecting 0x%x\n",
			lexer->inbuffer[3], csum, c);
	    lexer->state = GROUND_STATE;
	    break;
	}
    }
	lexer->state = NAVCOM_CSUM;
	break;
    case NAVCOM_CSUM:
	if (c == 0x03)
	    lexer->state = NAVCOM_RECOGNIZED;
	else
	    lexer->state = GROUND_STATE;
	break;
    case NAVCOM_RECOGNIZED:
	if (c == 0x02)
	    lexer->state = NAVCOM_LEADER_1;
	else
	    lexer->state = GROUND_STATE;
	break;
#endif /* NAVCOM_ENABLE */
#endif /* TSIP_ENABLE || EVERMORE_ENABLE || GARMIN_ENABLE */
#ifdef RTCM104V3_ENABLE
    case RTCM3_LEADER_1:
	/* high 6 bits must be zero, low 2 bits are MSB of a 10-bit length */
	if ((c & 0xFC) == 0) {
	    lexer->length = (size_t) (c << 8);
	    lexer->state = RTCM3_LEADER_2;
	    break;
	} else
	    lexer->state = GROUND_STATE;
	break;
    case RTCM3_LEADER_2:
	/* third byte is the low 8 bits of the RTCM3 packet length */
	lexer->length |= c;
	lexer->length += 3;	/* to get the three checksum bytes */
	lexer->state = RTCM3_PAYLOAD;
	break;
    case RTCM3_PAYLOAD:
	if (--lexer->length == 0)
	    lexer->state = RTCM3_RECOGNIZED;
	break;
#endif /* RTCM104V3_ENABLE */
#ifdef ZODIAC_ENABLE
    case ZODIAC_EXPECTED:
    case ZODIAC_RECOGNIZED:
	if (c == 0xff)
	    lexer->state = ZODIAC_LEADER_1;
	else
	    lexer->state = GROUND_STATE;
	break;
    case ZODIAC_LEADER_1:
	if (c == 0x81)
	    lexer->state = ZODIAC_LEADER_2;
	else
	    lexer->state = GROUND_STATE;
	break;
    case ZODIAC_LEADER_2:
	lexer->state = ZODIAC_ID_1;
	break;
    case ZODIAC_ID_1:
	lexer->state = ZODIAC_ID_2;
	break;
    case ZODIAC_ID_2:
	lexer->length = (size_t) c;
	lexer->state = ZODIAC_LENGTH_1;
	break;
    case ZODIAC_LENGTH_1:
	lexer->length += (c << 8);
	lexer->state = ZODIAC_LENGTH_2;
	break;
    case ZODIAC_LENGTH_2:
	lexer->state = ZODIAC_FLAGS_1;
	break;
    case ZODIAC_FLAGS_1:
	lexer->state = ZODIAC_FLAGS_2;
	break;
    case ZODIAC_FLAGS_2:
	lexer->state = ZODIAC_HSUM_1;
	break;
    case ZODIAC_HSUM_1:
    {
#define getword(i) (short)(lexer->inbuffer[2*(i)] | (lexer->inbuffer[2*(i)+1] << 8))
	short sum = getword(0) + getword(1) + getword(2) + getword(3);
	sum *= -1;
	if (sum != getword(4)) {
	    gpsd_report(LOG_IO,
			"Zodiac Header checksum 0x%hx expecting 0x%hx\n",
			sum, getword(4));
	    lexer->state = GROUND_STATE;
	    break;
	}
    }
	gpsd_report(LOG_RAW + 1, "Zodiac header id=%hd len=%hd flags=%hx\n",
		    getword(1), getword(2), getword(3));
#undef getword
	if (lexer->length == 0) {
	    lexer->state = ZODIAC_RECOGNIZED;
	    break;
	}
	lexer->length *= 2;	/* word count to byte count */
	lexer->length += 2;	/* checksum */
	/* 10 bytes is the length of the Zodiac header */
	if (lexer->length <= MAX_PACKET_LENGTH - 10)
	    lexer->state = ZODIAC_PAYLOAD;
	else
	    lexer->state = GROUND_STATE;
	break;
    case ZODIAC_PAYLOAD:
	if (--lexer->length == 0)
	    lexer->state = ZODIAC_RECOGNIZED;
	break;
#endif /* ZODIAC_ENABLE */
#ifdef UBX_ENABLE
    case UBX_LEADER_1:
	if (c == 0x62)
	    lexer->state = UBX_LEADER_2;
	else
	    lexer->state = GROUND_STATE;
	break;
    case UBX_LEADER_2:
	lexer->state = UBX_CLASS_ID;
	break;
    case UBX_CLASS_ID:
	lexer->state = UBX_MESSAGE_ID;
	break;
    case UBX_MESSAGE_ID:
	lexer->length = (size_t) c;
	lexer->state = UBX_LENGTH_1;
	break;
    case UBX_LENGTH_1:
	lexer->length += (c << 8);
	if (lexer->length <= MAX_PACKET_LENGTH)
	    lexer->state = UBX_LENGTH_2;
	else
	    lexer->state = GROUND_STATE;
	break;
    case UBX_LENGTH_2:
	lexer->state = UBX_PAYLOAD;
	break;
    case UBX_PAYLOAD:
	if (--lexer->length == 0)
	    lexer->state = UBX_CHECKSUM_A;
	/* else stay in payload state */
	break;
    case UBX_CHECKSUM_A:
	lexer->state = UBX_RECOGNIZED;
	break;
    case UBX_RECOGNIZED:
	if (c == 0xb5)
	    lexer->state = UBX_LEADER_1;
#ifdef NMEA_ENABLE
	else if (c == '$')	/* LEA-5H can and will output NMEA and UBX back to back */
	    lexer->state = NMEA_DOLLAR;
#endif /* NMEA_ENABLE */
	else
	    lexer->state = GROUND_STATE;
	break;
#endif /* UBX_ENABLE */
#ifdef EVERMORE_ENABLE
    case EVERMORE_LEADER_1:
	if (c == STX)
	    lexer->state = EVERMORE_LEADER_2;
	else
	    lexer->state = GROUND_STATE;
	break;
    case EVERMORE_LEADER_2:
	lexer->length = (size_t) c;
	if (c == DLE)
	    lexer->state = EVERMORE_PAYLOAD_DLE;
	else
	    lexer->state = EVERMORE_PAYLOAD;
	break;
    case EVERMORE_PAYLOAD:
	if (c == DLE)
	    lexer->state = EVERMORE_PAYLOAD_DLE;
	else if (--lexer->length == 0)
	    lexer->state = GROUND_STATE;
	break;
    case EVERMORE_PAYLOAD_DLE:
	switch (c) {
	case DLE:
	    lexer->state = EVERMORE_PAYLOAD;
	    break;
	case ETX:
	    lexer->state = EVERMORE_RECOGNIZED;
	    break;
	default:
	    lexer->state = GROUND_STATE;
	}
	break;
    case EVERMORE_RECOGNIZED:
	if (c == DLE)
	    lexer->state = EVERMORE_LEADER_1;
	else
	    lexer->state = GROUND_STATE;
	break;
#endif /* EVERMORE_ENABLE */
#ifdef ITRAX_ENABLE
    case ITALK_LEADER_1:
	if (c == '!')
	    lexer->state = ITALK_LEADER_2;
	else
	    lexer->state = GROUND_STATE;
	break;
    case ITALK_LEADER_2:
	lexer->length = (size_t) (lexer->inbuffer[6] & 0xff);
	lexer->state = ITALK_LENGTH;
	break;
    case ITALK_LENGTH:
	lexer->length += 1;	/* fix number of words in payload */
	lexer->length *= 2;	/* convert to number of bytes */
	lexer->length += 3;	/* add trailer length */
	lexer->state = ITALK_PAYLOAD;
	break;
    case ITALK_PAYLOAD:
	/* lookahead for "<!" because sometimes packets are short but valid */
	if ((c == '>') && (lexer->inbufptr[0] == '<') &&
	    (lexer->inbufptr[1] == '!')) {
	    lexer->state = ITALK_RECOGNIZED;
	    gpsd_report(LOG_IO, "ITALK: trying to process runt packet\n");
	    break;
	} else if (--lexer->length == 0)
	    lexer->state = ITALK_DELIVERED;
	break;
    case ITALK_DELIVERED:
	if (c == '>')
	    lexer->state = ITALK_RECOGNIZED;
	else
	    lexer->state = GROUND_STATE;
	break;
    case ITALK_RECOGNIZED:
	if (c == '<')
	    lexer->state = ITALK_LEADER_1;
	else
	    lexer->state = GROUND_STATE;
	break;
#endif /* ITRAX_ENABLE */
#ifdef GEOSTAR_ENABLE
    case GEOSTAR_LEADER_1:
	if (c == 'S')
	    lexer->state = GEOSTAR_LEADER_2;
	else
	    lexer->state = GROUND_STATE;
	break;
    case GEOSTAR_LEADER_2:
	if (c == 'G')
	    lexer->state = GEOSTAR_LEADER_3;
	else
	    lexer->state = GROUND_STATE;
	break;
    case GEOSTAR_LEADER_3:
	if (c == 'G')
	    lexer->state = GEOSTAR_LEADER_4;
	else
	    lexer->state = GROUND_STATE;
	break;
    case GEOSTAR_LEADER_4:
	lexer->state = GEOSTAR_MESSAGE_ID_1;
	break;
    case GEOSTAR_MESSAGE_ID_1:
	lexer->state = GEOSTAR_MESSAGE_ID_2;
	break;
    case GEOSTAR_MESSAGE_ID_2:
	lexer->length = (size_t)(c * 4);
	lexer->state = GEOSTAR_LENGTH_1;
	break;
    case GEOSTAR_LENGTH_1:
	lexer->length += (c << 8) * 4;
	if (lexer->length <= MAX_PACKET_LENGTH)
	    lexer->state = GEOSTAR_LENGTH_2;
	else
	    lexer->state = GROUND_STATE;
	break;
    case GEOSTAR_LENGTH_2:
	lexer->state = GEOSTAR_PAYLOAD;
	break;
    case GEOSTAR_PAYLOAD:
	if (--lexer->length == 0)
	    lexer->state = GEOSTAR_CHECKSUM_A;
	/* else stay in payload state */
	break;
    case GEOSTAR_CHECKSUM_A:
	lexer->state = GEOSTAR_CHECKSUM_B;
	break;
    case GEOSTAR_CHECKSUM_B:
	lexer->state = GEOSTAR_CHECKSUM_C;
	break;
    case GEOSTAR_CHECKSUM_C:
	lexer->state = GEOSTAR_RECOGNIZED;
	break;
    case GEOSTAR_RECOGNIZED:
	if (c == 'P')
	    lexer->state = GEOSTAR_LEADER_1;
	else
	    lexer->state = GROUND_STATE;
	break;
#endif /* GEOSTAR_ENABLE */
#ifdef TSIP_ENABLE
    case TSIP_LEADER:
	/* unused case */
	if (c >= 0x13)
	    lexer->state = TSIP_PAYLOAD;
	else
	    lexer->state = GROUND_STATE;
	break;
    case TSIP_PAYLOAD:
	if (c == DLE)
	    lexer->state = TSIP_DLE;
	break;
    case TSIP_DLE:
	switch (c) {
	case ETX:
	    lexer->state = TSIP_RECOGNIZED;
	    break;
	case DLE:
	    lexer->state = TSIP_PAYLOAD;
	    break;
	default:
	    lexer->state = GROUND_STATE;
	    break;
	}
	break;
    case TSIP_RECOGNIZED:
	if (c == DLE)
	    /*
	     * Don't go to TSIP_LEADER state -- TSIP packets aren't
	     * checksummed, so false positives are easy.  We might be
	     * looking at another DLE-stuffed protocol like EverMore
	     * or Garmin streaming binary.
	     */
	    lexer->state = DLE_LEADER;
	else
	    lexer->state = GROUND_STATE;
	break;
#endif /* TSIP_ENABLE */
#ifdef RTCM104V2_ENABLE
    case RTCM2_SYNC_STATE:
    case RTCM2_SKIP_STATE:
	if ((isgpsstat = rtcm2_decode(lexer, c)) == ISGPS_MESSAGE) {
	    lexer->state = RTCM2_RECOGNIZED;
	    break;
	} else if (isgpsstat == ISGPS_NO_SYNC)
	    lexer->state = GROUND_STATE;
	break;

    case RTCM2_RECOGNIZED:
	if (rtcm2_decode(lexer, c) == ISGPS_SYNC) {
	    lexer->state = RTCM2_SYNC_STATE;
	    break;
	} else
	    lexer->state = GROUND_STATE;
	break;
#endif /* RTCM104V2_ENABLE */
#ifdef PASSTHROUGH_ENABLE
    case JSON_LEADER:
	if (c == '{' || c == '[') {
	    lexer->json_depth++;
	} else if (c == '}' || c == ']') {
	    if (--lexer->json_depth == 0)
		lexer->state = JSON_RECOGNIZED;
	} else if (isspace(c) || c == ',')
	    break;
	else if (c == '"') {
	    lexer->state = JSON_STRINGLITERAL;
	    lexer->json_after = JSON_END_ATTRIBUTE;
	} else {
	    gpsd_report(LOG_RAW + 2,
			"%08ld: missing attribute start after header\n",
			lexer->char_counter);
	    lexer->state = GROUND_STATE;
	}
	break;
    case JSON_STRINGLITERAL:
	if (c == '\\')
	    lexer->state = JSON_STRING_SOLIDUS;
	else if (c == '"')
	    lexer->state = lexer->json_after;
	break;
    case JSON_STRING_SOLIDUS:
	lexer->state = JSON_STRINGLITERAL;
	break;
    case JSON_END_ATTRIBUTE:
	if (isspace(c))
	    break;
	else if (c == ':')
	    lexer->state = JSON_EXPECT_VALUE;
	else
	    /* saw something other than value start after colon */
	    lexer->state = GROUND_STATE;
	break;
    case JSON_EXPECT_VALUE:
	if (isspace(c))
	    break;
	else if (c == '"') {
	    lexer->state = JSON_STRINGLITERAL;
	    lexer->json_after = JSON_END_VALUE;
	} else if (c == '{' || c == '[') {
	    lexer->state = JSON_LEADER;
	    character_pushback(lexer);
	} else if (strchr("-0123456789", c) != NULL) {
	    lexer->state = JSON_NUMBER;
	} else if (c == 't' || c == 'f' || c == 'n')
	    /*
	     * This is a bit more permissive than strictly necessary, as
	     * GPSD JSON does not include the null token.  Still, it's
	     * futureproofing.
	     */
	    lexer->state = JSON_SPECIAL;
	else
	    /* couldn't recognize start of value literal */
	    lexer->state = GROUND_STATE;
	break;
    case JSON_NUMBER:
	/*
	 * Will recognize some ill-formed numeric literals.
	 * Should be OK as we're already three stages deep inside
	 * JSON recognition; odds that we'll actually see an
	 * ill-formed literal are quite low. and the worst
	 * possible result if it happens is our JSON parser will
	 * quietly chuck out the object.
	 */
	if (strchr("1234567890.eE+-", c) == NULL) {
	    lexer->state = JSON_END_VALUE;
	    character_pushback(lexer);
	}
	break;
    case JSON_SPECIAL:
	if (strchr("truefalsnil", c) == NULL) {
	    lexer->state = JSON_END_VALUE;
	    character_pushback(lexer);
	}
	break;
    case JSON_END_VALUE:
	if (isspace(c))
	    break;
	else if (c == ',')
	    lexer->state = JSON_LEADER;
	else if (c == '}' || c == ']') {
	    lexer->state = JSON_LEADER;
	    character_pushback(lexer);
	} else
	    /* trailing garbage after JSON value */
	    lexer->state = GROUND_STATE;
	break;
#endif /* PASSTHROUGH_ENABLE */
    }
/*@ -charint +casebreak @*/
}