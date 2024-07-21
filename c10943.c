void kvm_lapic_reset(struct kvm_vcpu *vcpu, bool init_event)
{
	struct kvm_lapic *apic = vcpu->arch.apic;
	int i;

	if (!init_event) {
		vcpu->arch.apic_base = APIC_DEFAULT_PHYS_BASE |
				       MSR_IA32_APICBASE_ENABLE;
		if (kvm_vcpu_is_reset_bsp(vcpu))
			vcpu->arch.apic_base |= MSR_IA32_APICBASE_BSP;
	}

	if (!apic)
		return;

	/* Stop the timer in case it's a reset to an active apic */
	hrtimer_cancel(&apic->lapic_timer.timer);

	if (!init_event) {
		apic->base_address = APIC_DEFAULT_PHYS_BASE;

		kvm_apic_set_xapic_id(apic, vcpu->vcpu_id);
	}
	kvm_apic_set_version(apic->vcpu);

	for (i = 0; i < KVM_APIC_LVT_NUM; i++)
		kvm_lapic_set_reg(apic, APIC_LVTT + 0x10 * i, APIC_LVT_MASKED);
	apic_update_lvtt(apic);
	if (kvm_vcpu_is_reset_bsp(vcpu) &&
	    kvm_check_has_quirk(vcpu->kvm, KVM_X86_QUIRK_LINT0_REENABLED))
		kvm_lapic_set_reg(apic, APIC_LVT0,
			     SET_APIC_DELIVERY_MODE(0, APIC_MODE_EXTINT));
	apic_manage_nmi_watchdog(apic, kvm_lapic_get_reg(apic, APIC_LVT0));

	kvm_apic_set_dfr(apic, 0xffffffffU);
	apic_set_spiv(apic, 0xff);
	kvm_lapic_set_reg(apic, APIC_TASKPRI, 0);
	if (!apic_x2apic_mode(apic))
		kvm_apic_set_ldr(apic, 0);
	kvm_lapic_set_reg(apic, APIC_ESR, 0);
	kvm_lapic_set_reg(apic, APIC_ICR, 0);
	kvm_lapic_set_reg(apic, APIC_ICR2, 0);
	kvm_lapic_set_reg(apic, APIC_TDCR, 0);
	kvm_lapic_set_reg(apic, APIC_TMICT, 0);
	for (i = 0; i < 8; i++) {
		kvm_lapic_set_reg(apic, APIC_IRR + 0x10 * i, 0);
		kvm_lapic_set_reg(apic, APIC_ISR + 0x10 * i, 0);
		kvm_lapic_set_reg(apic, APIC_TMR + 0x10 * i, 0);
	}
	kvm_apic_update_apicv(vcpu);
	apic->highest_isr_cache = -1;
	update_divide_count(apic);
	atomic_set(&apic->lapic_timer.pending, 0);

	vcpu->arch.pv_eoi.msr_val = 0;
	apic_update_ppr(apic);
	if (vcpu->arch.apicv_active) {
		static_call(kvm_x86_apicv_post_state_restore)(vcpu);
		static_call(kvm_x86_hwapic_irr_update)(vcpu, -1);
		static_call(kvm_x86_hwapic_isr_update)(vcpu, -1);
	}

	vcpu->arch.apic_arb_prio = 0;
	vcpu->arch.apic_attention = 0;

	kvm_recalculate_apic_map(vcpu->kvm);
}