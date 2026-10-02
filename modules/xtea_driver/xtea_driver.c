#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/types.h>
#include <linux/string.h>

#define DEVICE_NAME "xtea_driver"
#define CLASS_NAME "xtea_class"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Author Name");
MODULE_DESCRIPTION("XTEA encryption");
MODULE_VERSION("0.2");

#define MAX_DATA_BYTES 120
#define CMD_BUF_SIZE 320

static int majorNumber;
static char message[256] = {0};
static short size_of_message;
static int numberOpens = 0;
static struct class *charClass = NULL;
static struct device *charDevice = NULL;

static char *key0 = "f0e1d2c3";
static char *key1 = "b4a59687";
static char *key2 = "78695a4b";
static char *key3 = "3c2d1e0f";

module_param(key0, charp, 0000);
module_param(key1, charp, 0000);
module_param(key2, charp, 0000);
module_param(key3, charp, 0000);

static u32 key[4];

static int dev_open(struct inode *, struct file *);
static int dev_release(struct inode *, struct file *);
static ssize_t dev_read(struct file *, char *, size_t, loff_t *);
static ssize_t dev_write(struct file *, const char *, size_t, loff_t *);

static struct file_operations fops = {
	.open = dev_open,
	.read = dev_read,
	.write = dev_write,
	.release = dev_release,
};

static void encipher(u32 num_rounds, u32 v[2], const u32 key[4]){
	u32 i;
	u32 v0 = v[0], v1 = v[1], sum = 0, delta = 0x9E3779B9;

	for (i = 0; i < num_rounds; i++){
		v0 += (((v1 << 4) ^ (v1 >> 5)) + v1) ^ (sum + key[sum & 3]);
		sum += delta;
		v1 += (((v0 << 4) ^ (v0 >> 5)) + v0) ^ (sum + key[(sum>>11) & 3]);
	}
	v[0] = v0; v[1] = v1;
}

static void decipher(u32 num_rounds, u32 v[2], const u32 key[4]){
	u32 i;
	u32 v0 = v[0], v1 = v[1], delta = 0x9E3779B9, sum = delta * num_rounds;

	for (i = 0; i < num_rounds; i++){
		v1 -= (((v0 << 4) ^ (v0 >> 5)) + v0) ^ (sum + key[(sum>>11) & 3]);
		sum -= delta;
		v0 -= (((v1 << 4) ^ (v1 >> 5)) + v1) ^ (sum + key[sum & 3]);
	}
	v[0] = v0; v[1] = v1;
}

static int __init xtea_init(void){
	int ret;

	ret = kstrtou32(key0, 16, &key[0]);
	if (ret){
		printk(KERN_ALERT "XTEA Driver: key0 invalido ('%s')\n", key0);
		return ret;
	}
	ret = kstrtou32(key1, 16, &key[1]);
	if (ret){
		printk(KERN_ALERT "XTEA Driver: key1 invalido ('%s')\n", key1);
		return ret;
	}
	ret = kstrtou32(key2, 16, &key[2]);
	if (ret){
		printk(KERN_ALERT "XTEA Driver: key2 invalido ('%s')\n", key2);
		return ret;
	}
	ret = kstrtou32(key3, 16, &key[3]);
	if (ret){
		printk(KERN_ALERT "XTEA Driver: key3 invalido ('%s')\n", key3);
		return ret;
	}

	printk(KERN_INFO "XTEA Driver: Initializing the LKM\n");

	majorNumber = register_chrdev(0, DEVICE_NAME, &fops);
	if (majorNumber<0){
		printk(KERN_ALERT "XTEA Driver failed to register a major number\n");
		return majorNumber;
	}

	printk(KERN_INFO "XTEA Driver: registered correctly with major number %d\n", majorNumber);

	charClass = class_create(THIS_MODULE, CLASS_NAME);
	if (IS_ERR(charClass)){
		unregister_chrdev(majorNumber, DEVICE_NAME);
		printk(KERN_ALERT "XTEA Driver: failed to register device class\n");
		return PTR_ERR(charClass);
	}

	printk(KERN_INFO "XTEA Driver: device class registered correctly\n");

	charDevice = device_create(charClass, NULL, MKDEV(majorNumber, 0), NULL, DEVICE_NAME);
	if (IS_ERR(charDevice)){
		class_destroy(charClass);
		unregister_chrdev(majorNumber, DEVICE_NAME);
		printk(KERN_ALERT "XTEA Driver: failed to create the device\n");
		return PTR_ERR(charDevice);
	}

	printk(KERN_INFO "XTEA Driver: device class created correctly\n");

	return 0;
}

static void __exit xtea_exit(void){
	device_destroy(charClass, MKDEV(majorNumber, 0));
	class_unregister(charClass);
	class_destroy(charClass);
	unregister_chrdev(majorNumber, DEVICE_NAME);
	printk(KERN_INFO "XTEA Driver: goodbye from the LKM!\n");
}

static int dev_open(struct inode *inodep, struct file *filep){
	numberOpens++;
	printk(KERN_INFO "XTEA Driver: device has been opened %d time(s)\n", numberOpens);
	return 0;
}

static ssize_t dev_read(struct file *filep, char *buffer, size_t len, loff_t *offset){
	int error_count = 0;

	error_count = copy_to_user(buffer, message, size_of_message);

	if (error_count==0){
		printk(KERN_INFO "XTEA Driver: sent %d characters to the user\n", size_of_message);
		return (size_of_message=0);
	}
	else {
		printk(KERN_INFO "XTEA Driver: failed to send %d characters to the user\n", error_count);
		return -EFAULT;
	}
}

static ssize_t dev_write(struct file *filep, const char *buffer, size_t len, loff_t *offset){
	char kbuf[CMD_BUF_SIZE];
	char cmd[8];
	char data_hex[2*MAX_DATA_BYTES + 1];
	u8 data_bytes[MAX_DATA_BYTES];
	int data_size;
	int available_chars;
	int n, i, hi, lo;

	if (len >= sizeof(kbuf)){
		len = sizeof(kbuf) - 1;
	}

	if (copy_from_user(kbuf, buffer, len)){
		return -EFAULT;
	}
	kbuf[len] = '\0';

	n = sscanf(kbuf, "%7s %d %240s", cmd, &data_size, data_hex);

	if (n != 3){
		printk(KERN_INFO "XTEA Driver: comando invalido\n");
		return -EINVAL;
	}

	available_chars = strlen(data_hex);
	if (data_size > available_chars / 2){
		data_size = available_chars / 2;
	}

	for (i = 0; i < data_size; i++){
		hi = hex_to_bin(data_hex[2*i]);
		lo = hex_to_bin(data_hex[2*i + 1]);
		if (hi < 0 || lo < 0){
			printk(KERN_INFO "XTEA Driver: dados com caractere hexadecimal invalido\n");
			return -EINVAL;
		}
		data_bytes[i] = (hi << 4) | lo;
	}

	for (i = 0; i + 8 <= data_size; i += 8){
		u32 v[2];

		v[0] = (data_bytes[i] << 24) | (data_bytes[i+1] << 16) | (data_bytes[i+2] << 8) | data_bytes[i+3];
		v[1] = (data_bytes[i+4] << 24) | (data_bytes[i+5] << 16) | (data_bytes[i+6] << 8) | data_bytes[i+7];

		if (strcmp(cmd, "enc") == 0){
			encipher(32, v, key);
		} else if (strcmp(cmd, "dec") == 0){
			decipher(32, v, key);
		} else {
			printk(KERN_INFO "XTEA Driver: comando desconhecido '%s'\n", cmd);
			return -EINVAL;
		}

		data_bytes[i] = (v[0] >> 24) & 0xff;
		data_bytes[i+1] = (v[0] >> 16) & 0xff;
		data_bytes[i+2] = (v[0] >> 8) & 0xff;
		data_bytes[i+3] = v[0] & 0xff;
		data_bytes[i+4] = (v[1] >> 24) & 0xff;
		data_bytes[i+5] = (v[1] >> 16) & 0xff;
		data_bytes[i+6] = (v[1] >> 8) & 0xff;
		data_bytes[i+7] = v[1] & 0xff;
	}

	size_of_message = 0;
	for (i = 0; i < data_size; i++){
		size_of_message += sprintf(message + size_of_message, "%02x", data_bytes[i]);
	}

	printk(KERN_INFO "XTEA Driver: comando '%s' processado, %d bytes\n", cmd, data_size);

	return len;
}

static int dev_release(struct inode *inodep, struct file *filep){
	printk(KERN_INFO "XTEA Driver: device successfully closed\n");
	return 0;
}

module_init(xtea_init);
module_exit(xtea_exit);