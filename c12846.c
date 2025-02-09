static irqreturn_t snd_msnd_interrupt(int irq, void *dev_id)
{
	struct snd_msnd *chip = dev_id;
	void *pwDSPQData = chip->mappedbase + DSPQ_DATA_BUFF;

	/* Send ack to DSP */
	/* inb(chip->io + HP_RXL); */

	/* Evaluate queued DSP messages */
	while (readw(chip->DSPQ + JQS_wTail) != readw(chip->DSPQ + JQS_wHead)) {
		u16 wTmp;

		snd_msnd_eval_dsp_msg(chip,
			readw(pwDSPQData + 2 * readw(chip->DSPQ + JQS_wHead)));

		wTmp = readw(chip->DSPQ + JQS_wHead) + 1;
		if (wTmp > readw(chip->DSPQ + JQS_wSize))
			writew(0, chip->DSPQ + JQS_wHead);
		else
			writew(wTmp, chip->DSPQ + JQS_wHead);
	}
	/* Send ack to DSP */
	inb(chip->io + HP_RXL);
	return IRQ_HANDLED;
}