plain(void)
{
	test(PTR1_ZEROES PTR1_STR " " PTR2_STR, "%p %p", PTR1, PTR2);
	/*
	 * The field width is overloaded for some %p extensions to
	 * pass another piece of information. For plain pointers, the
	 * behaviour is slightly odd: One cannot pass either the 0
	 * flag nor a precision to %p without gcc complaining, and if
	 * one explicitly gives a field width, the number is no longer
	 * zero-padded.
	 */
	test("|" PTR1_STR PTR1_SPACES "  |  " PTR1_SPACES PTR1_STR "|",
	     "|%-*p|%*p|", PTR_WIDTH+2, PTR1, PTR_WIDTH+2, PTR1);
	test("|" PTR2_STR "  |  " PTR2_STR "|",
	     "|%-*p|%*p|", PTR_WIDTH+2, PTR2, PTR_WIDTH+2, PTR2);

	/*
	 * Unrecognized %p extensions are treated as plain %p, but the
	 * alphanumeric suffix is ignored (that is, does not occur in
	 * the output.)
	 */
	test("|"PTR1_ZEROES PTR1_STR"|", "|%p0y|", PTR1);
	test("|"PTR2_STR"|", "|%p0y|", PTR2);
}