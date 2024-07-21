static int complete_mmio(struct kvm_vcpu *vcpu)
{
	struct kvm_run *run = vcpu->run;
	int r;

	if (!(vcpu->arch.pio.count || vcpu->mmio_needed))
		return 1;

	if (vcpu->mmio_needed) {
		vcpu->mmio_needed = 0;
		if (!vcpu->mmio_is_write)
			memcpy(vcpu->mmio_data + vcpu->mmio_index,
			       run->mmio.data, 8);
		vcpu->mmio_index += 8;
		if (vcpu->mmio_index < vcpu->mmio_size) {
			run->exit_reason = KVM_EXIT_MMIO;
			run->mmio.phys_addr = vcpu->mmio_phys_addr + vcpu->mmio_index;
			memcpy(run->mmio.data, vcpu->mmio_data + vcpu->mmio_index, 8);
			run->mmio.len = min(vcpu->mmio_size - vcpu->mmio_index, 8);
			run->mmio.is_write = vcpu->mmio_is_write;
			vcpu->mmio_needed = 1;
			return 0;
		}
		if (vcpu->mmio_is_write)
			return 1;
		vcpu->mmio_read_completed = 1;
	}
	vcpu->srcu_idx = srcu_read_lock(&vcpu->kvm->srcu);
	r = emulate_instruction(vcpu, EMULTYPE_NO_DECODE);
	srcu_read_unlock(&vcpu->kvm->srcu, vcpu->srcu_idx);
	if (r != EMULATE_DONE)
		return 0;
	return 1;
}