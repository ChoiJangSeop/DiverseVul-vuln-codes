static int emulator_read_write_onepage(unsigned long addr, void *val,
				       unsigned int bytes,
				       struct x86_exception *exception,
				       struct kvm_vcpu *vcpu,
				       struct read_write_emulator_ops *ops)
{
	gpa_t gpa;
	int handled, ret;
	bool write = ops->write;

	if (ops->read_write_prepare &&
		  ops->read_write_prepare(vcpu, val, bytes))
		return X86EMUL_CONTINUE;

	ret = vcpu_mmio_gva_to_gpa(vcpu, addr, &gpa, exception, write);

	if (ret < 0)
		return X86EMUL_PROPAGATE_FAULT;

	/* For APIC access vmexit */
	if (ret)
		goto mmio;

	if (ops->read_write_emulate(vcpu, gpa, val, bytes))
		return X86EMUL_CONTINUE;

mmio:
	/*
	 * Is this MMIO handled locally?
	 */
	handled = ops->read_write_mmio(vcpu, gpa, bytes, val);
	if (handled == bytes)
		return X86EMUL_CONTINUE;

	gpa += handled;
	bytes -= handled;
	val += handled;

	vcpu->mmio_needed = 1;
	vcpu->run->exit_reason = KVM_EXIT_MMIO;
	vcpu->run->mmio.phys_addr = vcpu->mmio_phys_addr = gpa;
	vcpu->mmio_size = bytes;
	vcpu->run->mmio.len = min(vcpu->mmio_size, 8);
	vcpu->run->mmio.is_write = vcpu->mmio_is_write = write;
	vcpu->mmio_index = 0;

	return ops->read_write_exit_mmio(vcpu, gpa, val, bytes);
}