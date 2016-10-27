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

#include "common.h"

#define  DEVICE_NAME "gchar"
#define  CLASS_NAME  "galaxy"

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("A simple char module for ioctl()&send_sig_info()");
MODULE_VERSION("1.0");


static int pid = 0;

static int    majorNumber;
static int    numberOpens = 0;
static struct class*  GCharClass  = NULL;
static struct device* GCharDevice = NULL;

static int     gchar_open(struct inode *, struct file *);
static int     gchar_release(struct inode *, struct file *);
static ssize_t gchar_read(struct file *, char *, size_t, loff_t *);
static ssize_t gchar_write(struct file *, const char *, size_t, loff_t *);
static ssize_t gchar_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);

static struct file_operations fops =
{
    .open = gchar_open,
    .read = gchar_read,
    .write = gchar_write,
    .release = gchar_release,
    .unlocked_ioctl = gchar_ioctl,
};

struct ioctl_data_t ioctl_data = {0};

static int __init gchar_init(void){
    printk(KERN_INFO "gchar: Initializing the gchar LKM\n");

    majorNumber = register_chrdev(0, DEVICE_NAME, &fops);
    if (majorNumber<0){
        printk(KERN_ALERT "gchar failed to register a major number\n");
        return majorNumber;
    }
    printk(KERN_INFO "gchar: registered correctly with major number %d\n", majorNumber);

    GCharClass = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(GCharClass)){
        unregister_chrdev(majorNumber, DEVICE_NAME);
        printk(KERN_ALERT "Failed to register device class\n");
        return PTR_ERR(GCharClass);
    }
    printk(KERN_INFO "gchar: device class registered correctly\n");

    GCharDevice = device_create(GCharClass, NULL, MKDEV(majorNumber, 0), NULL, DEVICE_NAME);
    if (IS_ERR(GCharDevice)){
        class_destroy(GCharClass);
        unregister_chrdev(majorNumber, DEVICE_NAME);
        printk(KERN_ALERT "Failed to create the device\n");
        return PTR_ERR(GCharDevice);
    }
    printk(KERN_INFO "gchar: device class created correctly\n");
    return 0;
}


static void __exit gchar_exit(void){
    device_destroy(GCharClass, MKDEV(majorNumber, 0));
    class_unregister(GCharClass);
    class_destroy(GCharClass);
    unregister_chrdev(majorNumber, DEVICE_NAME);
    printk(KERN_INFO "gchar: Goodbye from the LKM!\n");
}


static int gchar_open(struct inode *inodep, struct file *filep){
    numberOpens++;
    printk(KERN_INFO "gchar: Device has been opened %d time(s)\n", numberOpens);
    return 0;
}


static ssize_t gchar_read(struct file *filep, char *buffer, size_t len, loff_t *offset){
    int error_count = 0;
    char msg[128] = "Hello from Kernel";
    error_count = copy_to_user(buffer, msg, strlen(msg));

    if (error_count==0){ 
        printk(KERN_INFO "gchar: Sent %d characters to the user\n", strlen(msg));
        return 0;
    }
    else {
        printk(KERN_INFO "gchar: Failed to send %d characters to the user\n", error_count);
        return -EFAULT; 
    }
}


static ssize_t gchar_write(struct file *filep, const char *buffer, size_t len, loff_t *offset){
    char   msg[256] = {0};
    short  msg_len = (len < 128) ? len : 128;
    printk(KERN_INFO "gchar: Received %d characters from the user\n", len);
    sprintf(msg, "%s(%d letters)", buffer, msg_len);
    printk(KERN_INFO "gchar: User write [ %s ]\n", msg);
    return len;
}


static int gchar_release(struct inode *inodep, struct file *filep){
    printk(KERN_INFO "gchar: Device successfully closed\n");
    return 0;
}



static ssize_t gchar_ioctl(struct file *filp, unsigned int cmd, unsigned long arg) {
    int err = 0, ret = 0, tmp;

    struct siginfo info;
    struct task_struct *t;
    /* prepare the signal */
    memset(&info, 0, sizeof(struct siginfo));
    info.si_signo = SIG_READ;
    /*
    this is bit of a trickery: SI_QUEUE is normally used by sigqueue from user space,
    and kernel space should use SI_KERNEL. But if SI_KERNEL is used the real_time data 
    is not delivered to the user space signal handler function. 
    */
    info.si_code = SI_QUEUE;
    /* real time signals may have 32 bits of data. */
    info.si_int = 1234;

    /* don't even decode wrong cmds: better returning  ENOTTY than EFAULT */
    if (_IOC_TYPE(cmd) != GCHAR_IOC_MAGIC) return -ENOTTY;
    if (_IOC_NR(cmd) > GCHAR_IOC_MAXNR) return -ENOTTY;
    switch(cmd) {
        case GCHAR_IOC_RESET:
            printk(KERN_INFO "gchar: Reset Device successfully\n");
            break;

        case GCHAR_IOC_READ:
            printk(KERN_INFO "gchar: Read Device\n");

            rcu_read_lock();
            /* find the task with that pid */
            t = pid_task(find_pid_ns(pid, &init_pid_ns), PIDTYPE_PID);  
            if(t == NULL){
                pr_err("no such pid: %d\n", pid);
                rcu_read_unlock();
                return -ENODEV;
            }
            rcu_read_unlock();

            ret = send_sig_info(SIG_READ, &info, t);    // Send the signal to user-space upon receiving
                                                        // a read irq from device.
            if (ret < 0) {
                pr_err("error sending signal: %d\n", ret);
                return ret;
            }
            break;

        case GCHAR_IOC_WRITE:  // Device read the dma descriptors and ret from an write irq.
            printk(KERN_INFO "gchar: Write Device arg addr %p, value %d\n", &arg, *(pid_t*)arg);         
            ret = copy_from_user((u8*)&ioctl_data, (u8*)arg, sizeof(struct ioctl_data_t));
            pid = ioctl_data.pid;
            printk(KERN_INFO "gchar: ret %d PID %d\n", ret, pid);
            break; 

        default:  /* redundant, as cmd was checked against MAXNR */
            printk(KERN_INFO "gchar: Unknown IOCTL_CODE\n");
            return -ENOTTY;
    }

    return ret;        
}

module_init(gchar_init);
module_exit(gchar_exit);
