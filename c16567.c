int EVP_DecodeUpdate(EVP_ENCODE_CTX *ctx, unsigned char *out, int *outl,
	     const unsigned char *in, int inl)
	{
	int seof= -1,eof=0,rv= -1,ret=0,i,v,tmp,n,ln,exp_nl;
	unsigned char *d;

	n=ctx->num;
	d=ctx->enc_data;
	ln=ctx->line_num;
	exp_nl=ctx->expect_nl;

	/* last line of input. */
	if ((inl == 0) || ((n == 0) && (conv_ascii2bin(in[0]) == B64_EOF)))
		{ rv=0; goto end; }
		
	/* We parse the input data */
	for (i=0; i<inl; i++)
		{
		/* If the current line is > 80 characters, scream alot */
		if (ln >= 80) { rv= -1; goto end; }

		/* Get char and put it into the buffer */
		tmp= *(in++);
		v=conv_ascii2bin(tmp);
		/* only save the good data :-) */
		if (!B64_NOT_BASE64(v))
			{
			OPENSSL_assert(n < (int)sizeof(ctx->enc_data));
			d[n++]=tmp;
			ln++;
			}
		else if (v == B64_ERROR)
			{
			rv= -1;
			goto end;
			}

		/* have we seen a '=' which is 'definitly' the last
		 * input line.  seof will point to the character that
		 * holds it. and eof will hold how many characters to
		 * chop off. */
		if (tmp == '=')
			{
			if (seof == -1) seof=n;
			eof++;
			}

		if (v == B64_CR)
			{
			ln = 0;
			if (exp_nl)
				continue;
			}

		/* eoln */
		if (v == B64_EOLN)
			{
			ln=0;
			if (exp_nl)
				{
				exp_nl=0;
				continue;
				}
			}
		exp_nl=0;

		/* If we are at the end of input and it looks like a
		 * line, process it. */
		if (((i+1) == inl) && (((n&3) == 0) || eof))
			{
			v=B64_EOF;
			/* In case things were given us in really small
			   records (so two '=' were given in separate
			   updates), eof may contain the incorrect number
			   of ending bytes to skip, so let's redo the count */
			eof = 0;
			if (d[n-1] == '=') eof++;
			if (d[n-2] == '=') eof++;
			/* There will never be more than two '=' */
			}

		if ((v == B64_EOF && (n&3) == 0) || (n >= 64))
			{
			/* This is needed to work correctly on 64 byte input
			 * lines.  We process the line and then need to
			 * accept the '\n' */
			if ((v != B64_EOF) && (n >= 64)) exp_nl=1;
			if (n > 0)
				{
				v=EVP_DecodeBlock(out,d,n);
				n=0;
				if (v < 0) { rv=0; goto end; }
				ret+=(v-eof);
				}
			else
				{
				eof=1;
				v=0;
				}

			/* This is the case where we have had a short
			 * but valid input line */
			if ((v < ctx->length) && eof)
				{
				rv=0;
				goto end;
				}
			else
				ctx->length=v;

			if (seof >= 0) { rv=0; goto end; }
			out+=v;
			}
		}
	rv=1;
end:
	*outl=ret;
	ctx->num=n;
	ctx->line_num=ln;
	ctx->expect_nl=exp_nl;
	return(rv);
	}