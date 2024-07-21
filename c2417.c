prepare_for_client_read(void)
{
	if (DoingCommandRead)
	{
		/* Enable immediate processing of asynchronous signals */
		EnableNotifyInterrupt();
		EnableCatchupInterrupt();

		/* Allow cancel/die interrupts to be processed while waiting */
		ImmediateInterruptOK = true;

		/* And don't forget to detect one that already arrived */
		CHECK_FOR_INTERRUPTS();
	}
}