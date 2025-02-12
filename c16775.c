horizontalDifference8(unsigned char *ip, int n, int stride, 
	unsigned short *wp, uint16 *From8)
{
    register int  r1, g1, b1, a1, r2, g2, b2, a2, mask;

#undef	 CLAMP
#define  CLAMP(v) (From8[(v)])

    mask = CODE_MASK;
    if (n >= stride) {
	if (stride == 3) {
	    r2 = wp[0] = CLAMP(ip[0]);  g2 = wp[1] = CLAMP(ip[1]);
	    b2 = wp[2] = CLAMP(ip[2]);
	    n -= 3;
	    while (n > 0) {
		n -= 3;
		r1 = CLAMP(ip[3]); wp[3] = (uint16)((r1-r2) & mask); r2 = r1;
		g1 = CLAMP(ip[4]); wp[4] = (uint16)((g1-g2) & mask); g2 = g1;
		b1 = CLAMP(ip[5]); wp[5] = (uint16)((b1-b2) & mask); b2 = b1;
		wp += 3;
		ip += 3;
	    }
	} else if (stride == 4) {
	    r2 = wp[0] = CLAMP(ip[0]);  g2 = wp[1] = CLAMP(ip[1]);
	    b2 = wp[2] = CLAMP(ip[2]);  a2 = wp[3] = CLAMP(ip[3]);
	    n -= 4;
	    while (n > 0) {
		n -= 4;
		r1 = CLAMP(ip[4]); wp[4] = (uint16)((r1-r2) & mask); r2 = r1;
		g1 = CLAMP(ip[5]); wp[5] = (uint16)((g1-g2) & mask); g2 = g1;
		b1 = CLAMP(ip[6]); wp[6] = (uint16)((b1-b2) & mask); b2 = b1;
		a1 = CLAMP(ip[7]); wp[7] = (uint16)((a1-a2) & mask); a2 = a1;
		wp += 4;
		ip += 4;
	    }
	} else {
	    wp += n + stride - 1;	/* point to last one */
	    ip += n + stride - 1;	/* point to last one */
	    n -= stride;
	    while (n > 0) {
		REPEAT(stride, wp[0] = CLAMP(ip[0]);
				wp[stride] -= wp[0];
				wp[stride] &= mask;
				wp--; ip--)
		n -= stride;
	    }
	    REPEAT(stride, wp[0] = CLAMP(ip[0]); wp--; ip--)
	}
    }
}