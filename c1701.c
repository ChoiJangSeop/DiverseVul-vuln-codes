static int write_exit_mmio(struct kvm_vcpu *vcpu, gpa_t gpa,
			   void *val, int bytes)
{
	memcpy(vcpu->mmio_data, val, bytes);
	memcpy(vcpu->run->mmio.data, vcpu->mmio_data, 8);
	return X86EMUL_CONTINUE;
}