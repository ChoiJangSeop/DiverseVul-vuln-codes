static void sasl_packet(sasl_session_t *p, char *buf, int len)
{
	int rc;
	size_t tlen = 0;
	char *cloak, *out = NULL;
	char temp[BUFSIZE];
	char mech[61];
	size_t out_len = 0;
	metadata_t *md;

	/* First piece of data in a session is the name of
	 * the SASL mechanism that will be used.
	 */
	if(!p->mechptr)
	{
		if(len > 60)
		{
			sasl_sts(p->uid, 'D', "F");
			destroy_session(p);
			return;
		}

		memcpy(mech, buf, len);
		mech[len] = '\0';

		if(!(p->mechptr = find_mechanism(mech)))
		{
			sasl_sts(p->uid, 'M', mechlist_string);

			sasl_sts(p->uid, 'D', "F");
			destroy_session(p);
			return;
		}

		rc = p->mechptr->mech_start(p, &out, &out_len);
	}else{
		if(len == 1 && *buf == '+')
			rc = p->mechptr->mech_step(p, (char []) { '\0' }, 0,
					&out, &out_len);
		else if ((tlen = base64_decode(buf, temp, BUFSIZE)) &&
				tlen != (size_t)-1)
			rc = p->mechptr->mech_step(p, temp, tlen, &out, &out_len);
		else
			rc = ASASL_FAIL;
	}

	/* Some progress has been made, reset timeout. */
	p->flags &= ~ASASL_MARKED_FOR_DELETION;

	if(rc == ASASL_DONE)
	{
		myuser_t *mu = login_user(p);
		if(mu)
		{
			if ((md = metadata_find(mu, "private:usercloak")))
				cloak = md->value;
			else
				cloak = "*";

			if (!(mu->flags & MU_WAITAUTH))
				svslogin_sts(p->uid, "*", "*", cloak, mu);
			sasl_sts(p->uid, 'D', "S");
			/* Will destroy session on introduction of user to net. */
		}
		else
		{
			sasl_sts(p->uid, 'D', "F");
			destroy_session(p);
		}
		return;
	}
	else if(rc == ASASL_MORE)
	{
		if(out_len)
		{
			if(base64_encode(out, out_len, temp, BUFSIZE))
			{
				sasl_write(p->uid, temp, strlen(temp));
				free(out);
				return;
			}
		}
		else
		{
			sasl_sts(p->uid, 'C', "+");
			free(out);
			return;
		}
	}

	/* If we reach this, they failed SASL auth, so if they were trying
	 * to identify as a specific user, bad_password them.
	 */
	if (p->username)
	{
		myuser_t *mu = myuser_find_by_nick(p->username);
		if (mu)
		{
			sourceinfo_t *si = sasl_sourceinfo_create(p);
			bad_password(si, mu);
			object_unref(si);
		}
	}

	free(out);
	sasl_sts(p->uid, 'D', "F");
	destroy_session(p);
}