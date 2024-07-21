static int handle_mmio(struct kvm_vcpu *vcpu, struct kvm_run *kvm_run)
{
	struct kvm_mmio_req *p;
	struct kvm_io_device *mmio_dev;
	int r;

	p = kvm_get_vcpu_ioreq(vcpu);

	if ((p->addr & PAGE_MASK) == IOAPIC_DEFAULT_BASE_ADDRESS)
		goto mmio;
	vcpu->mmio_needed = 1;
	vcpu->mmio_phys_addr = kvm_run->mmio.phys_addr = p->addr;
	vcpu->mmio_size = kvm_run->mmio.len = p->size;
	vcpu->mmio_is_write = kvm_run->mmio.is_write = !p->dir;

	if (vcpu->mmio_is_write)
		memcpy(vcpu->mmio_data, &p->data, p->size);
	memcpy(kvm_run->mmio.data, &p->data, p->size);
	kvm_run->exit_reason = KVM_EXIT_MMIO;
	return 0;
mmio:
	if (p->dir)
		r = kvm_io_bus_read(vcpu->kvm, KVM_MMIO_BUS, p->addr,
				    p->size, &p->data);
	else
		r = kvm_io_bus_write(vcpu->kvm, KVM_MMIO_BUS, p->addr,
				     p->size, &p->data);
	if (r)
		printk(KERN_ERR"kvm: No iodevice found! addr:%lx\n", p->addr);
	p->state = STATE_IORESP_READY;

	return 1;
}