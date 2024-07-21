sec_recv(uint8 * rdpver)
{
	uint32 sec_flags;
	uint16 channel;
	STREAM s;

	while ((s = mcs_recv(&channel, rdpver)) != NULL)
	{
		if (rdpver != NULL)
		{
			if (*rdpver != 3)
			{
				if (*rdpver & 0x80)
				{
					in_uint8s(s, 8);	/* signature */
					sec_decrypt(s->p, s->end - s->p);
				}
				return s;
			}
		}
		if (g_encryption || (!g_licence_issued && !g_licence_error_result))
		{
			in_uint32_le(s, sec_flags);

			if (g_encryption)
			{
				if (sec_flags & SEC_ENCRYPT)
				{
					in_uint8s(s, 8);	/* signature */
					sec_decrypt(s->p, s->end - s->p);
				}

				if (sec_flags & SEC_LICENCE_NEG)
				{
					licence_process(s);
					continue;
				}

				if (sec_flags & 0x0400)	/* SEC_REDIRECT_ENCRYPT */
				{
					uint8 swapbyte;

					in_uint8s(s, 8);	/* signature */
					sec_decrypt(s->p, s->end - s->p);

					/* Check for a redirect packet, starts with 00 04 */
					if (s->p[0] == 0 && s->p[1] == 4)
					{
						/* for some reason the PDU and the length seem to be swapped.
						   This isn't good, but we're going to do a byte for byte
						   swap.  So the first foure value appear as: 00 04 XX YY,
						   where XX YY is the little endian length. We're going to
						   use 04 00 as the PDU type, so after our swap this will look
						   like: XX YY 04 00 */
						swapbyte = s->p[0];
						s->p[0] = s->p[2];
						s->p[2] = swapbyte;

						swapbyte = s->p[1];
						s->p[1] = s->p[3];
						s->p[3] = swapbyte;

						swapbyte = s->p[2];
						s->p[2] = s->p[3];
						s->p[3] = swapbyte;
					}
#ifdef WITH_DEBUG
					/* warning!  this debug statement will show passwords in the clear! */
					hexdump(s->p, s->end - s->p);
#endif
				}
			}
			else
			{
				if ((sec_flags & 0xffff) == SEC_LICENCE_NEG)
				{
					licence_process(s);
					continue;
				}
				s->p -= 4;
			}
		}

		if (channel != MCS_GLOBAL_CHANNEL)
		{
			channel_process(s, channel);
			if (rdpver != NULL)
				*rdpver = 0xff;
			return s;
		}

		return s;
	}

	return NULL;
}