static __u32 twothirdsMD4Transform(__u32 const buf[4], __u32 const in[12])
{
	__u32 a = buf[0], b = buf[1], c = buf[2], d = buf[3];

	/* Round 1 */
	ROUND(F, a, b, c, d, in[ 0] + K1,  3);
	ROUND(F, d, a, b, c, in[ 1] + K1,  7);
	ROUND(F, c, d, a, b, in[ 2] + K1, 11);
	ROUND(F, b, c, d, a, in[ 3] + K1, 19);
	ROUND(F, a, b, c, d, in[ 4] + K1,  3);
	ROUND(F, d, a, b, c, in[ 5] + K1,  7);
	ROUND(F, c, d, a, b, in[ 6] + K1, 11);
	ROUND(F, b, c, d, a, in[ 7] + K1, 19);
	ROUND(F, a, b, c, d, in[ 8] + K1,  3);
	ROUND(F, d, a, b, c, in[ 9] + K1,  7);
	ROUND(F, c, d, a, b, in[10] + K1, 11);
	ROUND(F, b, c, d, a, in[11] + K1, 19);

	/* Round 2 */
	ROUND(G, a, b, c, d, in[ 1] + K2,  3);
	ROUND(G, d, a, b, c, in[ 3] + K2,  5);
	ROUND(G, c, d, a, b, in[ 5] + K2,  9);
	ROUND(G, b, c, d, a, in[ 7] + K2, 13);
	ROUND(G, a, b, c, d, in[ 9] + K2,  3);
	ROUND(G, d, a, b, c, in[11] + K2,  5);
	ROUND(G, c, d, a, b, in[ 0] + K2,  9);
	ROUND(G, b, c, d, a, in[ 2] + K2, 13);
	ROUND(G, a, b, c, d, in[ 4] + K2,  3);
	ROUND(G, d, a, b, c, in[ 6] + K2,  5);
	ROUND(G, c, d, a, b, in[ 8] + K2,  9);
	ROUND(G, b, c, d, a, in[10] + K2, 13);

	/* Round 3 */
	ROUND(H, a, b, c, d, in[ 3] + K3,  3);
	ROUND(H, d, a, b, c, in[ 7] + K3,  9);
	ROUND(H, c, d, a, b, in[11] + K3, 11);
	ROUND(H, b, c, d, a, in[ 2] + K3, 15);
	ROUND(H, a, b, c, d, in[ 6] + K3,  3);
	ROUND(H, d, a, b, c, in[10] + K3,  9);
	ROUND(H, c, d, a, b, in[ 1] + K3, 11);
	ROUND(H, b, c, d, a, in[ 5] + K3, 15);
	ROUND(H, a, b, c, d, in[ 9] + K3,  3);
	ROUND(H, d, a, b, c, in[ 0] + K3,  9);
	ROUND(H, c, d, a, b, in[ 4] + K3, 11);
	ROUND(H, b, c, d, a, in[ 8] + K3, 15);

	return buf[1] + b; /* "most hashed" word */
	/* Alternative: return sum of all words? */
}