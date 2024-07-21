NIAddSpell(IspellDict *Conf, const char *word, const char *flag)
{
	if (Conf->nspell >= Conf->mspell)
	{
		if (Conf->mspell)
		{
			Conf->mspell *= 2;
			Conf->Spell = (SPELL **) repalloc(Conf->Spell, Conf->mspell * sizeof(SPELL *));
		}
		else
		{
			Conf->mspell = 1024 * 20;
			Conf->Spell = (SPELL **) tmpalloc(Conf->mspell * sizeof(SPELL *));
		}
	}
	Conf->Spell[Conf->nspell] = (SPELL *) tmpalloc(SPELLHDRSZ + strlen(word) + 1);
	strcpy(Conf->Spell[Conf->nspell]->word, word);
	strncpy(Conf->Spell[Conf->nspell]->p.flag, flag, MAXFLAGLEN);
	Conf->nspell++;
}