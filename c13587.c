static int snd_ctl_elem_user_put(struct snd_kcontrol *kcontrol,
				 struct snd_ctl_elem_value *ucontrol)
{
	int change;
	struct user_element *ue = kcontrol->private_data;
	
	change = memcmp(&ucontrol->value, ue->elem_data, ue->elem_data_size) != 0;
	if (change)
		memcpy(ue->elem_data, &ucontrol->value, ue->elem_data_size);
	return change;
}