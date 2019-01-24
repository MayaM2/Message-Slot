#undef __KERNEL__
#define __KERNEL__
#undef MODULE
#define MODULE

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/string.h>
#include <linux/slab.h>

MODULE_LICENSE("GPL");
#include "message_slot.h"

static int num_channels=0;
static int dev_open_flag = 0;

typedef struct channel_node{
	unsigned long channel_id;
	int minor;
	char message[BUFFER_LENGTH];
	int ever_written;
	struct channel_node * next;
}channel_node;

struct chardev_info
{
  spinlock_t lock;
};

typedef struct minor_node{
	int minor;
	struct minor_node * next;
}minor_node;

static channel_node * head = NULL;
struct minor_node * minors = NULL;

static struct chardev_info device_info;


//functions which will be used to free memory
void delete_channel_list(struct channel_node * head){
	if (head == NULL)
		return;
	delete_channel_list(head->next);
	kfree(head);
}

void delete_minors(struct minor_node * mhead){
	if(mhead==NULL)
		return;
	delete_minors(mhead->next);
	kfree(mhead);
}

// define fops:
static int device_open(struct inode * inode, struct file * file){
	minor_node * currentm = NULL;
	currentm = minors;
	if(minors == NULL){
		//case first file opened
		minors = kmalloc(1*sizeof(channel_node),GFP_KERNEL);
		if(!minors){ //allocation failed
			return -ENOMEM;
		}
		minors->minor = iminor(inode);
		minors->next = NULL;
		return SUCCESS;
	}
	//case there are already files opened
	//check if this minor already exists
	while(currentm!=NULL){
		if(currentm->minor == iminor(inode)){
			return SUCCESS;
		}
		currentm = currentm->next;
	}
	//add to end of list
	currentm = minors;
	while(currentm->next!=NULL){
		currentm = currentm->next;
	}
	currentm->next = kmalloc(1*sizeof(channel_node),GFP_KERNEL);
	if(!(currentm->next)){
		return -ENOMEM;
	}
	currentm->next->minor = iminor(inode);
	currentm->next->next = NULL;
	return SUCCESS;
}

static ssize_t device_read(struct file *file, char *buffer, size_t len, loff_t *offset){
	unsigned long flags;
	int i=0,j=0;
	int my_minor = iminor(file->f_path.dentry->d_inode);
	unsigned long my_channel = (unsigned long) (file ->private_data);
	channel_node * searcher =  NULL;
	searcher = head;

	for(i=0; i<num_channels; i++){ //case we find the right node
		if(((searcher->minor) == my_minor)&&((searcher->channel_id)== my_channel)){
			if(searcher->ever_written==0){
				//case mo message here
				return -EWOULDBLOCK;
			}
			if(len<strlen(searcher->message)+1){
				return -ENOSPC;
			}
			//copy my_message to user
			spin_lock_irqsave(&device_info.lock, flags);
			  if( 1 == dev_open_flag )
			  {
			    spin_unlock_irqrestore(&device_info.lock, flags);
			    return -EBUSY;
			  }
			  ++dev_open_flag;

			for( j = 0; j < len && j < BUFFER_LENGTH; ++j ){
				put_user(searcher->message[j],&buffer[j]);
			}
			//put_user('\0',&buffer[j]);
			--dev_open_flag;
			spin_unlock_irqrestore(&device_info.lock, flags);
			return len;
		}
		searcher = searcher->next;
	}
	//case no such channel exist or other problem
	return -EINVAL;
}


static ssize_t device_write(struct file *file, const char *buffer, size_t len, loff_t *offset){
	unsigned long flags;
	int i=0,j=0;
	int my_minor = iminor(file->f_path.dentry->d_inode);
	unsigned long my_channel = (unsigned long) (file ->private_data);
	channel_node * searcher =  NULL;
	searcher = head;

	//now we will find the file's node:
	for(i=0; i<num_channels; i++){
		if(((searcher->minor) == my_minor)&&((searcher->channel_id)== my_channel)){
			//case found channel of file
			if ((len==0) || (len>BUFFER_LENGTH)){ //size of message is not valid
				return -EMSGSIZE;
			}
			spin_lock_irqsave(&device_info.lock, flags);
			  if( 1 == dev_open_flag )
			  {
				spin_unlock_irqrestore(&device_info.lock, flags);
				return -EBUSY;
			  }
			  ++dev_open_flag;
			//write message
			for( j = 0; j < len && j < BUFFER_LENGTH; ++j ){
				get_user(searcher->message[j],&buffer[j]);
			}
			searcher->message[j]='\0';
			searcher->ever_written = 1;
			--dev_open_flag;
			spin_unlock_irqrestore(&device_info.lock, flags);
			return len;
		}
		searcher = searcher->next;
	}
	return -EINVAL; //no channel in file found
}

static long device_ioctl(struct file* file, unsigned int ioctl_command_id, unsigned long  ioctl_param){
	int file_exists = 0;
	int i=0;
	int my_minor = iminor(file->f_path.dentry->d_inode);
	unsigned long my_channel = ioctl_param;
	channel_node * searcher =  NULL;
	minor_node * currentm = NULL;
	searcher = head;

	if( MSG_SLOT_CHANNEL == ioctl_command_id)
	  {
		if(ioctl_param==0){
			return -EINVAL;
		}
		// associate channel id with fd
		file-> private_data = (void*) ioctl_param;

		//case no such file had ever opened:
		currentm = minors;
		while(currentm!=NULL){
			if(currentm->minor == my_minor){
				file_exists =1;
			}
			currentm = currentm->next;
		}
		if(file_exists==0){
			return -EINVAL;
		}

		//case first channel on first file
		if(head==NULL){
			head = kmalloc(1*sizeof(channel_node),GFP_KERNEL);
			if(!head){ //allocation failed
				return -ENOMEM;
			}
			//case first allocation successful, insert minor number
			(head->channel_id)= my_channel;
			(head-> minor) = my_minor;
			(head -> ever_written) = 0;
			(head->next)=NULL;
			num_channels = 1;
			return SUCCESS;
		}

		//case channel id on file already exist - return success
		for(i=0; i<num_channels; i++){
			if(((searcher->minor) == my_minor)&&((searcher->channel_id)== my_channel)){
				return SUCCESS;
			}
			searcher = searcher->next;
		}

		//case channel id on file does not exist yet - add node to end of list
		searcher = head;
		while(searcher->next !=NULL){
			searcher = searcher->next;
		}
		searcher->next = kmalloc(1*sizeof(channel_node),GFP_KERNEL);
		if(!(searcher->next)){
			return -ENOMEM;
		}
		searcher->next->channel_id = my_channel;
		searcher->next->minor = my_minor;
		searcher->next->ever_written = 0;
		searcher->next->next = NULL;
		num_channels++;
		return SUCCESS;
	  }
	return -EINVAL;
}

struct file_operations Fops =
{
   .open = device_open,
   .read = device_read,
   .write = device_write,
   .unlocked_ioctl = device_ioctl,
};

// Initialize the module - Register the character device
static int __init my_init(void){
	int majorNumber;
	memset( &device_info, 0, sizeof(struct chardev_info) );
	spin_lock_init( &device_info.lock );

	majorNumber = register_chrdev(MAJOR_NUM, DEVICE_RANGE_NAME, &Fops);
	if (majorNumber<0){
		printk(KERN_ERR "message_slot: failed to register a major number\n");
		return majorNumber;
	}
	printk(KERN_INFO "message_slot: registered major number %d\n", MAJOR_NUM);

	return 0;
}

//-----------------------------------------------------------
static void __exit my_cleanup(void){
	// free memory
	//free all channel nodes
	delete_channel_list(head);
	//free minors list
	delete_minors(minors);

	// unregister device
	unregister_chrdev(MAJOR_NUM, DEVICE_RANGE_NAME);
}

// and finally:
module_init(my_init);
module_exit(my_cleanup);
