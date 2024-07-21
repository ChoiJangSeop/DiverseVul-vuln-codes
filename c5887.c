static void i8042_stop(struct serio *serio)
{
	struct i8042_port *port = serio->port_data;

	port->exists = false;

	/*
	 * We synchronize with both AUX and KBD IRQs because there is
	 * a (very unlikely) chance that AUX IRQ is raised for KBD port
	 * and vice versa.
	 */
	synchronize_irq(I8042_AUX_IRQ);
	synchronize_irq(I8042_KBD_IRQ);
	port->serio = NULL;
}