/*
 * OHCI HCD (Host Controller Driver) for USB.
 *
 * (C) Copyright 1999 Roman Weissgaerber <weissg@vienna.at>
 * (C) Copyright 2000-2002 David Brownell <dbrownell@users.sourceforge.net>
 * (C) Copyright 2002 Hewlett-Packard Company
 *
 * Bus Glue for Sharp hisilicon
 *
 * Written by Christopher Hoover <ch@hpl.hp.com>
 * Based on fragments of previous driver by Rusell King et al.
 *
 * Modified for hisilicon from ohci-sa111.c
 *  by Durgesh Pattamatta <pattamattad@sharpsec.com>
 *
 * This file is licenced under the GPL.
 */

#include <asm/hardware.h>
#include <asm/hardware/clock.h>


extern int usb_disabled(void);

static unsigned int  pSysClkAddrBase;

/* for clk enable and disable */
#define SYS_CONTROL                       0x101e0000
#define SCPEREN                           0x24
#define SCPERDIS                          0x28
#define SCPER_FREQ_FACTOR                 0x34
#define USB_CLOCK_ENABLE                  0x200000
#define USB_CLOCK_DISABLE                 0x200000
#define USB_CLOCK_FACTOR_13               13
#define USB_CLOCK_FACTOR_10               10
#define USB_CLOCK_FACTOR_11               11
#define USB_CLOCK_FACTOR_12               12
#define USB_CLOCK_FACTOR_13_MASK                0x0
#define USB_CLOCK_FACTOR_10_MASK               0x100
#define USB_CLOCK_FACTOR_11_MASK               0x200
#define USB_CLOCK_FACTOR_12_MASK               0x300

#define SCPRESET                          0x1c
#define USB_RESET_DISFROCK                0x00400000
#define USB_SAMPLE_FREQ                    48000000

#define OSDRV_MODULE_VERSION_STRING "USB1_1-M0001C030002 @Hi3511v110_OSDrv_1_0_0_7 2009-03-18 20:52:41"

/*-------------------------------------------------------------------------*/

static void hisilicon_start_hc(struct platform_device *dev)
{

	unsigned char* pSysBase = NULL;
	unsigned int tmp;
	unsigned int ahb_clock=0;
        unsigned int divider_factor=0,clock_divider=0;
	struct clk *clk=NULL;
	pSysClkAddrBase = IO_ADDRESS(SYS_CONTROL);
	
	printk(KERN_DEBUG __FILE__
	       ": starting hisilicon OHCI USB Controller\n");
    pSysBase = (unsigned char*)pSysClkAddrBase;	       
	*((volatile unsigned int*)(pSysBase + SCPEREN)) = USB_CLOCK_ENABLE;

	clk = clk_get(NULL,"BUSCLK");
        ahb_clock= clk_get_rate(clk);
        clock_divider=(ahb_clock*4)/USB_SAMPLE_FREQ;
        
        switch(clock_divider)
        {
            case USB_CLOCK_FACTOR_13:
                 divider_factor=USB_CLOCK_FACTOR_13_MASK;
                 break;
            case USB_CLOCK_FACTOR_10:
                 divider_factor=USB_CLOCK_FACTOR_10_MASK;
                 break;
            case USB_CLOCK_FACTOR_11:
                 divider_factor=USB_CLOCK_FACTOR_11_MASK;
                 break;
            case USB_CLOCK_FACTOR_12:
                 divider_factor=USB_CLOCK_FACTOR_12_MASK;
                 break;
            default:
                 printk("invalid frequency facor!\n");
                 break;
        }

           
        tmp = *((volatile unsigned int*)(pSysBase + SCPER_FREQ_FACTOR));
        *((volatile unsigned int*)(pSysBase + SCPER_FREQ_FACTOR)) = (tmp &(~0x300)) |divider_factor;

        /* disfrock the  reset */
        tmp = *((volatile unsigned int*)(pSysBase + SCPRESET));
        *((volatile unsigned int*)(pSysBase + SCPRESET)) = tmp | USB_RESET_DISFROCK;
        printk("Clock to USB host has been enabled \n");
       	
		       
	printk(KERN_DEBUG __FILE__
		   ": Clock to USB host has been enabled \n");		   
}

static void hisilicon_stop_hc(struct platform_device *dev)
{

    unsigned char* pSysBase = NULL;
    
	printk(KERN_DEBUG __FILE__
	       ": stopping hisilicon OHCI USB Controller\n");
	
	pSysBase = (unsigned char*)pSysClkAddrBase;       
	*((volatile unsigned int*)(pSysBase + SCPERDIS)) = USB_CLOCK_DISABLE;	
	
}


/*-------------------------------------------------------------------------*/

/* configure so an HC device and id are always provided */
/* always called with process context; sleeping is OK */


/**
 * usb_hcd_hisilicon_probe - initialize hisilicon-based HCDs
 * Context: !in_interrupt()
 *
 * Allocates basic resources for this USB host controller, and
 * then invokes the start() method for the HCD associated with it
 * through the hotplug entry's driver_data.
 *
 */
int usb_hcd_hisilicon_probe (const struct hc_driver *driver,
			  struct platform_device *dev)
{
	int retval;
	struct usb_hcd *hcd;

		if (dev->resource[1].flags != IORESOURCE_IRQ) {
		pr_debug("resource[1] is not IORESOURCE_IRQ");
		return -ENOMEM;
	}

	hcd = usb_create_hcd(driver, &dev->dev, "hisilicon");
	if (!hcd)
		return -ENOMEM;
	hcd->rsrc_start = dev->resource[0].start;
	hcd->rsrc_len = dev->resource[0].end - dev->resource[0].start + 1;

	if (!request_mem_region(hcd->rsrc_start, hcd->rsrc_len, hcd_name)) {
		pr_debug("request_mem_region failed");
		retval = -EBUSY;
		goto err1;
	}
	
	//hcd->regs = ioremap(hcd->rsrc_start, hcd->rsrc_len);
	hcd->regs = ioremap_nocache(hcd->rsrc_start, hcd->rsrc_len);
	if (!hcd->regs) {
		pr_debug("ioremap failed");
		retval = -ENOMEM;
		goto err2;
	}

	hisilicon_start_hc(dev);
	ohci_hcd_init(hcd_to_ohci(hcd));
	
	//printk("%s irq num =%d\n",__FUNCTION__,dev->resource[1].start);
	retval = usb_add_hcd(hcd, dev->resource[1].start, SA_INTERRUPT);	
	
	if (retval == 0)
		return retval;
	hisilicon_stop_hc(dev);
	iounmap(hcd->regs);
 err2:
	release_mem_region(hcd->rsrc_start, hcd->rsrc_len);
 err1:
 	//printk("in put_hc\n");
	usb_put_hcd(hcd);
	return retval;
}


/* may be called without controller electrically present */
/* may be called with controller, bus, and devices active */

/**
 * usb_hcd_hisilicon_remove - shutdown processing for hisilicon-based HCDs
 * @dev: USB Host Controller being removed
 * Context: !in_interrupt()
 *
 * Reverses the effect of usb_hcd_hisilicon_probe(), first invoking
 * the HCD's stop() method.  It is always called from a thread
 * context, normally "rmmod", "apmd", or something similar.
 *
 */
void usb_hcd_hisilicon_remove (struct usb_hcd *hcd, struct platform_device *dev)
{
	usb_remove_hcd(hcd);
	hisilicon_stop_hc(dev);
	iounmap(hcd->regs);
	release_mem_region(hcd->rsrc_start, hcd->rsrc_len);
	usb_put_hcd(hcd);
}

/*-------------------------------------------------------------------------*/

static int __devinit
ohci_hisilicon_start (struct usb_hcd *hcd)
{
	struct ohci_hcd	*ohci = hcd_to_ohci (hcd);
	int		ret;

	ohci_dbg (ohci, "ohci_hisilicon_start, ohci:%p", ohci);
	if ((ret = ohci_init(ohci)) < 0)
		return ret;

	if ((ret = ohci_run (ohci)) < 0) {
		err ("can't start %s", hcd->self.bus_name);
		//printk("can't start\n");
		ohci_stop (hcd);
		return ret;
	}
	return 0;
}

/*-------------------------------------------------------------------------*/

static const struct hc_driver ohci_hisilicon_hc_driver = {
	.description =		hcd_name,
	.product_desc =		"hisilicon OHCI",
	.hcd_priv_size =	sizeof(struct ohci_hcd),

	/*
	 * generic hardware linkage
	 */
	.irq =			ohci_irq,
	.flags =		HCD_USB11 | HCD_MEMORY,

	/*
	 * basic lifecycle operations
	 */
	.start =		ohci_hisilicon_start,
#ifdef	CONFIG_PM
	/* suspend:		ohci_hisilicon_suspend,  -- tbd */
	/* resume:		ohci_hisilicon_resume,   -- tbd */
#endif /*CONFIG_PM*/
	.stop =			ohci_stop,

	/*
	 * managing i/o requests and associated device resources
	 */
	.urb_enqueue =		ohci_urb_enqueue,
	.urb_dequeue =		ohci_urb_dequeue,
	.endpoint_disable =	ohci_endpoint_disable,

	/*
	 * scheduling support
	 */
	.get_frame_number =	ohci_get_frame,

	/*
	 * root hub support
	 */
	.hub_status_data =	ohci_hub_status_data,
	.hub_control =		ohci_hub_control,
};

/*-------------------------------------------------------------------------*/

static int ohci_hcd_hisilicon_drv_probe(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	int ret;

	pr_debug ("In ohci_hcd_hisilicon_drv_probe");

	if (usb_disabled())
		return -ENODEV;

	ret = usb_hcd_hisilicon_probe(&ohci_hisilicon_hc_driver, pdev);
	return ret;
}

static int ohci_hcd_hisilicon_drv_remove(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct usb_hcd *hcd = dev_get_drvdata(dev);

	usb_hcd_hisilicon_remove(hcd, pdev);
	return 0;
}
	/*TBD*/
/*static int ohci_hcd_hisilicon_drv_suspend(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct usb_hcd *hcd = dev_get_drvdata(dev);

	return 0;
}
static int ohci_hcd_hisilicon_drv_resume(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct usb_hcd *hcd = dev_get_drvdata(dev);


	return 0;
}
*/

static struct device_driver ohci_hcd_hisilicon_driver = {
	.name		= "hisilicon-ohci",
	.bus		= &platform_bus_type,
	.probe		= ohci_hcd_hisilicon_drv_probe,
	.remove		= ohci_hcd_hisilicon_drv_remove,
	/*.suspend	= ohci_hcd_hisilicon_drv_suspend, */
	/*.resume	= ohci_hcd_hisilicon_drv_resume, */
};

static int __init ohci_hcd_hisilicon_init (void)
{
	printk(KERN_INFO OSDRV_MODULE_VERSION_STRING "\n");
	pr_debug (DRIVER_INFO " (hisilicon)");
	pr_debug ("block sizes: ed %d td %d\n",
		sizeof (struct ed), sizeof (struct td));

	return driver_register(&ohci_hcd_hisilicon_driver);
}

static void __exit ohci_hcd_hisilicon_cleanup (void)
{
	driver_unregister(&ohci_hcd_hisilicon_driver);
}

module_init (ohci_hcd_hisilicon_init);
module_exit (ohci_hcd_hisilicon_cleanup);
MODULE_VERSION("HI_VERSION=" OSDRV_MODULE_VERSION_STRING);
