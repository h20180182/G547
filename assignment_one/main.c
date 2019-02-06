#include<linux/kernel.h>
#include<linux/init.h>
#include<linux/module.h>
#include<linux/kdev_t.h>
#include<linux/fs.h>
#include<linux/cdev.h>
#include<linux/version.h>
#include<linux/uaccess.h>
#include<linux/random.h>
#include<linux/slab.h>

unsigned int i;
struct file *fp;
char *buf_ent;
static dev_t first;
static struct class *cls;
static struct cdev accel_cdev;

//Driver call function

static int accel_open(struct inode *i,struct file *f)
{
	printk(KERN_INFO "accelerometer: open()\n");
	return 0;
}

static int accel_close(struct inode *i,struct file *f)
{
	printk(KERN_INFO "accelerometer: close()\n");
	return 0;
}

 

static ssize_t accel_read (struct file *pfile, char __user *pbuffer, size_t len, loff_t *offset){
int* tep;
printk(KERN_INFO "accelerometer: read()\n");
	buf_ent=kmalloc(4,GFP_KERNEL);
	tep = (int*)buf_ent;	
	//true random generate
	fp = filp_open("/dev/random",O_NONBLOCK,0);
	if(fp==NULL){printk(KERN_INFO "couldn't open /dev/random\n");return -EFAULT;}
	
	if(!(fp->f_op->read(fp,buf_ent,2,&fp->f_pos)))
	{ printk(KERN_INFO "couldn't read /dev/random \n");return -EFAULT;}
	*tep =*tep & (0x03ff);

	printk(KERN_INFO "random No: %u \n",*tep);
	if(copy_to_user(pbuffer, tep,4)) printk(KERN_INFO "Could not be sent\n");
	return 0;
}

static ssize_t accel_write(struct file *f,const char __user *buf,size_t len,loff_t *off)
{
	printk(KERN_INFO "accelerometer: write()\n");
	return len;
}
static struct file_operations fops =
                                {
                                        .owner= THIS_MODULE,
                                        .open= accel_open,
                                        .release= accel_close,
                                        .read= accel_read,
                                        .write= accel_write
                                };

static int __init accelchar_init(void)
{
	printk(KERN_INFO "accelerometer registration\n");
//step-1 reserve <major,minor>
	if(alloc_chrdev_region(&first,0,3,"ADXL")<0)
	{
		return -1;
	}
	printk(KERN_INFO"<Major,Minor>:<%d,%d>\n",MAJOR(first),MINOR(first));

//step-2 creation of dev file
	if((cls=class_create(THIS_MODULE,"accel_chardev"))==NULL)
	{
		unregister_chrdev_region(first,3);
		return -1;
	}
	if(device_create(cls,NULL,MKDEV(MAJOR(first),0),NULL,"adxl_x")==NULL)
	{
		class_destroy(cls);
		unregister_chrdev_region(first,1);
		return -1;
	}
	if(device_create(cls,NULL,MKDEV(MAJOR(first),1),NULL,"adxl_y")==NULL)
	{
		class_destroy(cls);
		unregister_chrdev_region(first,1);
		return -1;
	}
	if(device_create(cls,NULL,MKDEV(MAJOR(first),2),NULL,"adxl_z")==NULL)
	{
		class_destroy(cls);
		unregister_chrdev_region(first,1);
		return -1;
	}
//Step-3 Link fops and cdev to device node
	cdev_init(&accel_cdev,&fops);
	if(cdev_add(&accel_cdev, first, 3)==-1)
	
	for(i=0;i<=2;i++)
	{
		unregister_chrdev_region(MKDEV(MAJOR(first),i),3);
	}
	return 0;

}

static void __exit accelchar_exit(void)
{
	cdev_del(&accel_cdev);
for(i=0;i<=2;i++)
	{
		device_destroy(cls,MKDEV(MAJOR(first),i));
	}
	
        class_destroy(cls);
	for(i=0;i<=2;i++)
	{
		unregister_chrdev_region(MKDEV(MAJOR(first),i),1);
	}
	printk(KERN_INFO "accelerometer unregistered\n");
	
}

module_init(accelchar_init);
module_exit(accelchar_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("sambit<sambitpatra5@gmail.com>");
MODULE_DESCRIPTION("Accelerometer ADXL device file");

