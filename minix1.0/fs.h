#pragma once
#include"ext2/list.h"
#include"ext2/type.h"


struct rw_semaphore { u_int count;};
struct semaphore { u_int count; };
extern struct list_head super_blocks;
//extern spinlock_t sb_lock;
static void down(struct semaphore* lock) { --lock->count; }
static void up(struct semaphore* lock) { ++lock->count; }



struct inode {
	struct hlist_node	i_hash;
	struct list_head	i_list;
	struct list_head	i_sb_list;
	struct list_head	i_dentry;
	unsigned long		i_ino;
	atomic_t		    i_count;
	umode_t			    i_mode;
	unsigned int		i_nlink;
	uid_t			    i_uid;
	gid_t			    i_gid;
	dev_t			    i_rdev;
	loff_t			    i_size;
	struct timespec		i_atime;
	struct timespec		i_mtime;
	struct timespec		i_ctime;
	unsigned int		i_blkbits;
	unsigned long		i_blksize;
	unsigned long		i_version;
	unsigned long		i_blocks;
	unsigned short          i_bytes;
	unsigned char		i_sock;
	spinlock_t		i_lock;	/* i_blocks, i_bytes, maybe i_size */
	struct semaphore	i_sem;
	struct rw_semaphore	i_alloc_sem;
	struct inode_operations	*i_op;
	struct file_operations	*i_fop;	/* former ->i_op->default_file_ops */
	struct super_block	*i_sb;
	struct file_lock	*i_flock;
	struct address_space	*i_mapping;
	struct address_space	i_data;
#ifdef CONFIG_QUOTA
	struct dquot		*i_dquot[MAXQUOTAS];
#endif
	/* These three should probably be a union */
	struct list_head	i_devices;
	struct pipe_inode_info	*i_pipe;
	struct block_device	*i_bdev;
	struct cdev		*i_cdev;
	int			i_cindex;

	__u32			i_generation;

#ifdef CONFIG_DNOTIFY
	unsigned long		i_dnotify_mask; /* Directory notify events */
	struct dnotify_struct	*i_dnotify; /* for directory notifications */
#endif

	unsigned long		i_state;
	unsigned long		dirtied_when;	/* jiffies of first dirtying */

	unsigned int		i_flags;

	atomic_t		i_writecount;
	void			*i_security;
	union {
		void		*generic_ip;
	} u;
#ifdef __NEED_I_SIZE_ORDERED
	seqcount_t		i_size_seqcount;
#endif
};


struct file {
	struct list_head	f_list;
	struct dentry		*f_dentry;
	struct vfsmount         *f_vfsmnt;
	struct file_operations	*f_op;
	atomic_t		f_count;
	unsigned int 		f_flags;
	mode_t			f_mode;
	int			f_error;
	loff_t			f_pos;
	struct fown_struct	f_owner;
	unsigned int		f_uid, f_gid;
	struct file_ra_state	f_ra;

	size_t			f_maxcount;
	unsigned long		f_version;
	void			*f_security;

	/* needed for tty driver, and maybe others */
	void			*private_data;

#ifdef CONFIG_EPOLL
	/* Used by fs/eventpoll.c to link all the hooks to this file */
	struct list_head	f_ep_links;
	spinlock_t		f_ep_lock;
#endif /* #ifdef CONFIG_EPOLL */
	struct address_space	*f_mapping;
}


#define sb_entry(list)	list_entry((list), struct super_block, s_list)
#define S_BIAS (1<<30)
struct super_block {
	struct list_head	s_list;		/* Keep this first */                     // 链表头
	dev_t			    s_dev;		/* search index; _not_ kdev_t */          // 设备号
	unsigned long		s_blocksize;                                          // 本文件系统的数据块大小（字节）
	unsigned long		s_old_blocksize;                                      // ? ? ?
	unsigned char		s_blocksize_bits;                                     // 数据块大小的值所占的位数
	unsigned char		s_dirt;                                               // 修改标志
	unsigned long long	s_maxbytes;	/* Max file size */                       // 文件的最大长度（字节）
	struct file_system_type	*s_type;                                          // 文件系统的类型
	struct super_operations	*s_op;                                            // 指向某个特定的具体文件系统的用于超级块操作的函数集合
	struct dquot_operations	*dq_op;                                           // 磁盘限额方法
	struct quotactl_ops	*s_qcop;                                              // 限额控制方法 ? ? ?
	struct export_operations *s_export_op;                                    // 导出方法
	unsigned long		s_flags;                                              // 文件系统的超级块的状态位
	unsigned long		s_magic;                                              // 区别于其他文件系统的标识--文件系统魔数
	struct dentry		*s_root;                                              // 指向该具体文件系统安装目录的目录项
	struct rw_semaphore	s_umount;                                             // 文件系统卸载时候用到的读写信号量 -- XXX
	struct semaphore	s_lock;                                               // 锁标志位，若置该位，则其它进程不能对该超级块操作
	int			s_count;                                                      // 对超级块的使用计数
	int			s_syncing;                                                    // 文件系统的同步标记位
	int			s_need_sync_fs;                                               // 需要同步的标记位
	atomic_t    s_active;                                                     // ？？？
	void                    *s_security;                                      // ？？？
	struct xattr_handler	**s_xattr;                                        // 属性操作结构体

	struct list_head	s_inodes;	/* all inodes */                          // 该文件系统的所有inode形成的链表
	struct list_head	s_dirty;	/* dirty inodes */                        // 已修改的索引节点inode形成的链表，
	struct list_head	s_io;		/* parked for writeback */                // 所有这个文件系统的将要写回设备的inode都在这个链表上
	struct hlist_head	s_anon;		/* anonymous dentries for (nfs) exporting */ // ？？？
	struct list_head	s_files;                                              // 所有的已经打开文件的链表，这个file和实实在在的进程相关的

	struct block_device	*s_bdev;                                              // 指向文件系统被安装的块设备
	struct list_head	s_instances;                                          // 同类型文件系统的所有实例通过该指针形成链表
	//struct quota_info	s_dquot;	/* Diskquota specific options */

	int			s_frozen;
	//wait_queue_head_t	s_wait_unfrozen;

	char s_id[32];				/* Informational name */                      // 

	void 			*s_fs_info;	/* Filesystem private info */                 // 指向实际挂载文件系统的超级块

								/*
								* The next field is for VFS *only*. No filesystems have any business
								* even looking at it. You had been warned.
								*/
	struct semaphore s_vfs_rename_sem;	/* Kludge */

										/* Granuality of c/m/atime in ns.
										Cannot be worse than a second */
	u32		   s_time_gran;
};


/*
* Superblock locking.
*/
static inline void lock_super(struct super_block * sb)
{
	down(&sb->s_lock);
}

static inline void unlock_super(struct super_block * sb)
{
	up(&sb->s_lock);
}




struct file_system_type {
	const char *name;                               // 文件系统的名字
	int fs_flags;                                   // 文件系统类型标志的bitmap
	struct super_block *(*get_sb) (struct file_system_type *, int,  // 在安装文件时，会调用get_sb()从磁盘中读取超级块
		const char *, void *);
	void(*kill_sb) (struct super_block *);                          // 卸载文件系统时，会调用此函数做一些清理工作
	struct module *owner;                                           // 指向拥有这个结构的模块，如果一个文件系统被编译进内核，则该字段为NULL
	struct file_system_type * next;                                 // 形成文件系统类型链表
	struct list_head fs_supers;                                     // 同一种文件类型的超级块形成一个链表，fs_supers是这个链表的头
};