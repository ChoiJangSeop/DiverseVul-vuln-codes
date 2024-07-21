check(str, sub, should)
char *str;
my_regmatch_t sub;
char *should;
{
	register int len;
	register int shlen;
	register char *p;
	static char grump[500];
	register char *at = NULL;

	if (should != NULL && strcmp(should, "-") == 0)
		should = NULL;
	if (should != NULL && should[0] == '@') {
		at = should + 1;
		should = (char*) "";
	}

	/* check rm_so and rm_eo for consistency */
	if (sub.rm_so > sub.rm_eo || (sub.rm_so == -1 && sub.rm_eo != -1) ||
				(sub.rm_so != -1 && sub.rm_eo == -1) ||
				(sub.rm_so != -1 && sub.rm_so < 0) ||
				(sub.rm_eo != -1 && sub.rm_eo < 0) ) {
		sprintf(grump, "start %ld end %ld", (long)sub.rm_so,
							(long)sub.rm_eo);
		return(grump);
	}

	/* check for no match */
	if (sub.rm_so == -1 && should == NULL)
		return(NULL);
	if (sub.rm_so == -1)
		return((char*) "did not match");

	/* check for in range */
	if ((int) sub.rm_eo > (int) strlen(str)) {
		sprintf(grump, "start %ld end %ld, past end of string",
					(long)sub.rm_so, (long)sub.rm_eo);
		return(grump);
	}

	len = (int)(sub.rm_eo - sub.rm_so);
	shlen = (int)strlen(should);
	p = str + sub.rm_so;

	/* check for not supposed to match */
	if (should == NULL) {
		sprintf(grump, "matched `%.*s'", len, p);
		return(grump);
	}

	/* check for wrong match */
	if (len != shlen || strncmp(p, should, (size_t)shlen) != 0) {
		sprintf(grump, "matched `%.*s' instead", len, p);
		return(grump);
	}
	if (shlen > 0)
		return(NULL);

	/* check null match in right place */
	if (at == NULL)
		return(NULL);
	shlen = strlen(at);
	if (shlen == 0)
		shlen = 1;	/* force check for end-of-string */
	if (strncmp(p, at, shlen) != 0) {
		sprintf(grump, "matched null at `%.20s'", p);
		return(grump);
	}
	return(NULL);
}