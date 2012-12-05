#include <linux/miscdevice.h> 
#include <linux/delay.h> 
#include <asm/irq.h> 
#include <mach/hardware.h> 
#include <linux/kernel.h> 
#include <linux/module.h> 
#include <linux/init.h> 
#include <linux/mm.h> 
#include <linux/fs.h> 
#include <linux/types.h> 
#include <linux/delay.h> 
#include <linux/moduleparam.h> 
#include <linux/slab.h> 
#include <linux/errno.h> 
#include <linux/ioctl.h> 
#include <linux/cdev.h> 
#include <linux/string.h> 
#include <linux/list.h> 
#include <linux/pci.h> 
#include <linux/gpio.h> 
#include <asm/uaccess.h> 
#include <asm/atomic.h> 
#include <asm/unistd.h> 

#include <plat/irqs.h>

#include <linux/version.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/completion.h>
#include <asm/gpio.h>
#include <linux/lierda_debug.h>

#include "dmtimer.h"
#define DEVICE_NAME "gpio" //设备名(/dev/led) 

#define MOTOR_MAGIC 'L'
#define SET_LED _IOW(MOTOR_MAGIC, 0,int)


#define TIMER_INITIAL_COUNT             (0xFFF00000u)
#define TIMER_RLD_COUNT                 (0xFF000000u)
#define TIMER_FINAL_COUNT               (0x0FFFFu)


#define SOC_DMTIMER_0_REGS                   (0x44E05000)
#define SOC_DMTIMER_1_REGS                   (0x44E31000)
#define SOC_DMTIMER_2_REGS                   (0x48040000)
#define SOC_DMTIMER_3_REGS                   (0x48042000)
#define SOC_DMTIMER_4_REGS                   (0x48044000)
#define SOC_DMTIMER_5_REGS                   (0x48046000)
#define SOC_DMTIMER_6_REGS                   (0x48048000)
#define SOC_DMTIMER_7_REGS                   (0x4804A000)
void __iomem *DMTIMER2_BASE; 



static int gpio_num[14] = {
	1*32+16,	
	1*32+17,
	1*32+18,
	1*32+19,
	1*32+20,
	1*32+21,
	1*32+22,	
	1*32+23,
	1*32+24,
	1*32+25,
	1*32+26,
	1*32+27,
	0*32+4,
	0*32+5,
};
 
// ioctl 函数的实现 
// 在应用用户层将通过 ioctl 函数向内核传递参数，以控制 LED的输出状态 
static int am1808_led_ioctl(  
 struct file *file,   
 unsigned int cmd,  
 unsigned long arg) 
{ 
	int real_cmd = 0;
	if(cmd < SET_LED)
	{
		lsd_dbg(LSD_ERR,"led cmd to small\n");
		return -1;
	}
	else
	{
		real_cmd = cmd - SET_LED;
		if(real_cmd >= 14)
		{
			lsd_dbg(LSD_ERR,"led cmd to big\n");
			return -1;
		}		
		//lsd_dbg(LSD_DBG,"led cmd=%d\n",real_cmd);
	}
	
	if(arg == 0)
	{
		gpio_set_value(gpio_num[real_cmd],0); 
		//lsd_dbg(LSD_DBG,"set led=%d level=%d\n",real_cmd,arg);
	}
	else if(arg == 1)
	{
		gpio_set_value(gpio_num[real_cmd],1); 
		//lsd_dbg(LSD_DBG,"set led=%d level=%d\n",real_cmd,arg);
	}
	else
	{
		lsd_dbg(LSD_ERR,"arg=%d error \n",arg);
	}
	return 0;       
} 
 
 
//  设备函数操作集，在此只有 ioctl函数，通常还有 read, write, open, close 等，因为本 LED驱动在下面已经
//  注册为 misc 设备，因此也可以不用 open/close  
static struct file_operations dev_fops = { 
 .owner = THIS_MODULE, 
 .unlocked_ioctl  = am1808_led_ioctl, 
}; 
  
//  把 LED驱动注册为 MISC 设备 
static struct miscdevice misc = { 
  //动态设备号
  .minor = MISC_DYNAMIC_MINOR,  
  .name = DEVICE_NAME, 
  .fops = &dev_fops, 
}; 
 

static irqreturn_t rotator_interrupt(int irq, void *dev_id)
{
printk(" AAA Lierda Eter in <%s,%s,%d>.\n",__FUNCTION__,__FILE__,__LINE__);    //wuming 20120627
    return IRQ_RETVAL(IRQ_HANDLED);
}
 
// 设备初始化 
static int __init dev_init(void) 
{ 
    int ret; 
    int i;  
    int status;
  int err = 0;
   unsigned long value=0;
printk(" AAA Lierda Eter in <%s,%s,%d>.\n",__FUNCTION__,__FILE__,__LINE__);    //wuming 20120627
   for(i = 0; i < 14 ; i++)
   {
	   status = gpio_request(gpio_num[i], "gpio_test\n");
	   if (status < 0) {
	 	lsd_dbg(LSD_ERR,"failed to open GPIO %d\n", gpio_num[i]);	
		return status;
	   }
	   else
	   {
		lsd_dbg(LSD_OK,"open GPIO %d ok\n", gpio_num[i]);	
	   }
	   gpio_direction_output(gpio_num[i],0); 
 
   }

   ret = misc_register(&misc); //注册设备     
DMTIMER2_BASE = ((volatile unsigned long)ioremap(SOC_DMTIMER_1_REGS,0x60));
     
// DMTimerCounterSet(SOC_DMTIMER_2_REGS, TIMER_INITIAL_COUNT);
__raw_writel(TIMER_INITIAL_COUNT, DMTIMER2_BASE+DMTIMER_TCRR);
value=__raw_readl(DMTIMER2_BASE+DMTIMER_TCRR);
printk(" AAA Lierda Eter in <%s,%s,%d>value=%x.\n",__FUNCTION__,__FILE__,__LINE__,value);    //wuming 20120627
    /* Load the load register with the reload count value */
  //  DMTimerReloadSet(SOC_DMTIMER_2_REGS, TIMER_RLD_COUNT);
__raw_writel(TIMER_RLD_COUNT, DMTIMER2_BASE+DMTIMER_TLDR);
value=__raw_readl(DMTIMER2_BASE+DMTIMER_TLDR);
printk(" AAA Lierda Eter in <%s,%s,%d>value=%x.\n",__FUNCTION__,__FILE__,__LINE__,value);    //wuming 20120627
    /* Configure the DMTimer for Auto-reload and compare mode */
   // DMTimerModeConfigure(SOC_DMTIMER_2_REGS, DMTIMER_AUTORLD_NOCMP_ENABLE);
__raw_writel(DMTIMER_AUTORLD_NOCMP_ENABLE+DMTIMER_TCLR_ST, DMTIMER2_BASE+DMTIMER_TCLR);
value=__raw_readl(DMTIMER2_BASE+DMTIMER_TCLR);
printk(" AAA Lierda Eter in <%s,%s,%d>value=%x.\n",__FUNCTION__,__FILE__,__LINE__,value);    //wuming 20120627
 err = request_irq(67, rotator_interrupt, IRQ_TYPE_EDGE_BOTH, DEVICE_NAME, NULL);
printk(" AAA Lierda Eter in <%s,%s,%d>.\n",__FUNCTION__,__FILE__,__LINE__);    //wuming 20120627
value=__raw_readl(DMTIMER2_BASE+DMTIMER_TCRR);
printk(" AAA Lierda Eter in <%s,%s,%d>value=%x.\n",__FUNCTION__,__FILE__,__LINE__,value);    //wuming 20120627
	if (err)
    {
		disable_irq(INT_TIMER2);
		free_irq(INT_TIMER2, NULL);
        return -EBUSY;
    }
//__raw_writel(DMTIMER_TCLR_ST+DMTIMER_AUTORLD_NOCMP_ENABLE, DMTIMER2_BASE+DMTIMER_TCLR);
value=__raw_readl(DMTIMER2_BASE+DMTIMER_TCLR);
printk(" AAA Lierda Eter in <%s,%s,%d>value=%x.\n",__FUNCTION__,__FILE__,__LINE__,value);    //wuming 20120627
value=__raw_readl(DMTIMER2_BASE+DMTIMER_TCRR);
printk(" AAA Lierda Eter in <%s,%s,%d>value=%x.\n",__FUNCTION__,__FILE__,__LINE__,value);    //wuming 20120627
value=__raw_readl(DMTIMER2_BASE+DMTIMER_TCRR);
printk(" AAA Lierda Eter in <%s,%s,%d>value=%x.\n",__FUNCTION__,__FILE__,__LINE__,value);    //wuming 20120627


value=__raw_readl(DMTIMER2_BASE+DMTIMER_IRQENABLE_SET);
printk(" AAA Lierda Eter in <%s,%s,%d>value=%x.\n",__FUNCTION__,__FILE__,__LINE__,value);    //wuming 20120627


__raw_writel(DMTIMER_INT_OVF_EN_FLAG+DMTIMER_INT_MAT_IT_FLAG, DMTIMER2_BASE+DMTIMER_IRQENABLE_SET);
value=__raw_readl(DMTIMER2_BASE+DMTIMER_IRQENABLE_SET);
printk(" AAA Lierda Eter in <%s,%s,%d>value=%x.\n",__FUNCTION__,__FILE__,__LINE__,value);    //wuming 20120627

//__raw_writel(DMTIMER_TCLR_ST+DMTIMER_AUTORLD_NOCMP_ENABLE, DMTIMER2_BASE+DMTIMER_TCLR);
value=__raw_readl(DMTIMER2_BASE+DMTIMER_TCLR);
printk(" AAA Lierda Eter in <%s,%s,%d>value=%x.\n",__FUNCTION__,__FILE__,__LINE__,value);    //wuming 20120627



value=__raw_readl(DMTIMER2_BASE+DMTIMER_IRQSTATUS);
printk(" AAA Lierda Eter in <%s,%s,%d>value=%x.\n",__FUNCTION__,__FILE__,__LINE__,value);    //wuming 20120627

value=__raw_readl(DMTIMER2_BASE+DMTIMER_IRQSTATUS);
printk(" AAA Lierda Eter in <%s,%s,%d>value=%x.\n",__FUNCTION__,__FILE__,__LINE__,value);    //wuming 20120627

value=__raw_readl(DMTIMER2_BASE+DMTIMER_IRQSTATUS);
printk(" AAA Lierda Eter in <%s,%s,%d>value=%x.\n",__FUNCTION__,__FILE__,__LINE__,value);    //wuming 20120627

value=__raw_readl(DMTIMER2_BASE+DMTIMER_IRQSTATUS);
printk(" AAA Lierda Eter in <%s,%s,%d>value=%x.\n",__FUNCTION__,__FILE__,__LINE__,value);    //wuming 20120627

value=__raw_readl(DMTIMER2_BASE+DMTIMER_IRQSTATUS);
printk(" AAA Lierda Eter in <%s,%s,%d>value=%x.\n",__FUNCTION__,__FILE__,__LINE__,value);    //wuming 20120627

value=__raw_readl(DMTIMER2_BASE+DMTIMER_IRQSTATUS);
printk(" AAA Lierda Eter in <%s,%s,%d>value=%x.\n",__FUNCTION__,__FILE__,__LINE__,value);    //wuming 20120627

value=__raw_readl(DMTIMER2_BASE+DMTIMER_TCRR);
printk(" AAA Lierda Eter in <%s,%s,%d>value=%x.\n",__FUNCTION__,__FILE__,__LINE__,value);    //wuming 20120627
value=__raw_readl(DMTIMER2_BASE+DMTIMER_TCRR);
printk(" AAA Lierda Eter in <%s,%s,%d>value=%x.\n",__FUNCTION__,__FILE__,__LINE__,value);    //wuming 20120627
for(i=0;i<5000;i++)
{
	;
}
value=__raw_readl(DMTIMER2_BASE+DMTIMER_TCRR);
printk(" AAA Lierda Eter in <%s,%s,%d>value=%x.\n",__FUNCTION__,__FILE__,__LINE__,value);    //wuming 20120627
value=__raw_readl(DMTIMER2_BASE+DMTIMER_TCRR);
printk(" AAA Lierda Eter in <%s,%s,%d>value=%x.\n",__FUNCTION__,__FILE__,__LINE__,value);    //wuming 20120627

value=__raw_readl(DMTIMER2_BASE+DMTIMER_TCRR);
printk(" AAA Lierda Eter in <%s,%s,%d>value=%x.\n",__FUNCTION__,__FILE__,__LINE__,value);    //wuming 20120627
value=__raw_readl(DMTIMER2_BASE+DMTIMER_TCRR);
printk(" AAA Lierda Eter in <%s,%s,%d>value=%x.\n",__FUNCTION__,__FILE__,__LINE__,value);    //wuming 20120627
for(i=0;i<5000;i++)
{
	;
}
value=__raw_readl(DMTIMER2_BASE+DMTIMER_TCRR);
printk(" AAA Lierda Eter in <%s,%s,%d>value=%x.\n",__FUNCTION__,__FILE__,__LINE__,value);    //wuming 20120627
value=__raw_readl(DMTIMER2_BASE+DMTIMER_TCRR);
printk(" AAA Lierda Eter in <%s,%s,%d>value=%x.\n",__FUNCTION__,__FILE__,__LINE__,value);    //wuming 20120627

value=__raw_readl(DMTIMER2_BASE+DMTIMER_TCRR);
printk(" AAA Lierda Eter in <%s,%s,%d>value=%x.\n",__FUNCTION__,__FILE__,__LINE__,value);    //wuming 20120627
value=__raw_readl(DMTIMER2_BASE+DMTIMER_TCRR);
printk(" AAA Lierda Eter in <%s,%s,%d>value=%x.\n",__FUNCTION__,__FILE__,__LINE__,value);    //wuming 20120627
for(i=0;i<5000;i++)
{
	;
}
value=__raw_readl(DMTIMER2_BASE+DMTIMER_TCRR);
printk(" AAA Lierda Eter in <%s,%s,%d>value=%x.\n",__FUNCTION__,__FILE__,__LINE__,value);    //wuming 20120627
value=__raw_readl(DMTIMER2_BASE+DMTIMER_TCRR);
printk(" AAA Lierda Eter in <%s,%s,%d>value=%x.\n",__FUNCTION__,__FILE__,__LINE__,value);    //wuming 20120627




value=__raw_readl(DMTIMER2_BASE+DMTIMER_IRQSTATUS);
printk(" AAA Lierda Eter in <%s,%s,%d>value=%x.\n",__FUNCTION__,__FILE__,__LINE__,value);    //wuming 20120627
value=__raw_readl(DMTIMER2_BASE+DMTIMER_IRQSTATUS);
printk(" AAA Lierda Eter in <%s,%s,%d>value=%x.\n",__FUNCTION__,__FILE__,__LINE__,value);    //wuming 20120627
value=__raw_readl(DMTIMER2_BASE+DMTIMER_IRQSTATUS);
printk(" AAA Lierda Eter in <%s,%s,%d>value=%x.\n",__FUNCTION__,__FILE__,__LINE__,value);    //wuming 20120627
value=__raw_readl(DMTIMER2_BASE+DMTIMER_IRQSTATUS);
printk(" AAA Lierda Eter in <%s,%s,%d>value=%x.\n",__FUNCTION__,__FILE__,__LINE__,value);    //wuming 20120627

   printk (DEVICE_NAME"\tinitialized\n"); //打印初始化信息 
   return ret; 
} 
 
static void __exit dev_exit(void) 
{ 
	int i;
	for(i = 0; i < 14; i++)
	{
		gpio_free(gpio_num[i]);
	}
   
   	misc_deregister(&misc); 
} 




 
// 模块初始化，仅当使用 insmod/podprobe 命令加载时有用，
// 如果设备不是通过模块方式加载，此处将不会被调用 
module_init(dev_init); 

// 卸载模块，当该设备通过模块方式加载后，
// 可以通过 rmmod 命令卸载，将调用此函数 
module_exit(dev_exit);

// 版权信息 
MODULE_LICENSE("GPL"); 
// 开发者信息 
MODULE_AUTHOR("lierda Inc."); 
