static void regulator_ena_gpio_free(struct regulator_dev *rdev)
{
	struct regulator_enable_gpio *pin, *n;

	if (!rdev->ena_pin)
		return;

	/* Free the GPIO only in case of no use */
	list_for_each_entry_safe(pin, n, &regulator_ena_gpio_list, list) {
		if (pin->gpiod == rdev->ena_pin->gpiod) {
			if (pin->request_count <= 1) {
				pin->request_count = 0;
				gpiod_put(pin->gpiod);
				list_del(&pin->list);
				kfree(pin);
			} else {
				pin->request_count--;
			}
		}
	}
}