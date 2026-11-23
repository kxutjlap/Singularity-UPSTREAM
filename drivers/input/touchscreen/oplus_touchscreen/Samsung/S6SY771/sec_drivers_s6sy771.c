static int sec_reset(void *chip_data)
{
	int ret = -1;
	struct chip_data_s6sy771 *chip_info =
		(struct chip_data_s6sy771 *)chip_data;

	TPD_INFO("%s is called\n", __func__);
	disable_irq_nosync(chip_info->client->irq);

	if (chip_info->is_power_down) {
		TPD_INFO("controller was power-down: staged resume reset\n");

		if (gpio_is_valid(chip_info->hw_res->reset_gpio))
			gpio_direction_output(chip_info->hw_res->reset_gpio, false);

		ret = tp_powercontrol_1v8(chip_info->hw_res, true);
		if (ret)
			goto reset_out;
		sec_mdelay(5);

		ret = tp_powercontrol_2v8(chip_info->hw_res, true);
		if (ret)
			goto reset_out;
		sec_mdelay(10);

		if (gpio_is_valid(chip_info->hw_res->reset_gpio))
			gpio_direction_output(chip_info->hw_res->reset_gpio, true);

		chip_info->is_power_down = false;
	} else if (gpio_is_valid(chip_info->hw_res->reset_gpio)) {
		TPD_INFO("reset by pull down rst pin");
		gpio_direction_output(chip_info->hw_res->reset_gpio, false);
		sec_mdelay(5);
		gpio_direction_output(chip_info->hw_res->reset_gpio, true);
	} else {
		touch_i2c_write_block(chip_info->client, SEC_CMD_SOFT_RESET, 0,
					      NULL);
	}

	sec_mdelay(RESET_TO_NORMAL_TIME);
	ret = sec_wait_for_ready(chip_info, SEC_ACK_BOOT_COMPLETE);
	if (ret < 0) {
		TPD_INFO("%s: BOOT_COMPLETE ack timeout/error\n", __func__);
		goto reset_out;
	}

	ret = touch_i2c_write_block(chip_info->client, SEC_CMD_SENSE_ON, 0,
				    NULL);
	if (ret < 0) {
		TPD_INFO("%s: write sense on failed\n", __func__);
		goto reset_out;
	}

	sec_refresh_switch(chip_info->display_refresh_rate ?
			   chip_info->display_refresh_rate : 90);

reset_out:
	enable_irq(chip_info->client->irq);
	if (ret < 0)
		return ret;

	return 0;
}
