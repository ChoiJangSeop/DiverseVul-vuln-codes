void rose_write_internal(struct sock *sk, int frametype)
{
	struct rose_sock *rose = rose_sk(sk);
	struct sk_buff *skb;
	unsigned char  *dptr;
	unsigned char  lci1, lci2;
	char buffer[100];
	int len, faclen = 0;

	len = AX25_BPQ_HEADER_LEN + AX25_MAX_HEADER_LEN + ROSE_MIN_LEN + 1;

	switch (frametype) {
	case ROSE_CALL_REQUEST:
		len   += 1 + ROSE_ADDR_LEN + ROSE_ADDR_LEN;
		faclen = rose_create_facilities(buffer, rose);
		len   += faclen;
		break;
	case ROSE_CALL_ACCEPTED:
	case ROSE_CLEAR_REQUEST:
	case ROSE_RESET_REQUEST:
		len   += 2;
		break;
	}

	if ((skb = alloc_skb(len, GFP_ATOMIC)) == NULL)
		return;

	/*
	 *	Space for AX.25 header and PID.
	 */
	skb_reserve(skb, AX25_BPQ_HEADER_LEN + AX25_MAX_HEADER_LEN + 1);

	dptr = skb_put(skb, skb_tailroom(skb));

	lci1 = (rose->lci >> 8) & 0x0F;
	lci2 = (rose->lci >> 0) & 0xFF;

	switch (frametype) {
	case ROSE_CALL_REQUEST:
		*dptr++ = ROSE_GFI | lci1;
		*dptr++ = lci2;
		*dptr++ = frametype;
		*dptr++ = 0xAA;
		memcpy(dptr, &rose->dest_addr,  ROSE_ADDR_LEN);
		dptr   += ROSE_ADDR_LEN;
		memcpy(dptr, &rose->source_addr, ROSE_ADDR_LEN);
		dptr   += ROSE_ADDR_LEN;
		memcpy(dptr, buffer, faclen);
		dptr   += faclen;
		break;

	case ROSE_CALL_ACCEPTED:
		*dptr++ = ROSE_GFI | lci1;
		*dptr++ = lci2;
		*dptr++ = frametype;
		*dptr++ = 0x00;		/* Address length */
		*dptr++ = 0;		/* Facilities length */
		break;

	case ROSE_CLEAR_REQUEST:
		*dptr++ = ROSE_GFI | lci1;
		*dptr++ = lci2;
		*dptr++ = frametype;
		*dptr++ = rose->cause;
		*dptr++ = rose->diagnostic;
		break;

	case ROSE_RESET_REQUEST:
		*dptr++ = ROSE_GFI | lci1;
		*dptr++ = lci2;
		*dptr++ = frametype;
		*dptr++ = ROSE_DTE_ORIGINATED;
		*dptr++ = 0;
		break;

	case ROSE_RR:
	case ROSE_RNR:
		*dptr++ = ROSE_GFI | lci1;
		*dptr++ = lci2;
		*dptr   = frametype;
		*dptr++ |= (rose->vr << 5) & 0xE0;
		break;

	case ROSE_CLEAR_CONFIRMATION:
	case ROSE_RESET_CONFIRMATION:
		*dptr++ = ROSE_GFI | lci1;
		*dptr++ = lci2;
		*dptr++  = frametype;
		break;

	default:
		printk(KERN_ERR "ROSE: rose_write_internal - invalid frametype %02X\n", frametype);
		kfree_skb(skb);
		return;
	}

	rose_transmit_link(skb, rose->neighbour);
}