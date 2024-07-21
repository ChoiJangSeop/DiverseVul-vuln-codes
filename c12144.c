static void ptirq_free_irte(const struct ptirq_remapping_info *entry)
{
	struct intr_source intr_src;

	if (entry->irte_idx < CONFIG_MAX_IR_ENTRIES) {
		if (entry->intr_type == PTDEV_INTR_MSI) {
			intr_src.is_msi = true;
			intr_src.src.msi.value = entry->phys_sid.msi_id.bdf;
		} else {
			intr_src.is_msi = false;
			intr_src.src.ioapic_id = ioapic_irq_to_ioapic_id(entry->allocated_pirq);
		}
		dmar_free_irte(&intr_src, entry->irte_idx);
	}
}