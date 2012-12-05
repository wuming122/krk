/**
 * @file lsddm-amr1808-b4-buttons.c
 * @brief buttons driver
 *
 * Copyright (C) 2001-2011, LSD Science & Technology Co.,Ltd
 * All rights reserved.
 * Software License Agreement
 *
 * LSD Science & Technology (LSD) is supplying this software for use solely
 * its suppliers, and is protected. You may not combine this software with
 * "viral" open software in order to form a larger program.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND WITH ALL FAULTS.
 * NO WARRANTIES, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT
 * NOT LIMITED TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. LSD SHALL NOT, UNDER ANY
 * CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL, OR CONSEQUENTIAL
 * DAMAGES, FOR ANY REASON WHATSOEVER.
 *
 * @author toby.zhang <zxj@lierda.com>
 * @date 2011-7-5 16:12:55
 * @version 0.01
 *
 * This file is pary of drivers for LSDDM-AMR1808-B4 board.
 */

/**
 * @note
 *  \n
 *  
 * \b Change \b Logs: \n
 * 2011.07.05   创建初始 by toby \n
 * 
 */

/**
 * @addtogroup api_lsddm-amr1808-b4-buttons
 * @{
 */

// TODO: Add your code here
#include <linux/module.h> 
#include <linux/kernel.h> 
#include <linux/fs.h> 
#include <linux/init.h> 
#include <linux/delay.h> 
#include <linux/poll.h> 
#include <linux/irq.h> 
#include <asm/irq.h> 
#include <linux/interrupt.h> 
#include <asm/uaccess.h> 
#include <mach/hardware.h> 
#include <linux/platform_device.h> 
#include <linux/cdev.h> 
#include <linux/miscdevice.h> 
#include <linux/sched.h> 
#include <linux/gpio.h> 

#include <linux/module.h>
#include <linux/version.h>
#include <linux/delay.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/completion.h>

#include <asm/gpio.h>

#include <linux/input.h> 
#include <linux/module.h> 
#include <linux/init.h> 
#include <asm/irq.h> 
#include <asm/io.h> 
#include <linux/kernel.h> 
#include <linux/fs.h> 
#include <linux/init.h> 
#include <linux/delay.h> 
#include <linux/poll.h> 
#include <linux/irq.h> 
#include <asm/irq.h> 
#include <linux/interrupt.h> 
#include <asm/uaccess.h> 
//#include <mach/regs-gpio.h> 
//#include <mach/hardware.h> 
#include <linux/platform_device.h> 
#include <linux/cdev.h> 
#include <linux/miscdevice.h> 
#include <linux/device.h>

#define LSD_KEY_DEBUG  0

unsigned char button_status[3] = {0xff,0xff,0xff};
/*  
  设备名(/dev/buttons)
*/
#define DEVICE_NAME "buttons" 
#define NO_USE   0

static int fd_open_flag = 0;

struct input_dev *button_dev;

// 定义中断所用的结构体 
struct button_irq_desc
{ 
    int irq; // 按键对应的中断号 
    int pin; // 按键所对应的 GPIO端口 
    irqreturn_t (*p_function)(int irq, void *dev_id); 
    int number; // 定义键值，以传递给应用层/用户态 
    char *name; // 每个按键的名称 
}; 

/*
 * BUTTONS GPGIO MAP:
 * S2 GPIO6[8]  --> button_irqs[0]
 * S3 GPIO6[10] --> button_irqs[1]
 * S4 GPIO6[9]  --> button_irqs[2]
 * S5 GPIO6[11] --> button_irqs[3]
 */
/*
 * 开发板上按键的状态变量，注意这里是’0’，对应的 ASCII码为30
 */
static volatile char pin_values[] = {1,1,1}; 

/*
 * 因为本驱动是基于中断方式的，在此创建一个等待队列，以配合中断函数使用；当有按键按下并读取到键
 * 值时，将会唤醒此队列，并设置中断标志，以便能通过read()函数判断和读取键值传递到用户态；当没有按
 * 键按下时，系统并不会轮询按键状态，以节省时钟资源 
 */
static DECLARE_WAIT_QUEUE_HEAD(buttons_waitq); // 等待队列

/*
 * 中断标识变量，配合上面的队列使用，中断服务程序会把它设置为1，read()函数会把它清零 
 */
static volatile int ev_press = 0; 
 
/*
 * 本按键驱动的中断服务程序
 */
irqreturn_t buttons_interrupt_key(int irq, void *dev_id) 
{ 
    struct button_irq_desc *button_irqs = (struct button_irq_desc *)dev_id; 
    int pin_level_now;
    static int cnt = 0;
 
    // 获取被按下的按键状态
    if((gpio_get_value(button_irqs->pin) == 0) && (gpio_get_value(button_irqs->pin) == 0))
    { 
        // 按键被按下
        pin_level_now = 0;
	#if (LSD_KEY_DEBUG > 0)
	printk("lierda debug: function=%s,gpio=%d,falling\n",__FUNCTION__,button_irqs->pin);
	#endif
	//printk("down\n");
    }
    else if((gpio_get_value(button_irqs->pin) != 0) && (gpio_get_value(button_irqs->pin) != 0))
    {
        // 按键未按下
        pin_level_now = 1;
	#if (LSD_KEY_DEBUG > 0)
	printk("lierda debug: function=%s,gpio=%d,rising\n",__FUNCTION__,button_irqs->pin);
	#endif
	//printk("up\n");
    }

    // 状态改变，按键被按下，从这句可以看出，当按键没有被按下的时候，寄存器的值为1(上拉)，但按
    // 键被按下的时候，寄存器对应的值为0 
    if(pin_level_now != (pin_values[button_irqs->number]))
    {
        // 如果按键被按下，则button_values[0]就变为’1’，对应的 ASCII 码为 31 
        pin_values[button_irqs->number] = pin_level_now; 
	if(pin_level_now == 1)
	{
		#if (LSD_KEY_DEBUG > 0)
		printk("lierda debug: function=%s,gpio=%d",__FUNCTION__,button_irqs->pin);
		#endif
		cnt++;
		//printk("KEY cnt=%d\n",cnt);
		button_status[0] = ~button_status[0];
		input_report_key(button_dev, KEY_1, button_status[0]); 
		//input_report_key(button_dev, KEY_2, cnt+1); 
		//input_report_key(button_dev, KEY_3, cnt+2); 
 		input_sync(button_dev); 
	}
        //ev_press = 1; // 设置中断标志为1 
        //wake_up_interruptible(&buttons_waitq); // 唤醒等待队列
    }
    return IRQ_RETVAL(IRQ_HANDLED);
}

irqreturn_t buttons_interrupt_aaa(int irq, void *dev_id) 
{ 
    struct button_irq_desc *button_irqs = (struct button_irq_desc *)dev_id; 
    int pin_level_now;
    static int a_to_b_cnt = 0;
    static int b_to_a_cnt = 0;
    // 获取被按下的按键状态
    if(gpio_get_value(button_irqs->pin) == 0)
    { 
        // 按键被按下
        pin_level_now = 0;
	#if (LSD_KEY_DEBUG > 0)
	printk("lierda debug: function=%s,gpio=%d,falling\n",__FUNCTION__,button_irqs->pin);
	#endif
    }
    else
    {
        // 按键未按下
        pin_level_now = 1;
	#if (LSD_KEY_DEBUG > 0)
	printk("lierda debug: function=%s,gpio=%d,rising\n",__FUNCTION__,button_irqs->pin);
	#endif
    }

    // 状态改变，按键被按下，从这句可以看出，当按键没有被按下的时候，寄存器的值为1(上拉)，但按
    // 键被按下的时候，寄存器对应的值为0 
    if(pin_level_now != (pin_values[button_irqs->number]))
    {
        // 如果按键被按下，则button_values[0]就变为’1’，对应的 ASCII 码为 31 
        pin_values[button_irqs->number] = pin_level_now; 
	if(pin_level_now == 0)
	{
		#if (LSD_KEY_DEBUG > 0)
		printk("lierda debug: function=%s,gpio=%d,",__FUNCTION__,button_irqs->pin);
		#endif
		
		if(gpio_get_value(1*32+20) == 0)
    		{
			b_to_a_cnt++; 
			//printk("B->A,cnt=%d\n",b_to_a_cnt); 
			button_status[1] = ~button_status[1];
			//input_report_key(button_dev, KEY_1, b_to_a_cnt); 
			input_report_key(button_dev, KEY_2, button_status[1]); 
			//input_report_key(button_dev, KEY_3, b_to_a_cnt+2); 
 			input_sync(button_dev); 
		}
		else
		{
			a_to_b_cnt++;
			//printk("A->B,cnt=%d\n",a_to_b_cnt); 
			button_status[2] = ~button_status[2];
			//input_report_key(button_dev, KEY_1, a_to_b_cnt); 
			//input_report_key(button_dev, KEY_2, a_to_b_cnt+1); 
			input_report_key(button_dev, KEY_3, button_status[2]);  
 			input_sync(button_dev); 
		}
	}
        //ev_press = 1; // 设置中断标志为1 
        //wake_up_interruptible(&buttons_waitq); // 唤醒等待队列
    }
    return IRQ_RETVAL(IRQ_HANDLED);
}

irqreturn_t buttons_interrupt_bbb(int irq, void *dev_id) 
{ 
    return 0;
}


static struct button_irq_desc button_irqs[] =
{ 
    {NO_USE, 1*32+18,  	buttons_interrupt_key,  0, "KEY"},  
    {NO_USE, 1*32+19, 	buttons_interrupt_aaa, 	1, "AAA"}, 
    {NO_USE, 1*32+20,  	buttons_interrupt_bbb,  2, "BBB"}, 
};

/*
 * 在应用程序执行 open(“/dev/buttons”,…)时会调用到此函数，在这里，它的作用主要是注册4个按键的中断。
 * 所用的中断类型是IRQ_TYPE_EDGE_BOTH，也就是双沿触发，在上升沿和下降沿均会产生中断，这样做
 * 是为了更加有效地判断按键状态
 */
static int lsddm_amr1808_b4_buttons_buttons_open(struct input_dev *dev)
{ 
    // 正常返回 
    return 0; 
} 
 
/*
 * 此函数对应应用程序的系统调用close(fd)函数， 在此它的主要作用是当关闭设备时释放4个按键的中断
 * 处理函数
 */
static void lsddm_amr1808_b4_buttons_buttons_close(struct input_dev *dev)
{ 
    //return 0; 
} 
 
/*
 * 对应应用程序的read(fd,…)函数，主要用来向用户空间传递键值
 */
static int lsddm_amr1808_b4_buttons_buttons_read(struct file *filp, char __user *buff, size_t count, loff_t *offp) 
{ 
    unsigned long err;

    if(!ev_press)
    { 
        if(filp->f_flags & O_NONBLOCK) 
            // 当中断标识为 0 时，并且该设备是以非阻塞方式打开时，返回 
            return -EAGAIN; 
        else 
            // 当中断标识为 0 时，并且该设备是以阻塞方式打开时，进入休眠状态，等待被唤醒 
            wait_event_interruptible(buttons_waitq, ev_press);
    } 

    // 把中断标识清零 
    ev_press = 0; 

    // 一组键值被传递到用户空间 
    err = copy_to_user(buff, (const void *)pin_values, min(sizeof(pin_values), count)); 

    return err ? -EFAULT : min(sizeof(pin_values), count); 
} 

static unsigned int lsddm_amr1808_b4_buttons_buttons_poll( struct file *file, struct poll_table_struct *wait) 
{ 
    unsigned int mask = 0; 
    
    // 把调用poll或者 select 的进程挂入队列，以便被驱动程序唤醒
    poll_wait(file, &buttons_waitq, wait); 

    if(ev_press)
        mask |= POLLIN | POLLRDNORM; 

    return mask; 
} 
 

static int __init lsddm_amr1808_b4_buttons_init(void) 
{ 
    	//int ret; 
 	int i; 
	int err = 0; 
        int status;
	struct input_dev *input_dev;  
	int error;

	printk("lsddm_amr1808_b4_buttons_init module start/n");
    	for(i=0; i<sizeof(button_irqs)/sizeof(button_irqs[0]); i++)
    	{ 

        	status = gpio_request(button_irqs[i].pin, "gpio_test\n");
       	 	if(status < 0)
        	{
            		printk("ERROR can not open GPIO %d\n", button_irqs[i].pin);
            		return status;
        	}
		else
		{
			printk("gpio_request ok\n");
		}

        	gpio_direction_input(button_irqs[i].pin);       

        	// 注册中断函数 
		// 中断标志 IRQ_TYPE_EDGE_BOTH   IRQ_TYPE_EDGE_FALLING  IRQ_TYPE_EDGE_RISING
		// IRQ_TYPE_LEVEL_LOW   IRQ_TYPE_LEVEL_HIGH
        	err = request_irq(gpio_to_irq(button_irqs[i].pin), button_irqs[i].p_function, IRQ_TYPE_EDGE_BOTH ,
            		button_irqs[i].name, (void *)&button_irqs[i]); 
    
        	if (err)
            		break;
		else
		{
			printk("request_irq ok\n");
		}
    	} 

    	if(err)
    	{ 
        	// 如果出错，释放已经注册的中断，并返回 
        	i--; 
        	for(; i>=0; i--)
        	{ 
            		if(button_irqs[i].irq < 0)
            		{ 
                		continue; 
            		} 
            		disable_irq(gpio_to_irq(button_irqs[i].pin)); 
            		free_irq(gpio_to_irq(button_irqs[i].pin), (void *)&button_irqs[i]); 
        	} 
        	return -EBUSY; 
        } 
 
    	// 注册成功，则中断队列标记为 1，表示可以通过 read 读取
    	ev_press = 1; 
    	fd_open_flag = 1;
	      
     	/*给input_dev这个结构体指针分配空间*/ 
     	input_dev = input_allocate_device();   
 	if(!input_dev)   
 	{   
  		printk("Unable to allocate the input device!!/n");   
  		return -ENOMEM;   
 	}   
 	button_dev = input_dev;   

	// 设置按键的类型和所支持的键，EV_KEY为按键类型，KEY_ESC、KEY_1-5为所支持的键，
	// 由于我希望我的按键取1-6，从include/linux/input.h中查询相关的宏可以知道我选择
	// 支持的键为1-6，当然你也可以选择你自己喜欢的键值，如果你选择的不是1-6，应用程序取
	// 得的按键号ev_key.code也会根据你的设置而改变
	set_bit(EV_KEY, button_dev->evbit); 
	set_bit(KEY_1, button_dev->keybit); 
	set_bit(KEY_2, button_dev->keybit); 
	set_bit(KEY_3, button_dev->keybit); 

	// 填充设备的名字，设置此名字可以让用户查询该设备的用途，该名字将存入在input/event*
	// 目录下的name 中，cat name将会显示你设置的名字 
 	button_dev->name = "buttons_yzx"; 
 	// 设置我的input子系统的名字，加载模块后，/sys/class/input目录下将会有你所设置的名字
 	button_dev->dev.init_name = "input_yzx"; 
	
	/*设置结构体中的回调函数*/ 
	button_dev->open = lsddm_amr1808_b4_buttons_buttons_open; 
	button_dev->close = lsddm_amr1808_b4_buttons_buttons_close;

	printk("input device has allocated\n");  

	 /*注册设备*/ 
 	error = input_register_device(button_dev); 
 	if (error) 
 	{ 
  		printk(KERN_ERR "button.c: Failed to register device\n"); 
  		input_free_device(button_dev); 
   		return error; 
 	} 
  
 	printk("register device has success\n"); 
    	// 把按键设备注册为 misc 设备，其设备号是自动分配的 
    	//ret = misc_register(&misc); 
    	//printk (DEVICE_NAME"\tinitialized\n"); 

    	return 0; 
} 

static void __exit lsddm_amr1808_b4_buttons_exit(void) 
{ 
    /*if(fd_open_flag != 0)
    {
      int i; 
     
      for (i = 0; i < sizeof(button_irqs)/sizeof(button_irqs[0]); i++) { 

         gpio_free(button_irqs[i].pin);
         // 释放中断号，并注销中断处理函数 
         free_irq(gpio_to_irq(button_irqs[i].pin), (void *)&button_irqs[i]); 
      } 
    }*/

    int i; 

    for (i= 0; i<sizeof(button_irqs)/sizeof(button_irqs[0]); i++)
    { 
        // GPIO 释放  
        gpio_free(button_irqs[i].pin);

        // 释放中断号，并注销中断处理函数 
        free_irq(gpio_to_irq(button_irqs[i].pin), (void *)&button_irqs[i]); 
    } 

    fd_open_flag = 0;
    /*删除设备*/ 
    input_unregister_device(button_dev);  
    //misc_deregister(&misc); 
} 

module_init(lsddm_amr1808_b4_buttons_init); 
module_exit(lsddm_amr1808_b4_buttons_exit);

MODULE_AUTHOR("Toby Zhang");
MODULE_DESCRIPTION("LSDDM-AMR1808-B4 LEDs Driver");
MODULE_LICENSE("GPL");

/**
 * Close the Doxygen group.
 * @}
 */

