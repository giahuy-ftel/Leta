#include "button.h"

uint8_t button_init(button_t *button, uint32_t u, uint8_t hw_state_pressed, pf_button_ctrl init, pf_button_read read, pf_button_callback callback)
{
	button->enable = BUTTON_DISABLE;
	button->counter = 0;
	button->unit = u;
	button->state = BUTTON_SW_STATE_RELEASED;
	button->counter_enable = BUTTON_ENABLE;
	button->hw_state_pressed = hw_state_pressed;

	button->init = init;
	button->read = read;
	button->callback = callback;

	if (button->init) {
		button->init();
	} else {
		return BUTTON_DRIVER_NG;
	}

	if (!button->read) {
		return BUTTON_DRIVER_NG;
	}

	if (!button->callback) {
		return BUTTON_DRIVER_NG;
	}

	return BUTTON_DRIVER_OK;
}

void button_enable(button_t *button)
{
	button->enable = BUTTON_ENABLE;
}

void button_disable(button_t *button)
{
	button->enable = BUTTON_DISABLE;
}

void button_timer_polling(button_t *button)
{
	uint8_t hw_button_state;

	if (button->enable == BUTTON_ENABLE) {
		hw_button_state = button->read();

		if (hw_button_state == button->hw_state_pressed) {
			if (button->counter_enable == BUTTON_ENABLE) {
				button->counter += button->unit;

				if (button->counter == BUTTON_LONG_PRESS_TIME &&
						button->state != BUTTON_SW_STATE_LONG_PRESSED) {
					button->enable = BUTTON_DISABLE;
					button->state = BUTTON_SW_STATE_LONG_PRESSED;
					button->callback(button);
					button->state = BUTTON_SW_STATE_PRESSED;
					button->enable = BUTTON_ENABLE;
				} else if (button->counter >= BUTTON_SHORT_PRESS_MIN_TIME &&
						button->state != BUTTON_SW_STATE_PRESSED) {
					button->enable = BUTTON_DISABLE;
					button->state = BUTTON_SW_STATE_PRESSED;
					button->callback(button);
					button->enable = BUTTON_ENABLE;
				}
			}
		} else {
			button->state = BUTTON_SW_STATE_RELEASED;

			if (button->counter > BUTTON_SHORT_PRESS_MIN_TIME) {
				button->enable = BUTTON_DISABLE;
				button->callback(button);
			}

			button->counter = 0;
			button->counter_enable = BUTTON_ENABLE;
			button->enable = BUTTON_ENABLE;
		}
	}
}
