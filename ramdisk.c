#define FUSE_USE_VERSION 26

#include<fuse.h>
#include<stdio.h>
#include<string.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<errno.h>
#include<fcntl.h>
#include<stdlib.h>
#include<unistd.h>
#include<time.h>

int iid=0;
size_t ramdisk_size;

typedef struct iInfo
{
	int id;
	char *addr;
//	char *p_addr; 
	struct iInfo *parent;  
	size_t size;
	char *data;
	int type;	//1 for dir, 0 for file
//	int isopen;  	// 1 if open, 0 if close
	struct stat *meta;
	struct iInfo *link;
	

}iInfo;

//iInfo *filesystem_f=NULL, *filesystem_r=NULL;
iInfo *root=NULL;

time_t get_time(void)
{
	time_t t;
	time (&t);
	return t;
}

void viewFilesystem(void)
{
	iInfo *temp=root;
	printf("\n=======viewing filesystem:========\n");
	while(temp!=NULL)
	{
		if(temp->id==0)
			printf ("address: %s\t parent addr=root\t id: %d\n", temp->addr,temp->id );
		else
				printf ("address: %s\t parent addr=%s\t id: %d\n", temp->addr,temp->parent->addr,temp->id );
		temp=temp->link;
	}
	printf("==========================\n");
}

void createNode(iInfo *info)
{
//	viewFilesystem();
	iInfo *temp = root;
	while(temp->link != NULL)
	{
		temp = temp->link;
	}
	temp->link = info;
//	viewFilesystem();
}

void deleteNode(iInfo *info)
{
//	viewFilesystem();
	iInfo *temp=root;
	while(temp->link != NULL)
	{	
		if(strcmp(temp->link->addr,info->addr)==0)
		{
			iInfo *temp1=temp->link;
			temp->link=temp1->link;
//			printf("removing node %s\n",info->addr);
		}
		else
			temp= temp->link;
	}
//	viewFilesystem();
}

iInfo *pathCompare(const char *path)
{
	iInfo *temp = root;
	while(temp!=NULL)
	{
//		printf ("comparing %s with %s\n", temp->addr, path); 
		if(strcmp(temp->addr, path)==0)
			return temp;
		temp=temp->link;
	}
//	printf("no match found\n");
	return NULL;
}

iInfo *getParent(const char *path)
{
	char str1[1024] = "",str[1024] = "";
	strcpy(str1,path);
	int k = strlen(str1);
	int j = strlen(strrchr(str1, '/'));
	
	strncpy(str,path,(k-j));
//	printf("in get parent parent=%swith strlen=%i\n",str,(int)strlen(str));
	if((int)strlen(str) == 0)
	{
		return root;
	}
	iInfo *temp=root->link;
	while(temp != NULL)
	{
		if(strcmp(temp->addr,str)==0)
			return temp;
		temp=temp->link;
	}
	return NULL;
}

void root_dir_init(void)
{
	root=(iInfo *)malloc(sizeof(iInfo));
//	printf("creating root\n");

	root->id	= iid;
	root->addr	= "/";	
	root->parent	= NULL;
	root->size	= 4096;
//	root->data	= NULL;
	root->type	= 1;
//	root->isopen	= 0;
	root->link	= NULL;
	root->meta = (struct stat *)malloc(sizeof(struct stat));
	root->meta->st_mode    	= S_IFDIR | 0755;
	root->meta->st_nlink	= 2;
	root->meta->st_uid 	= getuid();
	root->meta->st_gid	= getgid();
	root->meta->st_size	= root->size;
//	root->meta->st_blksize	= 4096;
	root->meta->st_atime	= get_time();
	root->meta->st_mtime	= get_time();
	root->meta->st_ctime	= get_time();
//	viewFilesystem();
//	createNode(root);
//	printf("root created\n");
}



//http://engineering.facile.it/blog/eng/write-filesystem-fuse/

static int ramdisk_getattr(const char *path, struct stat *stbuf) 
{
//	printf("getattr path=%s\n",path);
	iInfo *temp;
//	memset(stbuf,0,sizeof(struct stat));
//	viewFilesystem();
	temp=pathCompare(path);
	
	if(temp==NULL)
		return -ENOENT;
	else
	{
//		printf ("returning %s\n", temp->addr);
		stbuf->st_mode	= temp->meta->st_mode;
		stbuf->st_nlink	= temp->meta->st_nlink;
		stbuf->st_uid	= temp->meta->st_uid; 
		stbuf->st_gid	= temp->meta->st_gid;
		stbuf->st_size	= temp->meta->st_size;
//		stbuf->st_blksize = temp->meta->st_blksize;
		stbuf->st_atime	= temp->meta->st_atime;
		stbuf->st_mtime	= temp->meta->st_mtime;
		stbuf->st_ctime	= temp->meta->st_ctime;
		return 0;
	}
}

int ramdisk_readdir(const char* path, void* buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info* fi)
{
//	printf("\nreaddir accessing path=%s\n",path);
//	(void) offset;
//	(void) fi;

	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);
	iInfo *temp = root->link;
	while(temp!=NULL)
	{
//		printf("temp->addr:%s temp->id:%d\n",temp->addr,temp->id);
		if(strcmp(temp->parent->addr, path) == 0)
		{
			char *str=malloc(sizeof(char) * strlen(path));
			strcpy(str,temp->addr);
			filler(buf, (strrchr(str,'/')+1) , NULL, 0);
//			printf("in filler, found %s\n", temp->addr);
		}
		temp=temp->link;
	}
  	return 0;
}

static int ramdisk_mkdir(const char* path, mode_t mode)
{
	iid++;
//	printf("mkdir path=%s\n",path);
	if(path==NULL || strcmp(path,"/")==0)
	{
		return -EPERM;
	}
	 else if(pathCompare(path)==NULL)
	{
//		printf ("case 2\n");
		iInfo *iparent=getParent(path);
//		printf("iparent->addr=%s, iparent->id=%d",iparent->addr, iparent->id);
		if(iparent != NULL)	
		{
			iInfo *temp=malloc(sizeof(iInfo));
//			printf("creating node\n");
		
			temp->id	= iid;
//			printf("mkdir creating new node: %s\n",path);
			temp->addr = malloc(sizeof(char) * strlen(path));
			strcpy(temp->addr, path);
//			temp->p_addr	= iparent->addr;
			temp->parent	= iparent;
			temp->size	= 0;
//			temp->data	= NULL;
			temp->type	= 1;
//			temp->isopen	= 0;
			temp->link	= NULL;
			temp->meta = malloc(sizeof(struct stat));
			temp->meta->st_mode    	= S_IFDIR | 0755;
			temp->meta->st_nlink	= 2;
			temp->meta->st_uid 	= getuid();
			temp->meta->st_gid	= getgid();
			temp->meta->st_size	= temp->size;
//			temp->meta->st_blksize	= 4096;
			temp->meta->st_atime	= get_time();
			temp->meta->st_mtime	= get_time();
			temp->meta->st_ctime	= get_time();
			
			temp->parent->meta->st_nlink++;
			createNode(temp);
//			viewFilesystem();
			return 0;
		}
		else
		{
			return -EPERM;
		}	
	}	
	else 
	{
		return -EACCES;
	}
}


int ramdisk_opendir(const char* path, struct fuse_file_info* fi)
{
//	printf("open dir path:%s\n",path);
	iInfo *temp=root;
	while(temp != NULL)
	{
		if(strcmp(temp->addr,path)==0)
			return 0;
		temp=temp->link;
	}
	return -ENOENT;
}


static int ramdisk_rmdir(const char *path)
{
//	printf("rmdir path:%s\n",path);
	iInfo *temp=pathCompare(path);
	if(temp !=NULL)
	{
		temp->parent->meta->st_nlink--;
		deleteNode(temp);
		free(temp);
	}

	return 0;
}

int ramdisk_create(const char *path, mode_t mode, struct fuse_file_info *fi)
{
	iid++;
//	printf("create path=%s\n",path);
	if(path==NULL || strcmp(path,"/")==0)
	{
		return -EPERM;
	}
	 else if(pathCompare(path)==NULL)
	{
//		printf ("case 2\n");
		iInfo *iparent=getParent(path);
//		printf("iparent->addr=%s, iparent->id=%d",iparent->addr, iparent->id);
		if(iparent != NULL)	
		{
			iInfo *temp=malloc(sizeof(iInfo));	
			temp->id	= iid;
//			printf("create creating new node: %s\n",path);
			temp->addr = malloc(sizeof(char) * strlen(path));
			strcpy(temp->addr, path);
			temp->parent	= iparent;
			temp->size	= 0;
			temp->data	= NULL;
			temp->type	= 0;
//			temp->isopen	= 0;
			temp->link	= NULL;
			temp->meta = malloc(sizeof(struct stat));
			temp->meta->st_mode    	= S_IFREG | 0777;
			temp->meta->st_nlink	= 1;
			temp->meta->st_uid 	= getuid();
			temp->meta->st_gid	= getgid();
			temp->meta->st_size	= temp->size;
//			temp->meta->st_blksize	= 4096;
			temp->meta->st_atime	= get_time();
			temp->meta->st_mtime	= get_time();
			temp->meta->st_ctime	= get_time();
			
			temp->parent->meta->st_nlink++;
			createNode(temp);
//			viewFilesystem();
			return 0;
		}
		else
		{
			return -EPERM;
		}	
	}	
	else 
	{
		return -EACCES;
	}
}

int ramdisk_unlink(const char *path)
{
//	printf("unlink path:%s\n",path);
	iInfo *temp=pathCompare(path);
	if(temp !=NULL)
	{
		temp->parent->meta->st_nlink--;
//		printf("unlink size=%lu\n",temp->size);
		ramdisk_size=ramdisk_size+temp->size;
//		printf("from unlink size:%lu\n",ramdisk_size);
		deleteNode(temp);
		free(temp);
	}

	return 0;

}

int ramdisk_open(const char *path, struct fuse_file_info *fi)
{
//	printf("open path:%s\n",path);
	iInfo *temp=root;
	while(temp != NULL)
	{
		if(strcmp(temp->addr,path)==0)
			return 0;
		temp=temp->link;
	}
	return -ENOENT;
}

int ramdisk_read(const char *path, char *buf, size_t size, off_t offset,struct fuse_file_info *fi)
{
//	printf("read path:%s\n",path);
//	printf("path:%s\tsize=%lu\t,offset:%lu\n",path,size,offset);
	iInfo *temp=pathCompare(path);
//	printf("Path compared\n");	
	if(temp != NULL)
	{
		if(temp->type == 1)
		return -EISDIR;
		else 
		{
			size_t len = strlen(temp->data);
//			printf("len of data=%ld",len);
			if (offset < len) 
			{
				if ((offset + size) > len)
					size = len - offset;
				memcpy(buf, temp->data + offset, size);
//				printf("buf = %s\n", buf);
			} else
				size = 0;
			return size;
		}
		
	}
	else 
		return -EBADF;

}

int ramdisk_write(const char *path, const char *src, size_t size, off_t offset,struct fuse_file_info *fi)
{
//	printf("write path:%s\t size=%lu\t offset:%lu",path,size,offset);
	iInfo *temp=pathCompare(path);
	if(temp != NULL)
	{
		if(temp->type == 1)
			return -EISDIR;
		else if(ramdisk_size<=0)
			return -ENOSPC;
		else 
		{
			size_t	len=0;
//			printf("data size=%lu\n",temp->size);				
			if(temp->data==NULL)
			{
				temp->data=malloc(size);
				temp->size=size;
			}
			else
			{
				len=temp->size;
//				printf("original size from append=%lu",len);
				if(offset<=len)
				{
					if((size+offset) > len)
					{
						temp->data=realloc(temp->data,size+offset);
						temp->size=size+offset;
//						printf("for increasing size:%lu",temp->size);
					}	
/*					else
					{
						temp->data=realloc(temp->data,size);
						temp->size=size;
					}
*/				}
			}
			memcpy(temp->data+offset,src,size);
			temp->meta->st_size=temp->size;
						
//			printf("temp->data: %s",temp->data);			
			ramdisk_size=ramdisk_size - temp->size + len;
//			printf("from write size=%lu\n",ramdisk_size);
			return size;
		}
		
	}
	else 
		return -EBADF;

}

int ramdisk_rename(const char *path, const char *src)
{
	iInfo *tempf=getParent(path);
	iInfo *tempt=getParent(src);
	
//	printf("from=%s\t to=%s\n",tempf->addr,tempt->addr);
	if(strcmp(tempf->addr,tempt->addr)==0)
	{
		iInfo *temp1=pathCompare(path);
//		printf("renaming %s",temp1->addr);
		strcpy(temp1->addr,src);
	}
	return 0;
}

int ramdisk_truncate(const char *path, off_t offset)
{
//	printf("path=%s\t offset=%lu",path,offset);	
	iInfo *temp=pathCompare(path);
	if(temp != NULL)
	{
		temp->data=realloc(temp->data,offset);	
	}
	return 0;
}

int ramdisk_utimens(const char* path, const struct timespec ts[2])
{
	return 0;
}

static struct fuse_operations ramdisk_func = {
	.getattr	= ramdisk_getattr,
//	.fgetattr	= ramdisk_fgetattr,
	.mkdir		= ramdisk_mkdir,
	.rmdir		= ramdisk_rmdir,
	.opendir	= ramdisk_opendir,
	.readdir	= ramdisk_readdir,
	.open		= ramdisk_open,
	.read		= ramdisk_read,
	.create 	= ramdisk_create,
//	.flush		= ramdisk_flush,
	.write 		= ramdisk_write , 
	.unlink		= ramdisk_unlink,
	.utimens	= ramdisk_utimens,
	.truncate	= ramdisk_truncate,
	.rename		= ramdisk_rename,
};


int main(int argc, char **argv)
{

//	printf("start\n");
//	printf("%s,%s,%s",argv[0],argv[1],argv[2]);
	if(argc != 3)
	{
		printf("improper input format\n");
		return 1;
	}
	if (atoi(argv[2]) <= 1)
	{
		printf("less size allocated\n");
		return -ENOENT;
	}
	ramdisk_size=(size_t)(atoi(argv[2])*1024*1024);
	root_dir_init();
	return fuse_main(argc-1, argv, &ramdisk_func, NULL);
}
