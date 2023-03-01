#include <linux/module.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

#define GLOBALMEM_SIZE 0X1000
#define MEM_CLEAR 0x1
#define GLOBALMEM_MAJOR 230
#define CDEV_ADD_ERR -10
#define CLASS_CREATE_ERR -11
#define DEVICE_CREATE_ERR -12
#define GLOBALMEM_SETUP_CDEV_OK 0

static int globalmem_major = GLOBALMEM_MAJOR;
module_param(globalmem_major, int, S_IRUGO);

struct globalmem_dev {
	struct cdev cdev;		//cdev
	unsigned char mem[GLOBALMEM_SIZE];
	struct class *class;
	struct device *device;
};

struct globalmem_dev *globalmem_devp = NULL;

static int globalmem_open(struct inode *inode, struct file *filp) {
	filp->private_data = globalmem_devp;
	return 0;
}

static int globalmem_release(struct inode *inode, struct file *filp) {
	return 0;
}

static long globalmem_ioctl(struct file *filp, unsigned int cmd, unsigned long arg) {
	struct globalmem_dev *dev = filp->private_data;
	switch (cmd) {
	case MEM_CLEAR:
		memset(dev->mem, 0, GLOBALMEM_SIZE);
		printk(KERN_INFO "globalmem is set to zero\n");
		break;

	default:
		return -EINVAL;
	}

	return 0;
}

static ssize_t globalmem_read(struct file *filp, char __user *buf, size_t size, loff_t *ppos) {
	unsigned long p = *ppos;
	unsigned int count = size;
	int ret = 0;
	struct globalmem_dev *dev = filp->private_data;

	if (p >= GLOBALMEM_SIZE)
		return 0;
	if (count > GLOBALMEM_SIZE - p)
		count = GLOBALMEM_SIZE - p;

	if (copy_to_user(buf, dev->mem + p, count)) {
		ret = -EFAULT;
	} else {
		*ppos += count;
		ret = count;

		printk(KERN_EMERG "read %u bytes(s) from %lu\n", count, p);
	}

	return ret;
}

static ssize_t globalmem_write(struct file *filp, const char __user *buf, size_t size, loff_t *ppos) {
	unsigned long p = *ppos;
	unsigned int count = size;
	int ret = 0;
	struct globalmem_dev *dev = filp->private_data;

	if (p >= GLOBALMEM_SIZE)
		return 0;
	if (count > GLOBALMEM_SIZE - p)
		count = GLOBALMEM_SIZE - p;

	if (copy_from_user(dev->mem + p, buf, count))
		ret = -EFAULT;
	else {
		*ppos += count;
		ret = count;

		printk(KERN_EMERG "written %u bytes(s) from %lu\n", count, p);
	}

	return ret;
}

static loff_t globalmem_llseek(struct file *filp, loff_t offset, int orig) {
	loff_t ret = 0;
	switch (orig) {
	case 0:
		if (offset < 0) {
			ret = -EINVAL;
			break;
		}
		if ((unsigned int)offset > GLOBALMEM_SIZE) {
			ret = -EINVAL;
			break;
		}
		filp->f_pos = (unsigned int)offset;
		ret = filp->f_pos;
		break;
	case 1:
		if ((filp->f_pos + offset) > GLOBALMEM_SIZE) {
			ret = -EINVAL;
			break;
		}
		if ((filp->f_pos + offset) < 0) {
			ret = -EINVAL;
			break;
		}
		filp->f_pos += offset;
		ret = filp->f_pos;
		break;
	default:
		ret = -EINVAL;
	break;
	}
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

static int globalmem_setup_cdec(struct globalmem_dev *dev, int index) {
	int ret = 0;
	int devno = MKDEV(globalmem_major, index);

	cdev_init(&dev->cdev, &globalmem_fops);
	dev->cdev.owner = THIS_MODULE;
	ret = cdev_add(&dev->cdev, devno, 1);
	if (ret) {
		ret = CDEV_ADD_ERR;
		printk(KERN_EMERG "Error %d cdev_add fail\n", ret);
		goto error_handle;
	}

	dev->class = class_create(THIS_MODULE, "globalmemclass");
	if(IS_ERR(dev->class)) {
		ret = CLASS_CREATE_ERR;
		cdev_del(&dev->cdev);
		printk(KERN_EMERG "Error %d class_create fail\n", ret);
		goto error_handle;
	}

	dev->device = device_create(dev->class, NULL, devno, NULL, "globalmem");
	if (IS_ERR(dev->device)) {
		ret = DEVICE_CREATE_ERR;
		class_destroy(dev->class);
		cdev_del(&dev->cdev);
		goto error_handle;;
	}
	return GLOBALMEM_SETUP_CDEV_OK;

error_handle:
	return ret;
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
		printk(KERN_EMERG "error %d alloc globalmem_devp fail\n", ret);
		goto fail_malloc;
	}

	ret = globalmem_setup_cdec(globalmem_devp, 0);
	if (ret != GLOBALMEM_SETUP_CDEV_OK) {
		kfree(globalmem_devp);
		printk(KERN_EMERG "error %d globalmem_setup_cdec fail\n", ret);
		goto fail_malloc;
	}
	printk(KERN_EMERG "globalmem module init finished\n");
	return 0;

fail_malloc:
	unregister_chrdev_region(devno, 1);
	return ret;
}
module_init(globalmem_init);

static void __exit globalmem_exit(void) {
	device_del(globalmem_devp->device);
	class_destroy(globalmem_devp->class);
	cdev_del(&globalmem_devp->cdev);
	kfree(globalmem_devp);
	unregister_chrdev_region(MKDEV(globalmem_major, 0), 1);
	
	printk(KERN_EMERG "globalmem module exit finished\n");
}
module_exit(globalmem_exit);

MODULE_AUTHOR("xiangrui.zhao");
MODULE_LICENSE("GPL v2");