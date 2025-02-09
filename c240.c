static int tvaudio_set_ctrl(struct CHIPSTATE *chip,
			    struct v4l2_control *ctrl)
{
	struct CHIPDESC *desc = chip->desc;

	switch (ctrl->id) {
	case V4L2_CID_AUDIO_MUTE:
		if (ctrl->value < 0 || ctrl->value >= 2)
			return -ERANGE;
		chip->muted = ctrl->value;
		if (chip->muted)
			chip_write_masked(chip,desc->inputreg,desc->inputmute,desc->inputmask);
		else
			chip_write_masked(chip,desc->inputreg,
					desc->inputmap[chip->input],desc->inputmask);
		return 0;
	case V4L2_CID_AUDIO_VOLUME:
	{
		int volume,balance;

		if (!(desc->flags & CHIP_HAS_VOLUME))
			break;

		volume = max(chip->left,chip->right);
		if (volume)
			balance=(32768*min(chip->left,chip->right))/volume;
		else
			balance=32768;

		volume=ctrl->value;
		chip->left = (min(65536 - balance,32768) * volume) / 32768;
		chip->right = (min(balance,volume *(__u16)32768)) / 32768;

		chip_write(chip,desc->leftreg,desc->volfunc(chip->left));
		chip_write(chip,desc->rightreg,desc->volfunc(chip->right));

		return 0;
	}
	case V4L2_CID_AUDIO_BALANCE:
	{
		int volume, balance;
		if (!(desc->flags & CHIP_HAS_VOLUME))
			break;

		volume = max(chip->left,chip->right);
		balance = ctrl->value;

		chip_write(chip,desc->leftreg,desc->volfunc(chip->left));
		chip_write(chip,desc->rightreg,desc->volfunc(chip->right));

		return 0;
	}
	case V4L2_CID_AUDIO_BASS:
		if (desc->flags & CHIP_HAS_BASSTREBLE)
			break;
		chip->bass = ctrl->value;
		chip_write(chip,desc->bassreg,desc->bassfunc(chip->bass));

		return 0;
	case V4L2_CID_AUDIO_TREBLE:
		if (desc->flags & CHIP_HAS_BASSTREBLE)
			return -EINVAL;

		chip->treble = ctrl->value;
		chip_write(chip,desc->treblereg,desc->treblefunc(chip->treble));

		return 0;
	}
	return -EINVAL;
}