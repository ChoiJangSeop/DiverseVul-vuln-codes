struct bpf_prog *bpf_prog_get(u32 ufd)
{
	struct fd f = fdget(ufd);
	struct bpf_prog *prog;

	prog = __bpf_prog_get(f);
	if (IS_ERR(prog))
		return prog;

	atomic_inc(&prog->aux->refcnt);
	fdput(f);

	return prog;
}