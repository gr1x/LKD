#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <asm/siginfo.h>   //siginfo
#include <linux/rcupdate.h>   //rcu_read_lock
#include <linux/sched.h>   //find_task_by_pid_type
#include <linux/ioctl.h>
#include <asm/signal.h>
#include <linux/wait.h>
#include <linux/kthread.h>
#define _BSD_SOURCE 
#include <linux/unistd.h>
#include <asm/siginfo.h>   //siginfo

#include "common.h"

#define LINUX

#define  DEVICE_NAME "sigchar"
#define  CLASS_NAME  "galaxy"

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("A simple char module to signal a kernel thread");
MODULE_VERSION("1.0");

static int pid = 0;
static int tpid = 0;
struct task_struct *task;

static int    majorNumber;
static int    numberOpens = 0;
static struct class*  sigcharClass  = NULL;
static struct device* sigcharDevice = NULL;

static int     sigchar_open(struct inode *, struct file *);
static int     sigchar_release(struct inode *, struct file *);
static ssize_t sigchar_read(struct file *, char *, size_t, loff_t *);
static ssize_t sigchar_write(struct file *, const char *, size_t, loff_t *);

static struct file_operations fops =
{
	.open = sigchar_open,
	.read = sigchar_read,
	.write = sigchar_write,
	.release = sigchar_release,
};

static DECLARE_WAIT_QUEUE_HEAD(thread_signal_waitqueue);

#define kthread_debug(fmt, arg...) \
printk(KERN_ALERT fmt, ##arg)

static int thread_signal(void *data)
{
	/* declare wait queue entry */
	DECLARE_WAITQUEUE(wait, current);   

	tpid = task_pid_nr(current);
	kthread_debug("thread_signal created PID %d.\n", tpid);

    /* to protect against module removal */
	try_module_get(THIS_MODULE);

    /* since daemonize blocks all signal we enable only SIGKILL here */
	allow_signal(SIGKILL);  // kill -l
	allow_signal(SIGSTOP);  // pkill -19 tid
	allow_signal(SIGCONT);
	allow_signal(SIGURG);
	allow_signal(SIGTERM);
	allow_signal(SIGCONT);
	allow_signal(63);

	add_wait_queue(&thread_signal_waitqueue, &wait);    

	while (!kthread_should_stop()) {

		if (signal_pending(current)) {
			kthread_debug("SIGKILL pending\n");
			siginfo_t info = {0};
			unsigned long signr;
			signr = kernel_dequeue_signal(&info);
			printk("signo: %d  --- si_code %d --- si_int %d\n", info.si_signo, info.si_code, info.si_int);
			switch(signr) {
				case SIGSTOP:
				printk(KERN_DEBUG "thread_process(): SIGSTOP received.\n");
				// set_current_state(TASK_STOPPED);
				schedule();
				break;
				case SIGCONT:
				printk(KERN_DEBUG "thread_process(): SIGCONT received.\n");
				// set_current_state (TASK_INTERRUPTIBLE);

				schedule();
				break;

				case SIGKILL:
				printk(KERN_DEBUG "thread_process(): SIGKILL received.\n");
				break;

				case 63:
				printk(KERN_DEBUG "thread_process(): Sig 63 received.\n");
				break;

				case SIGHUP:
				printk(KERN_DEBUG "thread_process(): SIGHUP received.\n");
				break;
				default:
				printk(KERN_DEBUG "thread_process(): signal %ld received\n", signr);
			}
		}
        /* must be set before calling schedule() to 
         * avoid race condition */
		set_current_state(TASK_INTERRUPTIBLE);    

        /* remove current from run queue and schedule a new process */
		schedule();

		kthread_debug("thread_signal woke up!\n");       
	}

        /* we wake up here */
        /* check for pending SIGKILL signal, die if there is any */

    /* set_current_state(TASK_RUNNING); */
	remove_wait_queue(&thread_signal_waitqueue, &wait);        

	module_put(THIS_MODULE);
	kthread_debug("thread_signal exit!\n");       

	return 0;    
}

int init_module(void)
{


	pid = task_pid_nr(current);

	printk("<1>Hello world from PID %d.\n", pid);
	
	printk(KERN_INFO "sigchar: Initializing the sigchar LKM\n");

	majorNumber = register_chrdev(0, DEVICE_NAME, &fops);
	if (majorNumber<0){
		printk(KERN_ALERT "sigchar failed to register a major number\n");
		return majorNumber;
	}
	printk(KERN_INFO "sigchar: registered correctly with major number %d\n", majorNumber);

	sigcharClass = class_create(THIS_MODULE, CLASS_NAME);
	if (IS_ERR(sigcharClass)){
		unregister_chrdev(majorNumber, DEVICE_NAME);
		printk(KERN_ALERT "Failed to register device class\n");
		return PTR_ERR(sigcharClass);
	}
	printk(KERN_INFO "sigchar: device class registered correctly\n");

	sigcharDevice = device_create(sigcharClass, NULL, MKDEV(majorNumber, 0), NULL, DEVICE_NAME);
	if (IS_ERR(sigcharDevice)){
		class_destroy(sigcharClass);
		unregister_chrdev(majorNumber, DEVICE_NAME);
		printk(KERN_ALERT "Failed to create the device\n");
		return PTR_ERR(sigcharDevice);
	}
	printk(KERN_INFO "sigchar: device class created correctly\n");


	task = kthread_run(&thread_signal, 0, "Thread_Signal");
	tpid = task->pid;
	printk("Signal_thread %s : %d Initialized\n", task->comm, task->pid);

	return 0;
}


void cleanup_module(void)
{

	device_destroy(sigcharClass, MKDEV(majorNumber, 0));
	class_unregister(sigcharClass);
	class_destroy(sigcharClass);
	unregister_chrdev(majorNumber, DEVICE_NAME);
	printk(KERN_ALERT "Goodbye world 1.\n");
}  

static int sigchar_open(struct inode *inodep, struct file *filep){
	printk(KERN_INFO "sigchar: Device has been opened %d time(s)\n", numberOpens);
	return 0;
}


static ssize_t sigchar_read(struct file *filep, char *buffer, size_t len, loff_t *offset){
	printk("Stop kernel thread...\n");
	kthread_stop(task);  
	return 0;
}

static ssize_t sigchar_write(struct file *filep, const char *buffer, size_t len, loff_t *offset){
	int ret = 0;
	struct siginfo info;
	struct task_struct *t;

	printk("Sending Signal within kernel..\n");
    memset(&info,0,sizeof(struct siginfo)); // Generate the struct for signals
    info.si_signo = SIGKILL;                // Config the signals
    info.si_code = SI_QUEUE;                // Config SI_QUERE std or SI_KERNEL for RTC, SI_ASYNCIO
    /* real time signals may have 32 bits of data. */
    info.si_int = 1234;


    t = pid_task(find_pid_ns(tpid, &init_pid_ns), PIDTYPE_PID);                // Find the task from pid      
    if(t == NULL) {
    	printk("Error finding the pid task\n"); 
    	return len;
    }
    ret = send_sig_info(63, &info, t);
    printk("send_sig_info ret %d\n", ret); 

  //  wake_up_interruptible(&thread_signal_waitqueue);    

    return len;
}


static int sigchar_release(struct inode *inodep, struct file *filep){
	printk(KERN_INFO "sigchar: Device successfully closed\n");
	kthread_stop(task);
	wake_up_interruptible(&thread_signal_waitqueue);    
	return 0;
}

MODULE_LICENSE("GPL");
