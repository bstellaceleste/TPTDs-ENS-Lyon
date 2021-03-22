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

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/uio_driver.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/types.h>
#include <linux/mm.h>

#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/timer.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("TP8");
MODULE_DESCRIPTION("UIO driver");

static struct uio_info* info;
static struct platform_device *pdev;
static struct timer_list intr_timer;

static int msec_uio_intr_delay = 2000; // 1 sec

void *shared_mem_buffer = NULL;

/* When releasing the module, method to unregister the uio device. */
void unregister_uio(void)
{
    pr_info("Deleting fake interrupt timer\n");
    del_timer_sync(&intr_timer);
    if (info) {
        pr_info("Unregistering uio\n");
        uio_unregister_device(info);
        kfree(info);
        info = 0;
    }
}

/* The interrupt callback function 
 * After each "intr_timer" seconds, the callback will be called to send interrupt signal to unblock the read from the user space.
 */
void intr_timer_cb(struct timer_list  *timer)
{
    pr_info("Firing fake interrupt\n");
    uio_event_notify(info); //send interrupt to the device.
    // Setup timer to fire again
    mod_timer(&intr_timer,
               jiffies + msecs_to_jiffies(msec_uio_intr_delay));
}

static int uio_device_open(struct uio_info *info, struct inode *inode)
{
    pr_info("%s called\n", __FUNCTION__);
    return 0;
}

static int uio_device_release(struct uio_info *info, struct inode *inode)
{
    pr_info("%s called\n", __FUNCTION__);
    return 0;
}

int uio_vtf_hotplug_mmap(struct uio_info *info, struct vm_area_struct *vma){
    unsigned long len, pfn;
    int ret ;

    len = vma->vm_end - vma->vm_start;
    pfn = virt_to_phys((void *)shared_mem_buffer)>>PAGE_SHIFT;

    ret = remap_pfn_range(vma, vma->vm_start, pfn, len, vma->vm_page_prot);
    if (ret < 0) {
        pr_err("could not map the address area\n");
        kfree(shared_mem_buffer);
        return -EIO;
    }

    pr_info("memory map called success \n");

    return ret;
}

static int __init uio_device_init(void)
{

    pdev = platform_device_register_simple("uio_device",
                                            0, NULL, 0);
    if (IS_ERR(pdev)) {
        pr_err("Failed to register platform device.\n");
        return -EINVAL;
    }

    info = kzalloc(sizeof(struct uio_info), GFP_KERNEL);
    
    if (!info)
        return -ENOMEM;

    shared_mem_buffer = (void *) kzalloc(PAGE_SIZE, GFP_ATOMIC);

    info->name = "uio_driver";
    info->version = "0.1";
    info->mem[0].addr = (phys_addr_t) shared_mem_buffer;// Allocate memory that can be accessed by the user
    if (!info->mem[0].addr)
        goto uiomem;
    info->mem[0].memtype = UIO_MEM_LOGICAL;
    info->mem[0].size = PAGE_SIZE;
    info->irq = UIO_IRQ_CUSTOM;
    info->handler = 0;
    info->open = uio_device_open;
    info->release = uio_device_release;
	
    if(uio_register_device(&pdev->dev, info)) {
        pr_err("Unable to register UIO device!\n");
        goto devmem;
    }

    timer_setup(&intr_timer, intr_timer_cb, 0 );
    if (mod_timer(&intr_timer, (jiffies +
                                     msecs_to_jiffies(msec_uio_intr_delay)))) {
        pr_err("Error setting up interrupt timer");
        goto devmem;
    }

    pr_info("UIO device loaded\n");
    return 0;

devmem:
    kfree((void *)info->mem[0].addr);
uiomem:
    kfree(info);
    
    return -ENODEV;
}

static void __exit uio_device_cleanup(void)
{
   pr_info("Cleaning up module.\n");
        
   del_timer_sync(&intr_timer);

   unregister_uio();

   if (pdev)
        platform_device_unregister(pdev);
}

module_init(uio_device_init);
module_exit(uio_device_cleanup);
