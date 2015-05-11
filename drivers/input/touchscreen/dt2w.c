/*
 * Copyright (c) 2015 Maxime Poulain. All rights reserved.
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

#define pr_fmt(fmt) "" fmt

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/kthread.h>
#include <linux/slab.h>
#include <linux/input.h>
#include <linux/time.h>
#include <linux/delay.h>

#define DT2W_TIMEOUT_US (650 * USEC_PER_MSEC)

static struct input_dev *dt2w_dev;
static DEFINE_MUTEX(dt2w_lock);
bool doubletap2wake = true;
static bool suspended = false;

static u64 now = 0;
static u64 last_input = 0;

static int counter = 0;

struct dt2w_inputopen {
	struct input_handle *handle;
	struct work_struct inputopen_work;
} dt2w_inputopen;

static void dt2w_presspwr(struct work_struct *dt2w_presspwr_work)
{
	dev_info(&dt2w_dev->dev, "sending press power to the input device\n");
	input_event(dt2w_dev, EV_KEY, KEY_POWER, 1);
	input_event(dt2w_dev, EV_SYN, 0, 0);
	msleep(100);
	input_event(dt2w_dev, EV_KEY, KEY_POWER, 0);
	input_event(dt2w_dev, EV_SYN, 0, 0);
	msleep(1000);
	mutex_unlock(&dt2w_lock);
}

static DECLARE_WORK(dt2w_presspwr_work, dt2w_presspwr);

void dt2w_pwrtrigger(void)
{
	if (mutex_trylock(&dt2w_lock))
	{
		schedule_work(&dt2w_presspwr_work);
	}
}

static void dt2w_input_event(struct input_handle *handle,
                unsigned int type, unsigned int code, int value)
{

	if (doubletap2wake && suspended && type == EV_ABS && code == ABS_MT_TRACKING_ID) {
		now = ktime_to_us(ktime_get());
		if (last_input + DT2W_TIMEOUT_US < now)
			counter = 0;
		counter++;
		if (counter > 1) {
				dt2w_pwrtrigger();
				counter = 0;
		}
		last_input = now;
	}
}

static void dt2w_input_open(struct work_struct *w)
{
	struct dt2w_inputopen *io = 
		container_of(w, struct dt2w_inputopen, inputopen_work);

	int error;

	error = input_open_device(io->handle);
	if (error)
		input_unregister_handle(io->handle);
}

static int dt2w_input_connect(struct input_handler *handler,
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

	dt2w_inputopen.handle = handle;
	schedule_work(&dt2w_inputopen.inputopen_work);

	return 0;

err:
	kfree(handle);
	return error;
}

static void dt2w_input_disconnect(struct input_handle *handle)
{
	flush_work(&dt2w_inputopen.inputopen_work);
	input_close_device(handle);
	input_unregister_handle(handle);
	kfree(handle);
}

static const struct input_device_id dt2w_ids[] = {
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

static struct input_handler dt2w_input_handler = {
	.event          = dt2w_input_event,
	.connect        = dt2w_input_connect,
	.disconnect     = dt2w_input_disconnect,
	.name           = "dt2w",
	.id_table       = dt2w_ids,
};

static ssize_t doubletap2wake_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", doubletap2wake);
}

static ssize_t doubletap2wake_store(struct device *dev,
				   struct device_attribute *attr,
				   const char *buf, size_t size)
{
	int ret;
	unsigned int value;

	ret = sscanf(buf, "%d\n", &value);

	if (ret != 1)
		return -EINVAL;
	else
		doubletap2wake = value ? true : false;

	return size;
}

static ssize_t suspended_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", suspended);
}

static ssize_t suspended_store(struct device *dev,
				   struct device_attribute *attr,
				   const char *buf, size_t size)
{
	int ret;
	unsigned int value;

	ret = sscanf(buf, "%d\n", &value);

	if (ret != 1)
		return -EINVAL;
	else
		suspended = value ? true : false;

	return size;
}

static DEVICE_ATTR(doubletap2wake, 0664,doubletap2wake_show, doubletap2wake_store);
static DEVICE_ATTR(suspended, 0664, suspended_show, suspended_store);

static struct kobject *dt2w_kobj;

static struct attribute *dt2w_attrs[] = {
	&dev_attr_doubletap2wake.attr,
	&dev_attr_suspended.attr,
	NULL
};

static const struct attribute_group dt2w_attr_group = {
	.attrs = dt2w_attrs,
};

static int init(void)
{

	pr_info("Registering DT2W driver\n");

	INIT_WORK(&dt2w_inputopen.inputopen_work, dt2w_input_open);

	dt2w_dev = input_allocate_device();
	if(!dt2w_dev) {
		printk(KERN_ERR "Not enough memory to allocate the input device\n");
		return -ENOMEM;
	}

	input_register_device(dt2w_dev);
	dt2w_dev->name = "Double tap driver";
	input_set_capability(dt2w_dev, EV_KEY, KEY_POWER);

	dt2w_kobj = kobject_create_and_add("android_touch", NULL);

	if (sysfs_create_group(dt2w_kobj, &dt2w_attr_group))
		dev_info(&dt2w_dev->dev,"[DT2W]:Unable to register the input device sysfs group\n");

	if (input_register_handler(&dt2w_input_handler))
		dev_info(&dt2w_dev->dev,"[DT2W]:Unable to register the input handler\n");

	return 0;
}
late_initcall(init);
