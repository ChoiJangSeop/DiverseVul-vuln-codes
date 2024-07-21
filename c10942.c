int kvm_create_lapic(struct kvm_vcpu *vcpu, int timer_advance_ns)
{
	struct kvm_lapic *apic;

	ASSERT(vcpu != NULL);

	apic = kzalloc(sizeof(*apic), GFP_KERNEL_ACCOUNT);
	if (!apic)
		goto nomem;

	vcpu->arch.apic = apic;

	apic->regs = (void *)get_zeroed_page(GFP_KERNEL_ACCOUNT);
	if (!apic->regs) {
		printk(KERN_ERR "malloc apic regs error for vcpu %x\n",
		       vcpu->vcpu_id);
		goto nomem_free_apic;
	}
	apic->vcpu = vcpu;

	hrtimer_init(&apic->lapic_timer.timer, CLOCK_MONOTONIC,
		     HRTIMER_MODE_ABS_HARD);
	apic->lapic_timer.timer.function = apic_timer_fn;
	if (timer_advance_ns == -1) {
		apic->lapic_timer.timer_advance_ns = LAPIC_TIMER_ADVANCE_NS_INIT;
		lapic_timer_advance_dynamic = true;
	} else {
		apic->lapic_timer.timer_advance_ns = timer_advance_ns;
		lapic_timer_advance_dynamic = false;
	}

	static_branch_inc(&apic_sw_disabled.key); /* sw disabled at reset */
	kvm_iodevice_init(&apic->dev, &apic_mmio_ops);

	return 0;
nomem_free_apic:
	kfree(apic);
	vcpu->arch.apic = NULL;
nomem:
	return -ENOMEM;
}