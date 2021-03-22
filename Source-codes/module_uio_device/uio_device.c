/*
 * This module creates a user input/output device: https://www.kernel.org/doc/html/v4.13/driver-api/uio-howto.html
 * This type of device gives the possibility to handle a fake or virtual interrupt on the device created.
 * In fact, real interrupts are only handled on real devices.
 * To compile the module and the user app type: make 
 * After what you can insert it by doing: sudo insmod uio_device.ko 
 * Before inserting an uio device, you have to load the uio module: sudo modprobe uio
 * If everything is ok, this will create the device at /dev/uio0
 * You can then launch the user app with: sudo ./uio_user "/dev/uio0"
 */

/* Files to include for module primitives. */
#include <linux/module.h>
/* Other files here. */

/* 
 * Files to include for timer management (as we dealing with a fake device, we cannot have real interrupts) 
 * So to simulate it, we'll periodically and manually generate a "virtual/fake" one.
 */
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/timer.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("TP8");
MODULE_DESCRIPTION("UIO driver");

static struct uio_info* info; //structure to create and register a new uio device
static struct platform_device *pdev; //the device to be registered
static struct timer_list intr_timer;

static int msec_unreg_delay = 3000; // 3 secs
static int msec_uio_intr_delay = 2000; // 1 sec

/* When releasing the module, method to unregister the uio device. */
void unregister_uio(void)
{
    pr_info("Deleting fake interrupt timer\n");
    /* Code to delete the timer here */

    /* Unregister the device and free the uio_info variable */
    if (info) {
        /*
         * Code
         */
    }
}

/* 
 * The interrupt callback function 
 * After each "intr_timer" seconds, the callback will be called to send interrupt signal to unblock the read from the user space.
 */
void intr_timer_cb(struct timer_list  *timer)
{
    pr_info("Firing fake interrupt\n");

    /* Notify the device by sending interrupt. */
    /* .. Code .. */

    /* Setup timer to fire again */
    mod_timer(&intr_timer,
               jiffies + msecs_to_jiffies(msec_uio_intr_delay));
}

/*
 * The uio_struct has some functions that must be initialized, even if we just add some print in their implementatio.
 */

/* Function called when the device is opened : when the user will do open() on it. */
static int uio_device_open(struct uio_info *info, struct inode *inode)
{
    pr_info(/*...*/);
    return 0;
}

/* Function called when the device is closed. */
static int uio_device_release(struct uio_info *info, struct inode *inode)
{
    pr_info(/*...*/);
    return 0;
}


/* The init method called on insmod. */
static int __init uio_device_init(void)
{
    /* Create and register a device. */
    pdev = /* ... */;

    if (IS_ERR(pdev)) {
        /*...*/
    }

    /* Allocate memory for the device */
    info = /*...*/; //use kzalloc
    
    if (!info)
        return -ENOMEM;

    /* Initialize the fields. */
    info->name = "uio_driver";
    /*
     * .... - You can look at the document (follow the link at the header).
     */
    info->release = uio_device_release;
	
    /* Register the new userspace IO device -- @info: UIO device capabilities */
    /*... code here ...*/

    /* Set up the timer */
    timer_setup(&intr_timer, intr_timer_cb, 0 );
    if (mod_timer(&intr_timer, (jiffies +
                                     msecs_to_jiffies(msec_uio_intr_delay)))) {
        pr_err("Error setting up interrupt timer");
        goto devmem;
    }

    pr_info("UIO device loaded\n");
    return 0;
}

/* The clean method called on rmmod. */
static void __exit uio_device_cleanup(void)
{
    pr_info("Cleaning up module.\n");

    /* Delete the timer. */
    del_timer_sync(&intr_timer);

    unregister_uio();

    /* Unregister the plateform device. */
    if (pdev)
        platform_device_unregister(pdev);
}

module_init(uio_device_init); //Specify who is the init.
module_exit(uio_device_cleanup); //Specify who is the exit.
