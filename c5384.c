void MSG_WriteBits( msg_t *msg, int value, int bits ) {
	int	i;

	oldsize += bits;

	// this isn't an exact overflow check, but close enough
	if ( msg->maxsize - msg->cursize < 4 ) {
		msg->overflowed = qtrue;
		return;
	}

	if ( bits == 0 || bits < -31 || bits > 32 ) {
		Com_Error( ERR_DROP, "MSG_WriteBits: bad bits %i", bits );
	}

	if ( bits < 0 ) {
		bits = -bits;
	}

	if ( msg->oob ) {
		if ( bits == 8 ) {
			msg->data[msg->cursize] = value;
			msg->cursize += 1;
			msg->bit += 8;
		} else if ( bits == 16 ) {
			short temp = value;

			CopyLittleShort( &msg->data[msg->cursize], &temp );
			msg->cursize += 2;
			msg->bit += 16;
		} else if ( bits==32 ) {
			CopyLittleLong( &msg->data[msg->cursize], &value );
			msg->cursize += 4;
			msg->bit += 32;
		} else {
			Com_Error( ERR_DROP, "can't write %d bits", bits );
		}
	} else {
		value &= (0xffffffff >> (32 - bits));
		if ( bits&7 ) {
			int nbits;
			nbits = bits&7;
			for( i = 0; i < nbits; i++ ) {
				Huff_putBit( (value & 1), msg->data, &msg->bit );
				value = (value >> 1);
			}
			bits = bits - nbits;
		}
		if ( bits ) {
			for( i = 0; i < bits; i += 8 ) {
				Huff_offsetTransmit( &msgHuff.compressor, (value & 0xff), msg->data, &msg->bit );
				value = (value >> 8);
			}
		}
		msg->cursize = (msg->bit >> 3) + 1;
	}
}