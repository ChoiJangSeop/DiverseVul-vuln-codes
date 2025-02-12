static void dns_resolver_describe(const struct key *key, struct seq_file *m)
{
	seq_puts(m, key->description);
	if (key_is_instantiated(key)) {
		int err = PTR_ERR(key->payload.data[dns_key_error]);

		if (err)
			seq_printf(m, ": %d", err);
		else
			seq_printf(m, ": %u", key->datalen);
	}
}