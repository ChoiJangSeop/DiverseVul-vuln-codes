static int load_em86(struct linux_binprm *bprm)
{
	char *interp, *i_name, *i_arg;
	struct file * file;
	int retval;
	struct elfhdr	elf_ex;

	/* Make sure this is a Linux/Intel ELF executable... */
	elf_ex = *((struct elfhdr *)bprm->buf);

	if (memcmp(elf_ex.e_ident, ELFMAG, SELFMAG) != 0)
		return  -ENOEXEC;

	/* First of all, some simple consistency checks */
	if ((elf_ex.e_type != ET_EXEC && elf_ex.e_type != ET_DYN) ||
		(!((elf_ex.e_machine == EM_386) || (elf_ex.e_machine == EM_486))) ||
		(!bprm->file->f_op || !bprm->file->f_op->mmap)) {
			return -ENOEXEC;
	}

	bprm->recursion_depth++; /* Well, the bang-shell is implicit... */
	allow_write_access(bprm->file);
	fput(bprm->file);
	bprm->file = NULL;

	/* Unlike in the script case, we don't have to do any hairy
	 * parsing to find our interpreter... it's hardcoded!
	 */
	interp = EM86_INTERP;
	i_name = EM86_I_NAME;
	i_arg = NULL;		/* We reserve the right to add an arg later */

	/*
	 * Splice in (1) the interpreter's name for argv[0]
	 *           (2) (optional) argument to interpreter
	 *           (3) filename of emulated file (replace argv[0])
	 *
	 * This is done in reverse order, because of how the
	 * user environment and arguments are stored.
	 */
	remove_arg_zero(bprm);
	retval = copy_strings_kernel(1, &bprm->filename, bprm);
	if (retval < 0) return retval; 
	bprm->argc++;
	if (i_arg) {
		retval = copy_strings_kernel(1, &i_arg, bprm);
		if (retval < 0) return retval; 
		bprm->argc++;
	}
	retval = copy_strings_kernel(1, &i_name, bprm);
	if (retval < 0)	return retval;
	bprm->argc++;

	/*
	 * OK, now restart the process with the interpreter's inode.
	 * Note that we use open_exec() as the name is now in kernel
	 * space, and we don't need to copy it.
	 */
	file = open_exec(interp);
	if (IS_ERR(file))
		return PTR_ERR(file);

	bprm->file = file;

	retval = prepare_binprm(bprm);
	if (retval < 0)
		return retval;

	return search_binary_handler(bprm);
}