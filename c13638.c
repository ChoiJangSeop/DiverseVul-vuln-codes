createenv(const struct rule *rule)
{
	struct env *env;
	u_int i;

	env = malloc(sizeof(*env));
	if (!env)
		err(1, NULL);
	RB_INIT(&env->root);
	env->count = 0;

	if (rule->options & KEEPENV) {
		extern char **environ;

		for (i = 0; environ[i] != NULL; i++) {
			struct envnode *node;
			const char *e, *eq;
			size_t len;
			char keybuf[1024];

			e = environ[i];

			/* ignore invalid or overlong names */
			if ((eq = strchr(e, '=')) == NULL || eq == e)
				continue;
			len = eq - e;
			if (len > sizeof(keybuf) - 1)
				continue;
			memcpy(keybuf, e, len);
			keybuf[len] = '\0';

			node = createnode(keybuf, eq + 1);
			if (RB_INSERT(envtree, &env->root, node)) {
				/* ignore any later duplicates */
				freenode(node);
			} else {
				env->count++;
			}
		}
	}

	return env;
}