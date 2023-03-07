/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>


/* 1000 msec = 1 sec */
#define SLEEP_TIME_MS   1000
#define LED_ON_TIME_MS 1000
#define DEC_ON_TIME_MS 100 
#define INC_ON_TIME_MS 100

static int TOTAL_TIME = 1000;
static int temp2 = 1;

int curr_Time = 1;

int temp = 0;

static bool sleepCondition = 1;
static bool resetCondition = 1;
/* The devicetree node identifier for the "led0" alias. */
/*
 * A build error on this line means your board is unsupported.
 * See the sample documentation for information on how to fix this.
 */

/* LEDs */
static const struct gpio_dt_spec heartbeat_led  = GPIO_DT_SPEC_GET(DT_ALIAS(heartbeat), gpios);
static const struct gpio_dt_spec buzzer_led  = GPIO_DT_SPEC_GET(DT_ALIAS(buzzer), gpios);
static const struct gpio_dt_spec ivdrip_led  = GPIO_DT_SPEC_GET(DT_ALIAS(ivdrip), gpios);
static const struct gpio_dt_spec alarm_led  = GPIO_DT_SPEC_GET(DT_ALIAS(alarmled), gpios);
static const struct gpio_dt_spec error_led  = GPIO_DT_SPEC_GET(DT_ALIAS(error), gpios);

/* Buttons */
static const struct gpio_dt_spec sleep = GPIO_DT_SPEC_GET(DT_ALIAS(button0), gpios);
static const struct gpio_dt_spec freq_up = GPIO_DT_SPEC_GET(DT_ALIAS(button1), gpios);
static const struct gpio_dt_spec freq_down = GPIO_DT_SPEC_GET(DT_ALIAS(button2), gpios);
static const struct gpio_dt_spec reset = GPIO_DT_SPEC_GET(DT_ALIAS(button3), gpios);

static struct gpio_callback sleep_cb;
static struct gpio_callback freq_up_cb;
static struct gpio_callback freq_down_cb;
static struct gpio_callback reset_cb;

/* Declarations */
void sleep_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins);
void freq_up_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins);
void freq_down_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins);
void reset_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins);
void heartbeat_toggle(struct k_timer *heartbeat_timer);
void leds_toggle(struct k_timer *leds_timer);
void leds_toggle_stop(struct k_timer *leds_timer);


/* Timer */
K_TIMER_DEFINE(heartbeat_timer, heartbeat_toggle, NULL);
K_TIMER_DEFINE(leds_timer, leds_toggle,leds_toggle_stop);

void heartbeat_toggle(struct k_timer *heartbeat_timer)
{
    gpio_pin_toggle_dt(&heartbeat_led);

}

void leds_toggle(struct k_timer *leds_timer)
{	
	if (resetCondition){
		if (TOTAL_TIME < 100 | TOTAL_TIME > 2000){
			//	LOG_ERR("limit reached");
			gpio_pin_toggle_dt(&error_led);
			// k_timer_stop(&leds_timer);
			gpio_pin_set_dt(&buzzer_led, 0);
			gpio_pin_set_dt(&alarm_led, 0);
			gpio_pin_set_dt(&ivdrip_led, 0);

			resetCondition = 0;
		}
		else {
			if (temp2 == 1){
			gpio_pin_set_dt(&buzzer_led,1);
			gpio_pin_set_dt(&alarm_led,0);
			gpio_pin_set_dt(&ivdrip_led,0);
			temp2 = 2;
			}
			else if (temp2 == 2)
			{
			gpio_pin_set_dt(&buzzer_led,0);
			gpio_pin_set_dt(&alarm_led,1);
			gpio_pin_set_dt(&ivdrip_led,0);
			temp2 = 3;
			}
			else if (temp2 == 3)
			{
			gpio_pin_set_dt(&buzzer_led,0);
			gpio_pin_set_dt(&alarm_led,0);
			gpio_pin_set_dt(&ivdrip_led,1);
			temp2 = 1;
			}
		}		
	}
}

void leds_toggle_stop(struct k_timer *leds_timer){
	gpio_pin_set_dt(&buzzer_led, 0);
	gpio_pin_set_dt(&alarm_led, 0);
	gpio_pin_set_dt(&ivdrip_led, 0);

}

void sleep_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	curr_Time = TOTAL_TIME;
	TOTAL_TIME = temp;
	temp = curr_Time;
	sleepCondition = !sleepCondition;
	if(sleepCondition){
		k_timer_start(&leds_timer, K_MSEC(TOTAL_TIME), K_MSEC(TOTAL_TIME));
	}
	else{
		k_timer_stop(&leds_timer);
	}
}

void freq_up_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	TOTAL_TIME = TOTAL_TIME - DEC_ON_TIME_MS;
	printk("%d\n",TOTAL_TIME);
	k_timer_start(&leds_timer, K_MSEC(TOTAL_TIME), K_MSEC(TOTAL_TIME));
}

void freq_down_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	TOTAL_TIME = TOTAL_TIME + INC_ON_TIME_MS;
	printk("%d\n",TOTAL_TIME);
	k_timer_start(&leds_timer, K_MSEC(TOTAL_TIME), K_MSEC(TOTAL_TIME));
}

void reset_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	TOTAL_TIME = 1000;
	resetCondition = 1;
	k_timer_start(&leds_timer, K_MSEC(TOTAL_TIME), K_MSEC(TOTAL_TIME));
}
 

void main(void)
{
	// check if interfaces are ready
	int err;

	//err = check_interfaces_ready();
	//if (err) {
	//	LOG_ERR("Device interfaces not ready (err = %d)", err);
	//}

	if (!device_is_ready(heartbeat_led.port)) {
		//LOG_ERR("gpio0 interface not ready.");
		return;
	}

	if (!device_is_ready(buzzer_led.port)) {
		//LOG_ERR("gpio0 interface not ready.");
		return;
	}

	if (!device_is_ready(ivdrip_led.port)) {
		//LOG_ERR("gpio0 interface not ready.");
		return;
	}

	if (!device_is_ready(alarm_led.port)) {
		//LOG_ERR("gpio0 interface not ready.");
		return;
	}

	if (!device_is_ready(error_led.port)) {
		//LOG_ERR("gpio0 interface not ready.");
		return;
	}

	if (!device_is_ready(sleep.port)) {
		//LOG_ERR("gpio0 interface not ready.");
		return;
	}

	if (!device_is_ready(freq_up.port)) {
		//LOG_ERR("gpio0 interface not ready.");
		return;
	}

	if (!device_is_ready(freq_down.port)) {
		//LOG_ERR("gpio0 interface not ready.");
		return;
	}

	if (!device_is_ready(reset.port)) {
		//LOG_ERR("gpio0 interface not ready.");
		return;
	}
	
	err = gpio_pin_configure_dt(&heartbeat_led, GPIO_OUTPUT_ACTIVE);
	err = gpio_pin_configure_dt(&buzzer_led, GPIO_OUTPUT_ACTIVE);
	err = gpio_pin_configure_dt(&alarm_led, GPIO_OUTPUT_ACTIVE);
	err = gpio_pin_configure_dt(&ivdrip_led, GPIO_OUTPUT_ACTIVE);

	err = gpio_pin_configure_dt(&sleep, GPIO_INPUT);
	err = gpio_pin_configure_dt(&freq_up, GPIO_INPUT);
	err = gpio_pin_configure_dt(&freq_down,GPIO_INPUT);
	err = gpio_pin_configure_dt(&reset, GPIO_INPUT);


	// set call backs

	err = gpio_pin_interrupt_configure_dt(&sleep, GPIO_INT_EDGE_TO_ACTIVE);		
	gpio_init_callback(&sleep_cb, sleep_callback, BIT(sleep.pin));
	gpio_add_callback(sleep.port, &sleep_cb);

	err = gpio_pin_interrupt_configure_dt(&freq_up, GPIO_INT_EDGE_TO_ACTIVE);		
	gpio_init_callback(&freq_up_cb, freq_up_callback, BIT(freq_up.pin));
	gpio_add_callback(freq_up.port, &freq_up_cb);

	err = gpio_pin_interrupt_configure_dt(&freq_down, GPIO_INT_EDGE_TO_ACTIVE);		
	gpio_init_callback(&freq_down_cb, freq_down_callback, BIT(freq_down.pin));
	gpio_add_callback(freq_down.port, &freq_down_cb);

	err = gpio_pin_interrupt_configure_dt(&reset, GPIO_INT_EDGE_TO_ACTIVE);		
	gpio_init_callback(&reset_cb, reset_callback, BIT(reset.pin));
	gpio_add_callback(reset.port, &reset_cb);

	k_timer_start(&heartbeat_timer, K_MSEC(LED_ON_TIME_MS), K_MSEC(LED_ON_TIME_MS));

	int ret4;
	ret4 = gpio_pin_configure_dt(&error_led, GPIO_OUTPUT_INACTIVE);

	k_timer_start(&leds_timer, K_MSEC(TOTAL_TIME),K_MSEC(TOTAL_TIME));
}