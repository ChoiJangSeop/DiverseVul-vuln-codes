static void handle_external_interrupt_irqoff(struct kvm_vcpu *vcpu)
{
	u32 intr_info = vmx_get_intr_info(vcpu);
	unsigned int vector = intr_info & INTR_INFO_VECTOR_MASK;
	gate_desc *desc = (gate_desc *)host_idt_base + vector;

	if (KVM_BUG(!is_external_intr(intr_info), vcpu->kvm,
	    "KVM: unexpected VM-Exit interrupt info: 0x%x", intr_info))
		return;

	handle_interrupt_nmi_irqoff(vcpu, gate_offset(desc));
}