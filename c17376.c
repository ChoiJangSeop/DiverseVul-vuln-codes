static int cms_copy_content(BIO *out, BIO *in, unsigned int flags)
	{
	unsigned char buf[4096];
	int r = 0, i;
	BIO *tmpout = NULL;

	if (out == NULL)
		tmpout = BIO_new(BIO_s_null());
	else if (flags & CMS_TEXT)
		{
		tmpout = BIO_new(BIO_s_mem());
		BIO_set_mem_eof_return(tmpout, 0);
		}
	else
		tmpout = out;

	if(!tmpout)
		{
		CMSerr(CMS_F_CMS_COPY_CONTENT,ERR_R_MALLOC_FAILURE);
		goto err;
		}

	/* Read all content through chain to process digest, decrypt etc */
	for (;;)
	{
		i=BIO_read(in,buf,sizeof(buf));
		if (i <= 0)
			{
			if (BIO_method_type(in) == BIO_TYPE_CIPHER)
				{
				if (!BIO_get_cipher_status(in))
					goto err;
				}
			if (i < 0)
				goto err;
			break;
			}
				
		if (tmpout && (BIO_write(tmpout, buf, i) != i))
			goto err;
	}

	if(flags & CMS_TEXT)
		{
		if(!SMIME_text(tmpout, out))
			{
			CMSerr(CMS_F_CMS_COPY_CONTENT,CMS_R_SMIME_TEXT_ERROR);
			goto err;
			}
		}

	r = 1;

	err:
	if (tmpout && (tmpout != out))
		BIO_free(tmpout);
	return r;

	}