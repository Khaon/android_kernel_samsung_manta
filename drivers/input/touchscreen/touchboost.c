/*
 * Copyright (c) 2013-2014, Francisco Franco. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#define pr_fmt(fmt) "cpu-boost: " fmt

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/notifier.h>
#include <linux/cpufreq.h>
#include <linux/sched.h>
#include <linux/jiffies.h>
#include <linux/kthread.h>
#include <linux/moduleparam.h>
#include <linux/slab.h>
#include <linux/input.h>
#include <linux/time.h>

#define MIN_TIME_INTERVAL_US (150 * USEC_PER_MSEC)

/*
 * Use this variable in your governor of choice to calculate when the cpufreq
 * core is allowed to ramp the cpu down after an input event. That logic is done
 * by you, this var only outputs the last time in us an event was captured
 */
u64 last_input_time;

inline u64 get_input_time(void)
{
	return last_input_time;
}

struct touchboost_inputopen {
	struct input_handle *handle;
	struct work_struct inputopen_work;
} touchboost_inputopen;

static void boost_input_event(struct input_handle *handle,
                unsigned int type, unsigned int code, int value)
{
	u64 now;

	if (type == EV_ABS) {
		now = ktime_to_us(ktime_get());
		
		if (now - last_input_time < MIN_TIME_INTERVAL_US)
			return;

		last_input_time = ktime_to_us(ktime_get());
	}
}

static void boost_input_open(struct work_struct *w)
{
	struct touchboost_inputopen *io = 
		container_of(w, struct touchboost_inputopen, inputopen_work);

	int error;

	error = input_open_device(io->handle);
	if (error)
		input_unregister_handle(io->handle);
}

static int boost_input_connect(struct input_handler *handler,
                struct input_dev *dev, const struct input_device_id *id)
{
	struct input_handle *handle;
	int error;

	handle = kzalloc(sizeof(*handle), GFP_KERNEL);
	if (handle == NULL)
		return -ENOMEM;

	handle->dev = dev;
	handle->handler = handler;
	handle->name = handler->name;

	error = input_register_handle(handle);
	if (error)
		goto err;

	touchboost_inputopen.handle = handle;
	schedule_work(&touchboost_inputopen.inputopen_work);

	return 0;

err:
	kfree(handle);
	return error;
}

static void boost_input_disconnect(struct input_handle *handle)
{
	flush_work(&touchboost_inputopen.inputopen_work);
	input_close_device(handle);
	input_unregister_handle(handle);
	kfree(handle);
}

static const struct input_device_id boost_ids[] = {
	{
		.flags = INPUT_DEVICE_ID_MATCH_EVBIT,
		.evbit = { BIT_MASK(EV_ABS) },
		/* assumption: MT_.._X & MT_.._Y are in the same long */
		.absbit = { [BIT_WORD(ABS_MT_POSITION_X)] =
				BIT_MASK(ABS_MT_POSITION_X) |
				BIT_MASK(ABS_MT_POSITION_Y) },
	},
	{ },
};

static struct input_handler boost_input_handler = {
	.event          = boost_input_event,
	.connect        = boost_input_connect,
	.disconnect     = boost_input_disconnect,
	.name           = "input-boost",
	.id_table       = boost_ids,
};

static int init(void)
{
	
	pr_info("Registering input touchboost driver\n");

	INIT_WORK(&touchboost_inputopen.inputopen_work, boost_input_open);

	if (input_register_handler(&boost_input_handler))
		pr_info("Unable to register the input handler\n");

	return 0;
}
late_initcall(init);
