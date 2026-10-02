#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/list.h>

#define  DEVICE_NAME "list_driver"
#define  CLASS_NAME  "list_class"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Author Name");
MODULE_DESCRIPTION("A generic Linux char driver.");
MODULE_VERSION("0.2");

static int    majorNumber;
static int    numberOpens = 0;
static struct class *charClass  = NULL;
static struct device *charDevice = NULL;

struct message_node {
	struct list_head list;
	char  message[256];
	short size_of_message;
};

static LIST_HEAD(message_list);

static int     dev_open(struct inode *, struct file *);
static int     dev_release(struct inode *, struct file *);
static ssize_t dev_read(struct file *, char *, size_t, loff_t *);
static ssize_t dev_write(struct file *, const char *, size_t, loff_t *);

static struct file_operations fops =
{
	.open = dev_open,
	.read = dev_read,
	.write = dev_write,
	.release = dev_release,
};


static int __init simple_init(void){
	printk(KERN_INFO "Simple Driver: Initializing the LKM\n");

	majorNumber = register_chrdev(0, DEVICE_NAME, &fops);
	if (majorNumber<0){
		printk(KERN_ALERT "Simple Driver failed to register a major number\n");
		return majorNumber;
	}
	
	printk(KERN_INFO "Simple Driver: registered correctly with major number %d\n", majorNumber);

	charClass = class_create(THIS_MODULE, CLASS_NAME);
	if (IS_ERR(charClass)){
		unregister_chrdev(majorNumber, DEVICE_NAME);
		printk(KERN_ALERT "Simple Driver: failed to register device class\n");
		return PTR_ERR(charClass);
	}
	
	printk(KERN_INFO "Simple Driver: device class registered correctly\n");

	charDevice = device_create(charClass, NULL, MKDEV(majorNumber, 0), NULL, DEVICE_NAME);
	if (IS_ERR(charDevice)){
		class_destroy(charClass);
		unregister_chrdev(majorNumber, DEVICE_NAME);
		printk(KERN_ALERT "Simple Driver: failed to create the device\n");
		return PTR_ERR(charDevice);
	}
	
	printk(KERN_INFO "Simple Driver: device class created correctly\n");
		
	return 0;
}

static void __exit simple_exit(void){
	struct message_node *node, *tmp;

	list_for_each_entry_safe(node, tmp, &message_list, list) {
		list_del(&node->list);
		kfree(node);
	}

	device_destroy(charClass, MKDEV(majorNumber, 0));
	class_unregister(charClass);
	class_destroy(charClass);
	unregister_chrdev(majorNumber, DEVICE_NAME);
	printk(KERN_INFO "Simple Driver: goodbye from the LKM!\n");
}

static int dev_open(struct inode *inodep, struct file *filep){
	numberOpens++;
	printk(KERN_INFO "Simple Driver: device has been opened %d time(s)\n", numberOpens);
	return 0;
}

static ssize_t dev_read(struct file *filep, char *buffer, size_t len, loff_t *offset){
	struct message_node *node;
	int error_count = 0;
	short msg_size;

	if (list_empty(&message_list)){
		printk(KERN_INFO "Simple Driver: no message available to read\n");
		return 0;
	}

	node = list_first_entry(&message_list, struct message_node, list);
	list_del(&node->list);
	msg_size = node->size_of_message;

	error_count = copy_to_user(buffer, node->message, msg_size);
	kfree(node);

	if (error_count==0){
		printk(KERN_INFO "Simple Driver: sent %d characters to the user\n", msg_size);
		return msg_size;
	}
	else {
		printk(KERN_INFO "Simple Driver: failed to send %d characters to the user\n", error_count);
		return -EFAULT;
	}
}

static ssize_t dev_write(struct file *filep, const char *buffer, size_t len, loff_t *offset){
	struct message_node *node;
	ssize_t ret;

	node = kmalloc(sizeof(struct message_node), GFP_KERNEL);
	if (!node)
		return -ENOMEM;

	if (len < sizeof(node->message)){
		sprintf(node->message, "%s(%zu letters)", buffer, len);
		printk(KERN_INFO "Simple Driver: received %zu characters from the user\n", len);
		ret = len;
	}else{
		sprintf(node->message, "(0 letters)");
		printk(KERN_INFO "Simple Driver: too many characters to deal with\n");
		ret = 0;
	}
	node->size_of_message = strlen(node->message);

	list_add_tail(&node->list, &message_list);

	return ret;
}

static int dev_release(struct inode *inodep, struct file *filep){
	printk(KERN_INFO "Simple Driver: device successfully closed\n");
	return 0;
}

module_init(simple_init);
module_exit(simple_exit);