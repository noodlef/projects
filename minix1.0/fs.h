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
	struct list_head	s_list;		/* Keep this first */                     // ����ͷ
	dev_t			    s_dev;		/* search index; _not_ kdev_t */          // �豸��
	unsigned long		s_blocksize;                                          // ���ļ�ϵͳ�����ݿ��С���ֽڣ�
	unsigned long		s_old_blocksize;                                      // ? ? ?
	unsigned char		s_blocksize_bits;                                     // ���ݿ��С��ֵ��ռ��λ��
	unsigned char		s_dirt;                                               // �޸ı�־
	unsigned long long	s_maxbytes;	/* Max file size */                       // �ļ�����󳤶ȣ��ֽڣ�
	struct file_system_type	*s_type;                                          // �ļ�ϵͳ������
	struct super_operations	*s_op;                                            // ָ��ĳ���ض��ľ����ļ�ϵͳ�����ڳ���������ĺ�������
	struct dquot_operations	*dq_op;                                           // �����޶��
	struct quotactl_ops	*s_qcop;                                              // �޶���Ʒ��� ? ? ?
	struct export_operations *s_export_op;                                    // ��������
	unsigned long		s_flags;                                              // �ļ�ϵͳ�ĳ������״̬λ
	unsigned long		s_magic;                                              // �����������ļ�ϵͳ�ı�ʶ--�ļ�ϵͳħ��
	struct dentry		*s_root;                                              // ָ��þ����ļ�ϵͳ��װĿ¼��Ŀ¼��
	struct rw_semaphore	s_umount;                                             // �ļ�ϵͳж��ʱ���õ��Ķ�д�ź��� -- XXX
	struct semaphore	s_lock;                                               // ����־λ�����ø�λ�����������̲��ܶԸó��������
	int			s_count;                                                      // �Գ������ʹ�ü���
	int			s_syncing;                                                    // �ļ�ϵͳ��ͬ�����λ
	int			s_need_sync_fs;                                               // ��Ҫͬ���ı��λ
	atomic_t    s_active;                                                     // ������
	void                    *s_security;                                      // ������
	struct xattr_handler	**s_xattr;                                        // ���Բ����ṹ��

	struct list_head	s_inodes;	/* all inodes */                          // ���ļ�ϵͳ������inode�γɵ�����
	struct list_head	s_dirty;	/* dirty inodes */                        // ���޸ĵ������ڵ�inode�γɵ�����
	struct list_head	s_io;		/* parked for writeback */                // ��������ļ�ϵͳ�Ľ�Ҫд���豸��inode�������������
	struct hlist_head	s_anon;		/* anonymous dentries for (nfs) exporting */ // ������
	struct list_head	s_files;                                              // ���е��Ѿ����ļ����������file��ʵʵ���ڵĽ�����ص�

	struct block_device	*s_bdev;                                              // ָ���ļ�ϵͳ����װ�Ŀ��豸
	struct list_head	s_instances;                                          // ͬ�����ļ�ϵͳ������ʵ��ͨ����ָ���γ�����
	//struct quota_info	s_dquot;	/* Diskquota specific options */

	int			s_frozen;
	//wait_queue_head_t	s_wait_unfrozen;

	char s_id[32];				/* Informational name */                      // 

	void 			*s_fs_info;	/* Filesystem private info */                 // ָ��ʵ�ʹ����ļ�ϵͳ�ĳ�����

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
	const char *name;                               // �ļ�ϵͳ������
	int fs_flags;                                   // �ļ�ϵͳ���ͱ�־��bitmap
	struct super_block *(*get_sb) (struct file_system_type *, int,  // �ڰ�װ�ļ�ʱ�������get_sb()�Ӵ����ж�ȡ������
		const char *, void *);
	void(*kill_sb) (struct super_block *);                          // ж���ļ�ϵͳʱ������ô˺�����һЩ������
	struct module *owner;                                           // ָ��ӵ������ṹ��ģ�飬���һ���ļ�ϵͳ��������ںˣ�����ֶ�ΪNULL
	struct file_system_type * next;                                 // �γ��ļ�ϵͳ��������
	struct list_head fs_supers;                                     // ͬһ���ļ����͵ĳ������γ�һ������fs_supers����������ͷ
};