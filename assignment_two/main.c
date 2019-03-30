#include <linux/init.h>
#include <linux/module.h>  
#include <linux/kernel.h>  
#include <linux/fs.h>      
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/blkdev.h>
#include <linux/genhd.h>
#include <linux/spinlock.h>
#include <linux/vmalloc.h>
#include <linux/hdreg.h>
#include <linux/errno.h>
#include <linux/string.h>



/*-------------------------------------------------*/
#define ARRAY_SIZE(a) (sizeof(a) / sizeof(*a))

#define SECTOR_SIZE 512
#define MBR_SIZE SECTOR_SIZE
#define MBR_DISK_SIGNATURE_OFFSET 440
#define MBR_DISK_SIGNATURE_SIZE 4
#define PARTITION_TABLE_OFFSET 446
#define PARTITION_ENTRY_SIZE 16 // sizeof(PartEntry)
#define PARTITION_TABLE_SIZE 64 // sizeof(PartTable)
#define MBR_SIGNATURE_OFFSET 510
#define MBR_SIGNATURE_SIZE 2
#define MBR_SIGNATURE 0xAA55
#define BR_SIZE SECTOR_SIZE
#define BR_SIGNATURE_OFFSET 510
#define BR_SIGNATURE_SIZE 2
#define BR_SIGNATURE 0xAA55
#define RB_SECTOR_SIZE 512
#define RB_DEVICE_SIZE 1024


#define MAJOR_NO 0
#define DOF_MINOR_CNT 2
#define DEVICE_NAME "SD_CARD"
#define NR_OF_SECTORS 1024
#define SECTOR_SIZE 512
#define CARD_CAPACITY  (NR_OF_SECTORS*SECTOR_SIZE)

int err=0;
/*....................................................*/

//MBR Partition table code

typedef struct
{
	unsigned char boot_type; // 0x00 - Inactive; 0x80 - Active (Bootable)
	unsigned char start_head;
	unsigned char start_sec:6;
	unsigned char start_cyl_hi:2;
	unsigned char start_cyl;
	unsigned char part_type;
	unsigned char end_head;
	unsigned char end_sec:6;
	unsigned char end_cyl_hi:2;
	unsigned char end_cyl;
	unsigned int abs_start_sec;
	unsigned int sec_in_part;
} PartEntry;

typedef PartEntry PartTable[4];

static PartTable def_part_table =
{
	{
		boot_type: 0x00,
		start_head: 0x00,
		start_sec: 0x2,
		start_cyl: 0x00,
		part_type: 0x83,
		end_head: 0x00,
		end_sec: 0x20,
		end_cyl: 0x09,
		abs_start_sec: 0x00000001,
		sec_in_part: 0x00000200
	},
	{
  		boot_type: 0x00,
       	 	start_head: 0x00,
        	start_sec: 0x1,
        	start_cyl: 0x14,
        	part_type: 0x83,
        	end_head: 0x00,
        	end_sec: 0x20,
        	end_cyl: 0x1F,
        	abs_start_sec: 0x00000280,
        	sec_in_part: 0x00000200
	},
	{},
	{},
};

static void copy_mbr(struct file *disk)
{
	unsigned char buff[512];
	int i=0;
	loff_t zero=0;
	unsigned char sign[2]={0x55,0xAA};
	char* dummy = kzalloc(446,GFP_KERNEL);
 	kernel_write(disk, dummy, 446,&disk->f_pos);	
	kernel_write(disk, &def_part_table, sizeof( PartEntry)*2,&disk->f_pos);
	kernel_write(disk,dummy ,32,&disk->f_pos);
	kernel_write(disk,sign,2,&disk->f_pos);
	kernel_read(disk,buff,512,&zero);
	for(i;i<512;i++)
		printk("%x\n",buff[i]);
}
 


/*..........................................................*/

//Block device creation

struct dof_read{
	struct work_struct read;
	u8 *buffer;
	unsigned int sectors;
	sector_t sector_off;
	struct file *disk;
	
}dof_read;

struct dof_write{
	struct work_struct write;
	u8 *buffer;
	unsigned int sectors;
	struct file *disk;
	sector_t sector_off;
}dof_write;



struct blkdev_private{
	struct request_queue *queue;
	struct gendisk *gd;
	spinlock_t lock;
};


struct request *req;

static int dof_open(struct block_device *bdev, fmode_t mode)
{
        unsigned unit = iminor(bdev->bd_inode);

        printk(KERN_INFO "dof: Device is opened\n");
        printk(KERN_INFO "dof: Inode number is %d\n", unit);

        if (unit > DOF_MINOR_CNT)
                return -ENODEV;
        return 0;
}

static void dof_close(struct gendisk *disk, fmode_t mode)
{
        printk(KERN_INFO "dof: Device is closed\n");
}


static struct block_device_operations blkdev_ops =
{
	.owner=	THIS_MODULE,
	.open= dof_open,
	.release=dof_close,
};

static struct blkdev_private *p_blkdev = NULL;



static int dof_transfer(struct request *req)
{

	int dir = rq_data_dir(req);
	printk("%d\n",dir);
	sector_t start_sector = blk_rq_pos(req);
	unsigned int sector_cnt = blk_rq_sectors(req);

	struct file *disk;
        disk=filp_open("/etc/sample.txt",O_RDWR,0666);
	if(disk==NULL)
		printk("File couldn't open\n");
	printk("Inside dof_transfer\n");
	#define BV_PAGE(bv) ((bv).bv_page)
	#define BV_OFFSET(bv) ((bv).bv_offset)
	#define BV_LEN(bv) ((bv).bv_len)
	struct bio_vec bv;

	struct req_iterator iter;

	sector_t sector_offset;
	unsigned int sectors;
	u8 *buffer;

	int ret = 0;

	sector_offset = 0;
	rq_for_each_segment(bv, req, iter)
	{
		buffer = page_address(BV_PAGE(bv)) + BV_OFFSET(bv);
		printk("Inside request for each segment\n");
		if (BV_LEN(bv) % SECTOR_SIZE != 0)
		{
			printk(KERN_ERR "dof: Should never happen: "
				"bio size (%d) is not a multiple of SECTOR_SIZE (%d).\n"
				"This may lead to data truncation.\n",
				BV_LEN(bv), SECTOR_SIZE);
			ret = -EIO;
		}
		sectors = BV_LEN(bv) / SECTOR_SIZE;
		printk(KERN_DEBUG "dof: Start Sector: %llu, Sector Offset: %llu; Buffer: %p; Length: %u sectors\n",
			(unsigned long long)(start_sector), (unsigned long long)(sector_offset), buffer, sectors);
		if (dir == WRITE) /* Write to the device */
		{
			printk("dof writing\n");
			dof_write.buffer =buffer;
			dof_write.sector_off=start_sector+sector_offset;
			dof_write.sectors=sectors;
			dof_write.disk=disk;
			schedule_work(&dof_write.write);
		}
		else /* Read from the device */
		{
			printk("dof reading\n");
                        dof_read.buffer =buffer;
                        dof_read.sector_off=start_sector+sector_offset;
                        dof_read.sectors=sectors;
                        dof_read.disk=disk;
                        schedule_work(&dof_read.read);
			
		}
		sector_offset += sectors;
	}
	if (sector_offset != sector_cnt)
	{
		printk(KERN_ERR "dof: bio info doesn't match with the request info");
		ret = -EIO;
	}
	filp_close(disk,NULL);
	return ret;
}


/*
 * Represents a block I/O request for us to execute
 */
static void dof_request(struct request_queue *q)
{
		struct request *req;
	int ret;
	req =  blk_fetch_request(q);	//fetch next request from req_queue
	while(req != NULL)
	{
		if(/*! blk_fs_request(req))*/req == NULL || blk_rq_is_passthrough(req))
		{
			printk(KERN_INFO "non FS request");
			__blk_end_request_all(req, -EIO);
			continue;
		}
		printk(KERN_ALERT "Inside request function\n");
		printk(KERN_ALERT "Target Sector No. %d ", req->__sector);
		 ret=dof_transfer(req);

		if ( ! __blk_end_request_cur(req, 0) ) {
//			ret=dof_transfer(req);
			req = blk_fetch_request(q);
		}
	}

}

static void read(struct work_struct *work)
{
	
	u8 *data;
	struct dof_read *mwp= container_of(work,struct dof_read,read);
	mwp->disk->f_pos=mwp->sector_off*512;
	printk("%d\n",kernel_read(mwp->disk,mwp->buffer,mwp->sectors*512,&(mwp->disk->f_pos)));
	data=mwp->buffer;
	printk(KERN_INFO "\n %s \n",*(data));
	return;
	
}

static void write(struct work_struct *work)
{
	
        struct dof_write *mwp= container_of(work,struct dof_write,write);
        mwp->disk->f_pos=mwp->sector_off*512;
        printk("%d\n",kernel_write(mwp->disk,mwp->buffer,mwp->sectors*512,&(mwp->disk->f_pos)));
        return;

}


int block_init(void)
{
	struct gendisk *dof_disk = NULL;
	struct file *disk;
	disk=filp_open("/etc/sample.txt",O_RDWR,0666);

	copy_mbr(disk);
	filp_close(disk,NULL);
	INIT_WORK(&dof_read.read,read);
	INIT_WORK(&dof_write.write,write);
	err = register_blkdev(0, "DOF_DISK");
	if (err < 0) 
		printk(KERN_WARNING "DiskonFILE: unable to get major number\n");
	
	p_blkdev = kmalloc(sizeof(struct blkdev_private),GFP_KERNEL);
	
	if(!p_blkdev)
	{
		printk("ENOMEM  at %d\n",__LINE__);
		return 0;
	}
	memset(p_blkdev, 0, sizeof(struct blkdev_private));

	spin_lock_init(&p_blkdev->lock);

	p_blkdev->queue = blk_init_queue(dof_request, &p_blkdev->lock);

	dof_disk = p_blkdev->gd = alloc_disk(2);
	if(!dof_disk)
	{
		kfree(p_blkdev);
		printk(KERN_INFO "alloc_disk failed\n");
		return 0;
	}
	

	dof_disk->major = err;
	dof_disk->first_minor = 0;
	dof_disk->fops = &blkdev_ops;
	dof_disk->queue = p_blkdev->queue;
	dof_disk->private_data = p_blkdev;
	strcpy(dof_disk->disk_name, "dof");
	set_capacity(dof_disk, NR_OF_SECTORS);//capacity of disk can be set and part  of gendisk ie no.of sec* sector size

	add_disk(dof_disk);	//after setting the disk properties add the disk
	printk(KERN_INFO "Registered disk\n");	
	return 0;	
}

void block_exit(void)
{
	struct gendisk *dof_disk = p_blkdev->gd;
	del_gendisk(dof_disk);
	blk_cleanup_queue(p_blkdev->queue);
	kfree(p_blkdev);
	flush_scheduled_work();
	//return;
}

module_init(block_init);
module_exit(block_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("SAMBIT");
MODULE_DESCRIPTION("DISK_ON_FILE");
