#include <linux/module.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

#define GLOBALMEM_SIZE 0X1000
#define MEM_CLEAR 0x1
#define GLOBALMEM_MAJOR 230

static int globalmem_major = GLOBALMEM_MAJOR;
module_param(globalmem_major, int, S_IRUGO);

struct globalmem_dev {
	struct cdev cdev;
	unsigned char mem[GLOBALMEM_SIZE];
};

struct globalmem_dev *globalmem_devp = NULL;

static int globalmem_open(struct inode *inode, struct file *filp) {
	return 0;
}

static int globalmem_release(struct inode *inode, struct file *filp) {
	return 0;
}

static long globalmem_ioctl(struct file *filp, unsigned int cmd, unsigned long arg) {
	return 0;
}

static ssize_t globalmem_read(struct file *filp, char __user *buf, size_t size, loff_t *ppos) {
	return 0;
}

static ssize_t globalmem_write(struct file *filp, const char __user *buf, size_t size, loff_t *ppos) {
	return 0;
}

static loff_t globalmem_llseek(struct file *filp, loff_t offset, int orig) {
	loff_t ret = 0;
	return ret;
}

static const struct file_operations globalmem_fops = {
	.owner = THIS_MODULE,
	.llseek = globalmem_llseek,
	.read = globalmem_read,
	.write = globalmem_write,
	.unlocked_ioctl = globalmem_ioctl,
	.open = globalmem_open,
	.release = globalmem_release,
};

static void globalmem_setup_cdec(struct globalmem_dev *dev, int index) {
	int err = -1;
	int devno = MKDEV(globalmem_major, index);

	cdev_init(&dev->cdev, &globalmem_fops);
	dev->cdev.owner = THIS_MODULE;
	err = cdev_add(&dev->cdev, devno, 1);
	if (err) {
		printk("Error %d adding globalmem %d\n", err, devno);
	}
}

static int __init globalmem_init(void) {
	int ret = -1;
	dev_t devno = MKDEV(globalmem_major, 0);

	if (globalmem_major) {
		ret = register_chrdev_region(devno, 1, "globalmem");
	}
	else {
		ret = alloc_chrdev_region(&devno, 0, 1, "globalmem");
		globalmem_major = MAJOR(devno);
	}
	if (ret < 0) {
		return ret;
	}
	globalmem_devp = kzalloc(sizeof(struct globalmem_dev), GFP_KERNEL);
	if (!globalmem_devp) {
		printk("error %d alloc globalmem_devp fail\n", ret);
		goto fail_malloc;
	}

	globalmem_setup_cdec(globalmem_devp, 0);
	printk(KERN_EMERG "globalmem module init finished\n");
	return 0;

fail_malloc:
	unregister_chrdev_region(devno, 1);
	return ret;
}
module_init(globalmem_init);

static void __exit globalmem_exit(void) {
	cdev_del(&globalmem_devp->cdev);
	kfree(globalmem_devp);
	unregister_chrdev_region(MKDEV(globalmem_major, 0), 1);
	printk(KERN_EMERG "globalmem module exit finished\n");
}
module_exit(globalmem_exit);

MODULE_AUTHOR("xiangrui.zhao");
MODULE_LICENSE("GPL v2");