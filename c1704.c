int emulator_read_write(struct x86_emulate_ctxt *ctxt, unsigned long addr,
			void *val, unsigned int bytes,
			struct x86_exception *exception,
			struct read_write_emulator_ops *ops)
{
	struct kvm_vcpu *vcpu = emul_to_vcpu(ctxt);

	/* Crossing a page boundary? */
	if (((addr + bytes - 1) ^ addr) & PAGE_MASK) {
		int rc, now;

		now = -addr & ~PAGE_MASK;
		rc = emulator_read_write_onepage(addr, val, now, exception,
						 vcpu, ops);

		if (rc != X86EMUL_CONTINUE)
			return rc;
		addr += now;
		val += now;
		bytes -= now;
	}

	return emulator_read_write_onepage(addr, val, bytes, exception,
					   vcpu, ops);
}