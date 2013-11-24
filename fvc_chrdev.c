#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/cdev.h>

int major = 0, minor = 0;
struct cdev chr_dev;
char* mem_data;
int mem_size = 4096;
int data_len = 0;

int chrdev_open(struct inode* inode, struct file* filep)
{
	return 0;
}

ssize_t chrdev_write(struct file* filep, const char* __user buf, int count, loff_t* fpos)
{
	int pos = filep->f_pos;
	int iRet = 0;
	if(pos > mem_size)
	{
		return -1;
	}
	else if( pos + count > mem_size)
	{
		iRet = copy_from_user(mem_data + pos, buf, mem_size - pos);
		*fpos = mem_size;
		data_len = mem_size;
		return mem_size - pos;
	}
	else
	{
		iRet = copy_from_user(mem_data + pos, buf, count);
		*fpos += count;
		data_len += count;
		return count;
	}

	return -1;
}

ssize_t chrdev_read(struct file* filep, char* __user buf, int count, loff_t* fpos)
{
	int pos = filep->f_pos;
	int iRet = 0;

	if(pos >= data_len)
		return -1;
	else if(pos + count > data_len)
	{
		iRet = copy_to_user(buf, mem_data + pos, data_len - pos);
		*fpos = data_len;
		return data_len - pos;
	}
	else
	{
		iRet = copy_to_user(buf, mem_data + pos, count);
		*fpos += count;
		return count;
	}
	return -1;
}
struct file_operations fops = 
{
	.owner	= THIS_MODULE,
	.open	= chrdev_open,
	.write	= chrdev_write,
	.read	= chrdev_read,
};

int chrdev_init(void)
{
	dev_t dev_no;
	int iRet = 0;
	if(major)
	{
		dev_no = MKDEV(major, minor);
		iRet = register_chrdev_region(dev_no, 1, "fvc_chrdev");
	}
	else
		iRet = alloc_chrdev_region(&dev_no, 0, 1, "fvc_chrdev");
	if(iRet)
	{
		printk("regiter dev error\n");
		return -1;
	}
	major = MAJOR(dev_no);
	
	memset(&chr_dev, 0, sizeof(chr_dev));
	cdev_init(&chr_dev, &fops);
	chr_dev.owner = THIS_MODULE;
	chr_dev.ops = &fops;
	cdev_add(&chr_dev, dev_no, 1);

	mem_data = kmalloc(mem_size, GFP_KERNEL);
	if(mem_data == NULL)
		return -1;
	else
		memset(mem_data, 0, mem_size);
	
	return 0;
}

void chrdev_exit(void)
{
	dev_t dev_no;
	dev_no = MKDEV(major, minor);
	
	cdev_del(&chr_dev);
	unregister_chrdev_region(dev_no, 1);
	kfree(mem_data);
}

module_init(chrdev_init);
module_exit(chrdev_exit);
MODULE_LICENSE("Dual BSD/GPL");
