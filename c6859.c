static void prepare_cmd(struct argv_array *out, const struct child_process *cmd)
{
	if (!cmd->argv[0])
		die("BUG: command is empty");

	/*
	 * Add SHELL_PATH so in the event exec fails with ENOEXEC we can
	 * attempt to interpret the command with 'sh'.
	 */
	argv_array_push(out, SHELL_PATH);

	if (cmd->git_cmd) {
		argv_array_push(out, "git");
		argv_array_pushv(out, cmd->argv);
	} else if (cmd->use_shell) {
		prepare_shell_cmd(out, cmd->argv);
	} else {
		argv_array_pushv(out, cmd->argv);
	}

	/*
	 * If there are no '/' characters in the command then perform a path
	 * lookup and use the resolved path as the command to exec.  If there
	 * are no '/' characters or if the command wasn't found in the path,
	 * have exec attempt to invoke the command directly.
	 */
	if (!strchr(out->argv[1], '/')) {
		char *program = locate_in_PATH(out->argv[1]);
		if (program) {
			free((char *)out->argv[1]);
			out->argv[1] = program;
		}
	}
}