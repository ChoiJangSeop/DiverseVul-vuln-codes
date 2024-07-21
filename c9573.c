static void handle_irq_for_port(evtchn_port_t port)
{
	int irq;

	irq = get_evtchn_to_irq(port);
	if (irq != -1)
		generic_handle_irq(irq);
}