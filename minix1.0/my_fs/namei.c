#include"file_sys.h"
#include"fcntl.h"
#include<string.h>
#include"stat.h"
#include"mix_erro.h"
#include"kernel.h"         // for suser()
#include "mix_const.h"     // for I_REGULAR
// ���ļ������Ҷ�Ӧ i �ڵ���ڲ�����
static struct m_inode * _namei(const char * filename, struct m_inode * base,
	int follow_links);

// �ļ�����ģʽ��
// 004--read, 002--write, 006--read and write
#define ACC_MODE(x) ("\004\002\006\377"[(x)&O_ACCMODE])

/*
* comment out this line if you want names > NAME_LEN chars to be
* truncated. Else they will be disallowed.
*/
/* #define NO_TRUNCATE */
// rwx -- 000
#define MAY_EXEC 1                   // ��ִ��Ȩ��
#define MAY_WRITE 2                  // ��дȨ��
#define MAY_READ 4                   // �ɶ�Ȩ��





// ����ļ��Ķ�дȨ��
//  000    000      000
// ����    ��      ����
// para : mask �������������룬ȡ MAY_EXEC, MAY_WRITE, MAY_READ, �������
// ret  : ����--1�� ������--0
static int permission(struct m_inode * inode, int mask)
{
	int mode = inode->i_mode;

	/* special case: not even root can read/write a deleted file */
	if (inode->i_dev && !inode->i_nlinks)
		return 0;
	// ����ļ�����Ȩ��
	else if (current->euid == inode->i_uid)
		mode >>= 6;
	// ������ԱȨ��
	else if (current->egid == inode->i_gid)
		mode >>= 3;
	// suser()�ж��Ƿ�Ϊ�����û�
	if (((mode & 0007 & mask) == mask) || suser() )
		return 1;
	return 0;
}

// �Ƚ��ļ�������
// ret : ��ͬ--1�� ��ͬ--0
static int match(int len, const char* name, struct dir_entry *de)
{
	int i;
	if (!de || !de->inode || len > NAME_LEN)
		return 0;
	/* "" means "." ---> so paths like "/usr/lib//libc.a" work */
	// ��·�����еĿ�·������( .)������ lib//test.c, ��ͬΪ·�� lib/test.c
	if (!len && (de->name[0] == '.') && (de->name[1] == '\0'))
		return 1;
	// Ŀ¼�ļ������� len, �����
	if (len < NAME_LEN && de->name[len])
		return 0;
	// ���ñ�׼�⺯������������ַ�����ͬ �򷵻� 0
	i = strncmp(name, de->name, len);
	if (i)
		return 0;
	return 1;
}


// ��ָ����Ŀ¼���ҵ�һ��������ƥ���Ŀ¼�
// ����һ�������ҵ�Ŀ¼��ĸ��ٻ�����Լ�Ŀ¼���
// para : dir -- ָ��Ŀ¼�� name -- �ļ����� namelen -- �ļ�����
// return : �����ҵ�Ŀ¼��ĸ��ٻ���飬 res_dir -- �ҵ���Ŀ¼��
static struct buffer_head * find_entry(struct m_inode ** dir,
	const char * name, int namelen, struct dir_entry ** res_dir)
{
	int entries;
	int block, i;
	struct buffer_head * bh;
	struct dir_entry * de;
	struct super_block * sb;
#ifdef NO_TRUNCATE
	if (namelen > NAME_LEN)
		return NULL;
#else
	if (namelen > NAME_LEN)
		namelen = NAME_LEN;
#endif
	// ����Ŀ¼�����
	entries = (*dir)->i_size / (sizeof(struct dir_entry));
	*res_dir = NULL;

	// ������������� ������ڸ�Ŀ¼�²�������Ϊ( ..)��Ŀ¼��
	// �� (..) ���� (.) ����
	if (namelen == 2 && get_fs_byte(name) == '.' && get_fs_byte(name + 1) == '.') {
		// ��һ��α���ڵ�ġ�..��, ��ͬ��.��
		if ((*dir) == current->root)
			namelen = 1;
		// ��һ����װ���ϵĲ�������Ϊ(..)��Ŀ¼�
		// ������Ŀ¼ i �ڵ㽻��������װ�ļ�ϵͳ��Ŀ¼ i �ڵ�
		else if ((*dir)->i_num == ROOT_INO) {
			sb = get_super((*dir)->i_dev);
			if (sb->s_imount) {
				iput(*dir);
				(*dir) = sb->s_imount;
				(*dir)->i_count++;
			}
		}
	}
	// ���ڿ�ʼ��������
	// �����ж�Ŀ¼�ļ��ĵ�һ�����ݿ��Ƿ�������
	if (!(block = (*dir)->i_zone[0]))
		return NULL;
	if (!(bh = bread((*dir)->i_dev, block))) 
		return NULL;
	i = 0;
	de = (struct dir_entry *) bh->b_data;
	while (i < entries) {
		if ((char *)de >= BLOCK_SIZE + bh->b_data) {
			brelse(bh);
			bh = NULL;
			// ����ÿ�Ϊ�գ��������ÿ�
			if (!(block = bmap(*dir, i / DIR_ENTRIES_PER_BLOCK)) ||
				!(bh = bread((*dir)->i_dev, block))) {
				i += DIR_ENTRIES_PER_BLOCK;
				continue;
			}
			de = (struct dir_entry *) bh->b_data;
		}
		// �ļ���ƥ��
		if (match(namelen, name, de)) {
			*res_dir = de;
			return bh;
		}
		de++;
		i++;
	}
	brelse(bh);
	return NULL;
}

// ��ָ��Ŀ¼�����ָ���ļ�����Ŀ¼��
// para : dir -- Ŀ¼ i �ڵ㣬 name -- �ļ����� 
// return : ���������Ŀ¼��ĸ��ٻ���飬 res_dir -- ��ӵ�Ŀ¼��
static struct buffer_head * add_entry(struct m_inode * dir,
	const char * name, int namelen, struct dir_entry ** res_dir)
{
	int block, i;
	struct buffer_head * bh;
	struct dir_entry * de;

	*res_dir = NULL;
#ifdef NO_TRUNCATE
	if (namelen > NAME_LEN)
		return NULL;
#else
	if (namelen > NAME_LEN)
		namelen = NAME_LEN;
#endif
	if (!namelen)
		return NULL;
	// ��õ�һ�����ݿ�Ĵ��̿���
	if (!(block = dir->i_zone[0]))
		return NULL;
	if (!(bh = bread(dir->i_dev, block)))
		return NULL;
	// i Ŀ¼����
	i = 0;
	de = (struct dir_entry *) bh->b_data;
	while (1) {
		if ((char *)de >= BLOCK_SIZE + bh->b_data) {
			brelse(bh);
			bh = NULL;
			block = create_block(dir, i / DIR_ENTRIES_PER_BLOCK);
			if (!block)
				return NULL;
			if (!(bh = bread(dir->i_dev, block))) {
				i += DIR_ENTRIES_PER_BLOCK;
				continue;
			}
			de = (struct dir_entry *) bh->b_data;
		}
		// ���Ŀ¼�����Ѿ������ļ�i�ڵ���ָ����Ŀ¼�ļ�����
		// ��˵������Ŀ¼�ļ���û������ɾ���ļ����µĿ�Ŀ¼��
		// �������Ŀ¼�ļ����ȣ�������Ŀ¼�������Ŀ¼�ļ���ĩβ
		if (i * sizeof(struct dir_entry) >= dir->i_size) {
			de->inode = 0;
			dir->i_size = (i + 1) * sizeof(struct dir_entry);
			dir->i_dirt = 1;
			dir->i_ctime = CURRENT_TIME;
		}
		if (!de->inode) {
			dir->i_mtime = CURRENT_TIME;
			for (i = 0; i < NAME_LEN; i++)
				de->name[i] = (i<namelen) ? get_fs_byte(name + i) : 0;
			bh->b_dirt = 1;
			*res_dir = de;
			return bh;
		}
		de++;
		i++;
	}
}


// ���ҷ������ӵ� i �ڵ�
// para : dir -- Ŀ¼i �ڵ㣬 inode -- Ŀ¼�� i �ڵ�
static struct m_inode * follow_link(struct m_inode * dir, struct m_inode * inode)
{
	struct buffer_head * bh;
	// dir Ϊ�� ����ʹ�ý�������ṹ�еĸ� i �ڵ�
	if (!dir) {
		dir = current->root;
		dir->i_count++;
	}
	if (!inode) {
		iput(dir);
		return NULL;
	}
	// �ж� inode �Ƿ�Ϊ�������� i �ڵ�
	if (!S_ISLNK(inode->i_mode)) {
		iput(dir);                               
		return inode;
	}
	// __asm__("mov %%fs,%0":"=r" (fs));
	// �����������ӵ�����·����
	if ( !inode->i_zone[0] ||
		!(bh = bread(inode->i_dev, inode->i_zone[0]))) {
		iput(dir);
		iput(inode);
		return NULL;
	}
	iput(inode);
	//__asm__("mov %0,%%fs"::"r" ((unsigned short)0x10));
	// ���·����ָ��� i �ڵ�
	inode = _namei(bh->b_data, dir, 0);
	//__asm__("mov %0,%%fs"::"r" (fs));
	brelse(bh);
	return inode;
}

// ���ݸ�����·��������������ֱ���ﵽ��˵�Ŀ¼
// ���磺 ����·���� pathname = home/noodle/test.c
// �������� test.c ����Ŀ¼�� i �ڵ㣬��Ŀ¼noodle�� i �ڵ�
// para : inode -- ·������������ʼĿ¼ i �ڵ�
static struct m_inode * get_dir(const char * pathname, struct m_inode * inode)
{
	char c;
	const char * thisname;
	struct buffer_head * bh;
	int namelen, inr;
	struct dir_entry * de;
	struct m_inode * dir;
	// û������ʼĿ¼���ͽ����̵ĵ�ǰ����Ŀ¼��Ϊ��ʼĿ¼
	if (!inode) {
		inode = current->pwd; // ���̵ĵ�ǰ����Ŀ¼
		inode->i_count++;
	}
	// ����������Ǿ���·����·�����ĵ�һ���ַ�Ϊ /����
	// �ʹӵ�ǰ�������õ��ļ�ϵͳ���ڵ㿪ʼ����
	if ((c = get_fs_byte(pathname)) == '/') {
		iput(inode);
		inode = current->root;
		pathname++;
		inode->i_count++;
	}
	// ÿ��ѭ������һ��Ŀ¼��
	while (1) {
		thisname = pathname;
		// �Ƿ�ΪĿ¼���ҿ�ִ��
		if (!S_ISDIR(inode->i_mode) || !permission(inode, MAY_EXEC)) {
			iput(inode);
			return NULL;
		}
		// ��ȡһ��Ŀ¼��
		for (namelen = 0; (c = get_fs_byte(pathname++)) && (c != '/'); namelen++)
			/* nothing */;
		// �Ƿ񵽴����Ŀ¼
		if (!c)
			return inode;
		// ����Ŀ¼����� Ŀ¼��
		if (!(bh = find_entry(&inode, thisname, namelen, &de))) {
			iput(inode);
			return NULL;
		}
		inr = de->inode;
		brelse(bh);
		dir = inode;
		// �õ�Ŀ¼ i �ڵ�
		if (!(inode = iget(dir->i_dev, inr))) {
			iput(dir);
			return NULL;
		}
		// ����Ƿ������ӣ��ҵ�����������ָ��� i �ڵ�
		if (!(inode = follow_link(dir, inode)))
			return NULL;
	}
}

// ����·�����ҵ��ļ���Ŀ¼ i �ڵ� �� �ļ���
// para : pathname -- ·������ namelen -- ���ص��ļ�������
//        name -- ָ���ļ����� base -- ·����������ʼĿ¼
// return �� �ļ���������Ŀ¼ i �ڵ�
// ���硡����pathname = home/noodle/test.c , �򷵻� noodle ����Ӧ�� i �ڵ�
// name ָ�� test.c
static struct m_inode * dir_namei(const char * pathname,
	int * namelen, const char ** name, struct m_inode * base)
{
	char c;
	const char * basename;
	struct m_inode * dir;
	// ����ļ�����Ŀ¼�� i �ڵ�
	if (!(dir = get_dir(pathname, base)))
		return NULL;
	basename = pathname;
	// ����ļ���
	while (c = get_fs_byte(pathname++))
		if (c == '/')
			basename = pathname;
	*namelen = pathname - basename - 1;
	*name = basename;
	return dir;
}

// ȡָ��·�����ļ��� i �ڵ��ڲ�����
// para : pathname -- ·������ base -- �������Ŀ¼ i �ڵ㣬
//        follow_links -- �Ƿ����������ӣ� 1--��Ҫ��0--����Ҫ��
// return : ���ض�Ӧ�� i �ڵ�
struct m_inode * _namei(const char * pathname, struct m_inode * base,
	int follow_links)
{
	const char * basename;
	int inr, namelen;
	struct m_inode * inode;
	struct buffer_head * bh;
	struct dir_entry * de;
	// ���·�����Ŀ¼ i �ڵ�� �ļ���
	if (!(base = dir_namei(pathname, &namelen, &basename, base)))
		return NULL;
	// ����������� ���磺 /usr/
	if (!namelen)			
		return base;
	// �ҵ��ļ���Ӧ��Ŀ¼��
	bh = find_entry(&base, basename, namelen, &de);
	if (!bh) {
		iput(base);
		return NULL;
	}
	inr = de->inode;
	brelse(bh);
	// ����ļ� i �ڵ�
	if (!(inode = iget(base->i_dev, inr))) {
		iput(base);
		return NULL;
	}
	// �Ƿ�����������
	if (follow_links)
		inode = follow_link(base, inode);
	else
		iput(base);
	// �޸������ʸ� i �ڵ�ʱ��
	inode->i_atime = CURRENT_TIME;
	inode->i_dirt = 1;
	return inode;
}

// ȡָ��·������ i �ڵ㣬�������������
// �ӽ��������õĸ��ڵ㿪ʼ����
struct m_inode * lnamei(const char * pathname)
{
	return _namei(pathname, NULL, 0);
}

// ȡָ��·������ i �ڵ㣬�����������
// �ӽ��������õĸ��ڵ㿪ʼ����
struct m_inode * namei(const char * pathname)
{
	return _namei(pathname, NULL, 1);
}

// �������ļ��򿪳���
// para : pathname -- �ļ����� flag -- �ļ���дȨ�ޣ�ֻ����ֻд����д��...��
//        mode -- �ļ��ķ�����ɱ�־�� res_inode -- �ļ��� i �ڵ�
// return : 0 -- �ɹ��� ���򷵻س�����
int open_namei(const char * pathname, int flag, int mode,
	struct m_inode ** res_inode)
{
	const char * basename;
	int inr, dev, namelen;
	struct m_inode * dir, *inode;
	struct buffer_head * bh;
	struct dir_entry * de;
	// ����������ļ��� 0 ��־�����ļ���ֻ��ģʽ�򿪣�������ļ���дȨ��
	if ((flag & O_TRUNC) && !(flag & O_ACCMODE))
		flag |= O_WRONLY;
	// ��ǰ���̵��ļ�����������
	mode &= 0777 & ~current->umask;
	// �����ͨ�ļ���־
	mode |= I_REGULAR;
	if (!(dir = dir_namei(pathname, &namelen, &basename, NULL)))
		return -ENOENT;
	// ����ļ�������Ϊ0 ���Ҳ������� �� д ִ��  ���� �� �� 0 
	// ����Ϊ�Ǵ�Ŀ¼�ļ���ֱ�ӷ���Ŀ¼ i �ڵ�
	if (!namelen) {			/* special case: '/usr/' etc */
		if (!(flag & (O_ACCMODE | O_CREAT | O_TRUNC))) {
			*res_inode = dir;
			return 0;
		}
		iput(dir);
		return -EISDIR;
	}
	bh = find_entry(&dir, basename, namelen, &de);
	// bh ==0 , ��ʾ�ļ������ڣ� �½��ļ�
	if (!bh) {
		// �ж��Ƿ� o_creat = 1
		if (!(flag & O_CREAT)) {
			iput(dir);
			return -ENOENT;
		}
		// �ж϶��ļ����ڵ�Ŀ¼��д��Ȩ��
		if (!permission(dir, MAY_WRITE)) {
			iput(dir);
			return -EACCES;
		}
		inode = new_inode(dir->i_dev);                   // ��������������������������ע��
		if (!inode) {
			iput(dir);
			return -ENOSPC;
		}
		// �����ļ����û�id
		inode->i_uid = current->euid;
		inode->i_mode = mode;
		inode->i_dirt = 1;
		// ���Ŀ¼��
		bh = add_entry(dir, basename, namelen, &de);
		if (!bh) {
			inode->i_nlinks--;
			iput(inode);
			iput(dir);
			return -ENOSPC;
		}
		de->inode = inode->i_num;
		bh->b_dirt = 1;
		brelse(bh);
		iput(dir);
		*res_inode = inode;
		return 0;
	}
	// �ļ�����
	inr = de->inode;
	dev = dir->i_dev;
	brelse(bh);
	if (flag & O_EXCL) {
		iput(dir);
		return -EEXIST;
	}
	// �����������
	if (!(inode = follow_link(dir, iget(dev, inr))))
		return -EACCES;
	// �����Ŀ¼�ļ����ļ����ʱ�־��д���߶�д����Ŀ¼�ļ����ɽ���д������
	// ����û���ļ�����Ȩ�ޣ����ش���
	if ((S_ISDIR(inode->i_mode) && (flag & O_ACCMODE)) ||
		!permission(inode, ACC_MODE(flag))) {
		iput(inode);
		return -EPERM;
	}
	inode->i_atime = CURRENT_TIME;
	// o_trunc �ļ���Ϊ 0 ��־
	if (flag & O_TRUNC)
		truncate(inode);
	*res_inode = inode;
	return 0;
}
 


// ����һ���豸�����ļ�����ͨ�ļ��ڵ�
// �ú�����������Ϊ filename ���ļ�ϵͳ�ڵ㣬��ͨ�ļ����豸�ļ�
// para : filename -- ·����, mode -- �ļ����ͺͷ�����ɣ�dev -- �豸��
// return �� 0 -- �ɹ������򷵻س�����
int sys_mknod(const char * filename, int mode, int dev)
{
	const char * basename;
	int namelen;
	struct m_inode * dir, *inode;
	struct buffer_head * bh;
	struct dir_entry * de;
	// �ж��Ƿ񳬼��û�
	if (!suser())
		return -EPERM;
	// �ҵ�·������˵� i �ڵ�
	if (!(dir = dir_namei(filename, &namelen, &basename, NULL)))
		return -ENOENT;
	// Ҫ�����ļ����ļ��������Ƿ�Ϊ��
	if (!namelen) {
		iput(dir);
		return -ENOENT;
	}
	// �ж��Ƿ�����Ȩ��Ҫ��
	if (!permission(dir, MAY_WRITE)) {
		iput(dir);
		return -EPERM;
	}
	// ���ҵ�ǰĿ¼���Ƿ����ͬ���ļ�
	bh = find_entry(&dir, basename, namelen, &de);
	if (bh) {
		brelse(bh);
		iput(dir);
		return -EEXIST;
	}
	// ��ȡ i �ڵ�
	inode = new_inode(dir->i_dev);
	if (!inode) {
		iput(dir);
		return -ENOSPC;
	}
	inode->i_mode = mode;
	// �ж��Ƿ��豸�ļ�
	if (S_ISBLK(mode) || S_ISCHR(mode))
		inode->i_zone[0] = dev;
	inode->i_mtime = inode->i_atime = CURRENT_TIME;
	inode->i_dirt = 1;
	// ���Ŀ¼��
	bh = add_entry(dir, basename, namelen, &de);
	if (!bh) {
		iput(dir);
		inode->i_nlinks = 0;
		iput(inode);
		return -ENOSPC;
	}
	de->inode = inode->i_num;
	bh->b_dirt = 1;
	iput(dir);
	iput(inode);
	brelse(bh);
	return 0;
}


// ����һ��Ŀ¼
// para : pathname -- ·������ mode -- ���Ժ�Ȩ��
// return : 0 -- �ɹ��� ���򷵻س�����
int sys_mkdir(const char * pathname, int mode)
{
	const char * basename;
	int namelen;
	struct m_inode * dir, *inode;
	struct buffer_head * bh, *dir_block;
	struct dir_entry * de;
    // ·�����Ķ���Ŀ¼ 
	if (!(dir = dir_namei(pathname, &namelen, &basename, NULL)))
		return -ENOENT;
	// Ҫ������Ŀ¼������Ϊ��
	if (!namelen) {
		iput(dir);
		return -ENOENT;
	}
	// Ȩ�޼��
	if (!permission(dir, MAY_WRITE)) {
		iput(dir);
		return -EPERM;
	}
	// ����Ƿ����ͬ���ļ�
	bh = find_entry(&dir, basename, namelen, &de);
	if (bh) {
		brelse(bh);
		iput(dir);
		return -EEXIST;
	}
	//
	inode = new_inode(dir->i_dev);
	if (!inode) {
		iput(dir);
		return -ENOSPC;
	}
	// ����Ŀ¼�ļ��Ĵ�С����ΪĿ¼�ļ�һ����������Ŀ¼�� (.)��(..)
	// ����ļ���СΪ����Ŀ¼��ĳ���
	inode->i_size = 2 * sizeof(struct dir_entry); // 32
	inode->i_dirt = 1;
	inode->i_mtime = inode->i_atime = CURRENT_TIME;
	if (!(inode->i_zone[0] = new_block(inode->i_dev))) {
		iput(dir);
		inode->i_nlinks--;
		iput(inode);
		return -ENOSPC;
	}
	inode->i_dirt = 1;
	// ����Ĭ��Ŀ¼�� . �� ..
	if (!(dir_block = bread(inode->i_dev, inode->i_zone[0]))) {
		iput(dir);
		inode->i_nlinks--;
		iput(inode);
		return -ERROR;
	}
	de = (struct dir_entry *) dir_block->b_data;
	de->inode = inode->i_num;
	strcpy(de->name,".");
	de++;
	de->inode = dir->i_num;
	strcpy(de->name,"..");
	inode->i_nlinks = 2;
	dir_block->b_dirt = 1;
	brelse(dir_block);
	inode->i_mode = I_DIRECTORY | (mode & 0777 & ~current->umask);
	inode->i_dirt = 1;
	// ���Ŀ¼�� 
	bh = add_entry(dir, basename, namelen, &de);
	if (!bh) {
		iput(dir);
		inode->i_nlinks = 0;
		iput(inode);
		return -ENOSPC;
	}
	de->inode = inode->i_num;
	bh->b_dirt = 1;
	dir->i_nlinks++;
	dir->i_dirt = 1;
	iput(dir);
	iput(inode);
	brelse(bh);
	return 0;
}


// ���ָ����Ŀ¼�Ƿ�Ϊ�յ��ӳ���
// para : inode -- Ŀ¼ i �ڵ�
// return �� 1 -- Ŀ¼�ǿ�Ŀ¼
static int empty_dir(struct m_inode * inode)
{
	int nr, block;
	int len;
	struct buffer_head * bh;
	struct dir_entry * de;
    // ����Ŀ¼�ļ��ĳ��Ȼ�������Ŀ¼��ĵĸ���
	len = inode->i_size / sizeof(struct dir_entry);
	if (len<2 || !inode->i_zone[0] ||
		!(bh = bread(inode->i_dev, inode->i_zone[0]))) {
		printk("warning - bad directory on dev %04x\n", inode->i_dev);
		return 0;
	}
	// ����һ���͵ڶ���Ŀ¼��������Ƿ���ȷ
	de = (struct dir_entry *) bh->b_data;
	if (de[0].inode != inode->i_num || !de[1].inode ||
		strcmp(".", de[0].name) || strcmp("..", de[1].name)) {
		printk("warning - bad directory on dev %04x\n", inode->i_dev);
		return 0;
	}
	nr = 2;
	de += 2;
	while (nr<len) {
		if ((void *)de >= (void *)(bh->b_data + BLOCK_SIZE)) {
			brelse(bh);
			block = bmap(inode, nr / DIR_ENTRIES_PER_BLOCK);
			if (!block) {
				nr += DIR_ENTRIES_PER_BLOCK;
				continue;
			}
			if (!(bh = bread(inode->i_dev, block)))
				return 0;
			de = (struct dir_entry *) bh->b_data;
		}
		// ��Ϊ 0 ��ʾ�����ļ�
		if (de->inode) {
			brelse(bh);
			return 0;
		}
		de++;
		nr++;
	}
	brelse(bh);
	return 1;
}


// ɾ��һ����Ŀ¼
// para : name -- ·����
// return : 0 -- �ɹ������򷵻ش�����
int sys_rmdir(const char * name)
{
	const char * basename;
	int namelen;
	struct m_inode * dir, *inode;
	struct buffer_head * bh;
	struct dir_entry * de;
    // Ŀ¼�ļ����ڵ�Ŀ¼ i �ڵ�
	if (!(dir = dir_namei(name, &namelen, &basename, NULL)))
		return -ENOENT;
	// 
	if (!namelen) {
		iput(dir);
		return -ENOENT;
	}
	if (!permission(dir, MAY_WRITE)) {
		iput(dir);
		return -EPERM;
	}
	bh = find_entry(&dir, basename, namelen, &de);
	if (!bh) {
		iput(dir);
		return -ENOENT;
	}
	if (!(inode = iget(dir->i_dev, de->inode))) {
		iput(dir);
		brelse(bh);
		return -EPERM;
	}
	// root -- euid = 0
	// ������������ޱ�־��������û����߸�Ŀ¼�Ĵ����߲ſ���ɾ����Ŀ¼
	if ((dir->i_mode & S_ISVTX) && !suser() &&                          // && current->euid
		inode->i_uid != current->euid) {
		iput(dir);
		iput(inode);
		brelse(bh);
		return -EPERM;
	}
	// i �ڵ����ü�������1 ��ʾ �з�������
	if (inode->i_dev != dir->i_dev || inode->i_count>1) {
		iput(dir);
		iput(inode);
		brelse(bh);
		return -EPERM;
	}
	if (inode == dir) {	/* we may not delete ".", but "../dir" is ok */
		iput(inode);
		iput(dir);
		brelse(bh);
		return -EPERM;
	}
	if (!S_ISDIR(inode->i_mode)) {
		iput(inode);
		iput(dir);
		brelse(bh);
		return -ENOTDIR;
	}
	if (!empty_dir(inode)) {
		iput(inode);
		iput(dir);
		brelse(bh);
		return -ENOTEMPTY;
	}
	if (inode->i_nlinks != 2)
		printk("empty directory has nlink!=2 (%d)", inode->i_nlinks);
	de->inode = 0;
	bh->b_dirt = 1;
	brelse(bh);
	inode->i_nlinks = 0;
	inode->i_dirt = 1;
	dir->i_nlinks--;
	dir->i_ctime = dir->i_mtime = CURRENT_TIME;
	dir->i_dirt = 1;
	iput(dir);
	iput(inode);
	return 0;
}


// ɾ���ļ�����Ӧ��Ŀ¼��
// ������ļ������һ�����ӣ� ��û�н������ڴ򿪸��ļ�����ɾ�����ļ�
// return : 0 -- �ɹ��� ���򷵻س�����
int sys_unlink(const char * name)
{
	const char * basename;
	int namelen;
	struct m_inode * dir, *inode;
	struct buffer_head * bh;
	struct dir_entry * de;

	if (!(dir = dir_namei(name, &namelen, &basename, NULL)))
		return -ENOENT;
	if (!namelen) {
		iput(dir);
		return -ENOENT;
	}
	if (!permission(dir, MAY_WRITE)) {
		iput(dir);
		return -EPERM;
	}
	bh = find_entry(&dir, basename, namelen, &de);
	if (!bh) {
		iput(dir);
		return -ENOENT;
	}
	if (!(inode = iget(dir->i_dev, de->inode))) {
		iput(dir);
		brelse(bh);
		return -ENOENT;
	}
	if ((dir->i_mode & S_ISVTX) && !suser() &&
		current->euid != inode->i_uid &&
		current->euid != dir->i_uid) {
		iput(dir);
		iput(inode);
		brelse(bh);
		return -EPERM;
	}
	if (S_ISDIR(inode->i_mode)) {
		iput(inode);
		iput(dir);
		brelse(bh);
		return -EPERM;
	}
	if (!inode->i_nlinks) {
		printk("Deleting nonexistent file (%04x:%d), %d\n",
			inode->i_dev, inode->i_num, inode->i_nlinks);
		inode->i_nlinks = 1;
	}
	de->inode = 0;
	bh->b_dirt = 1;
	brelse(bh);
	inode->i_nlinks--;
	inode->i_dirt = 1;
	inode->i_ctime = CURRENT_TIME;
	iput(inode);
	iput(dir);
	return 0;
}

// ������������
// para : oldname -- ԭ·������ newname -- ��·����
// return �� 0 -- �ɹ��� ���򷵻س�����
int sys_symlink(const char * oldname, const char * newname)
{
	struct dir_entry * de;
	struct m_inode * dir, *inode;
	struct buffer_head * bh, *name_block;
	const char * basename;
	int namelen, i;
	char c;

	dir = dir_namei(newname, &namelen, &basename, NULL);
	if (!dir)
		return -EACCES;
	if (!namelen) {
		iput(dir);
		return -EPERM;
	}
	if (!permission(dir, MAY_WRITE)) {
		iput(dir);
		return -EACCES;
	}
	if (!(inode = new_inode(dir->i_dev))) {
		iput(dir);
		return -ENOSPC;
	}
	inode->i_mode = S_IFLNK | (0777 & ~current->umask);
	inode->i_dirt = 1;
	if (!(inode->i_zone[0] = new_block(inode->i_dev))) {
		iput(dir);
		inode->i_nlinks--;
		iput(inode);
		return -ENOSPC;
	}
	inode->i_dirt = 1;
	if (!(name_block = bread(inode->i_dev, inode->i_zone[0]))) {
		iput(dir);
		inode->i_nlinks--;
		iput(inode);
		return -ERROR;
	}
	i = 0;
	while (i < (BLOCK_SIZE - 1) && (c = get_fs_byte(oldname++)))
		name_block->b_data[i++] = c;
	name_block->b_data[i] = 0;
	name_block->b_dirt = 1;
	brelse(name_block);
	inode->i_size = i;
	inode->i_dirt = 1;
	bh = find_entry(&dir, basename, namelen, &de);
	if (bh) {
		inode->i_nlinks--;
		iput(inode);
		brelse(bh);
		iput(dir);
		return -EEXIST;
	}
	bh = add_entry(dir, basename, namelen, &de);
	if (!bh) {
		inode->i_nlinks--;
		iput(inode);
		iput(dir);
		return -ENOSPC;
	}
	de->inode = inode->i_num;
	bh->b_dirt = 1;
	brelse(bh);
	iput(dir);
	iput(inode);
	return 0;
}

// Ϊ�ļ�����һ���ļ���Ŀ¼��
// Ϊ�Ѿ����ڵ��ļ�����һ�������ӣ�Ӳ����
// para : oldname -- ԭ·������ newname -- ��·����
// return �� 0 -- �ɹ��� ���򷵻س�����
int sys_link(const char * oldname, const char * newname)
{
	struct dir_entry * de;
	struct m_inode * oldinode, *dir;
	struct buffer_head * bh;
	const char * basename;
	int namelen;
	// ���Դ�ļ��� i �ڵ㣬�䲻����Ŀ¼�ļ�
	oldinode = namei(oldname);
	if (!oldinode)
		return -ENOENT;
	if (S_ISDIR(oldinode->i_mode)) {
		iput(oldinode);
		return -EPERM;
	}
	// �����·���Ķ���Ŀ¼
	dir = dir_namei(newname, &namelen, &basename, NULL);
	if (!dir) {
		iput(oldinode);
		return -EACCES;
	}
	if (!namelen) {
		iput(oldinode);
		iput(dir);
		return -EPERM;
	}
	// ����·��Ҫ��ͬһ���豸��
	if (dir->i_dev != oldinode->i_dev) {
		iput(dir);
		iput(oldinode);
		return -EXDEV;
	}
	if (!permission(dir, MAY_WRITE)) {
		iput(dir);
		iput(oldinode);
		return -EACCES;
	}
	bh = find_entry(&dir, basename, namelen, &de);
	if (bh) {
		brelse(bh);
		iput(dir);
		iput(oldinode);
		return -EEXIST;
	}
	bh = add_entry(dir, basename, namelen, &de);
	if (!bh) {
		iput(dir);
		iput(oldinode);
		return -ENOSPC;
	}
	de->inode = oldinode->i_num;
	bh->b_dirt = 1;
	brelse(bh);
	iput(dir);
	oldinode->i_nlinks++;
	oldinode->i_ctime = CURRENT_TIME;
	oldinode->i_dirt = 1;
	iput(oldinode);
	return 0;
}


static int print_dir(struct dir_entry * de)
{
	M_printf("dir name  = %-15s", de->name);
	M_printf("inode number = %-5d\n", de->inode);
	return 0;
}

// ��ӡĿ¼�ڵ��ļ���
void sys_ls(const char* pathname, struct m_inode* base)
{
	int entries;
	int block, i;
	struct buffer_head * bh;
	struct dir_entry * de;
	struct m_inode* dir;

	if (!(dir = _namei(pathname, base, 1))) {
		printk("ls : dirctory doesn't exsit\n");
		return;
	}
	if (!(S_ISDIR(dir->i_mode))) {
		iput(dir);
		printk("ls : pathname is not a dirctory\n");
		return;
	}
	entries = (dir)->i_size / (sizeof(struct dir_entry));
	if (!(block = (dir)->i_zone[0]))
		return;
	if (!(bh = bread((dir)->i_dev, block)))
		return;
	i = 0;
	de = (struct dir_entry *) bh->b_data;
	while (i < entries) {
		if ((char *)de >= BLOCK_SIZE + bh->b_data) {
			brelse(bh);
			bh = NULL;
			// ����ÿ�Ϊ�գ��������ÿ�
			if (!(block = bmap(dir, i / DIR_ENTRIES_PER_BLOCK)) ||
				!(bh = bread((dir)->i_dev, block))) {
				i += DIR_ENTRIES_PER_BLOCK;
				continue;
			}
			de = (struct dir_entry *) bh->b_data;
		}
		// ��ӡĿ¼��
		if (de->inode)
			print_dir(de);
		de++;
		i++;
	}
	iput(dir);
	brelse(bh);
}


// cd -- dir
int sys_cd(const char* filename){
	int flag = 0, error, i;
	struct m_inode* new_pwd;
	char pathname[1024];

	if (strlen(filename) > 1022)
		return -ERROR;
	for (i = 0; i < strlen(filename); i++)
		pathname[i] = *(filename + i);
	if (pathname[i - 1] != '/')
		pathname[i++] = '/';
	pathname[i] = '\0';
	// ��Ŀ¼�ļ�
	if((error = open_namei(pathname, flag, 0, &new_pwd)) < 0)
		return error;
	iput(current->pwd);
	current->pwd = new_pwd;
	return 1;
}



// ��ָ����Ŀ¼���ҵ�һ�� ��Ӧi�ڵ�� ��Ŀ¼�
// ����һ�������ҵ�Ŀ¼��ĸ��ٻ�����Լ�Ŀ¼���
// para : dir -- ָ��Ŀ¼��inr -- i�ڵ��
// return : �����ҵ�Ŀ¼��ĸ��ٻ���飬 res_dir -- �ҵ���Ŀ¼��
static struct buffer_head * get_entry(struct m_inode ** dir,
	unsigned int inr, struct dir_entry ** res_dir)
{
	int entries;
	int block, i;
	struct buffer_head * bh;
	struct dir_entry * de;

	// ����Ŀ¼�����
	entries = (*dir)->i_size / (sizeof(struct dir_entry));
	*res_dir = NULL;
	// �����ж�Ŀ¼�ļ��ĵ�һ�����ݿ��Ƿ�������
	if (!(block = (*dir)->i_zone[0]))
		return NULL;
	if (!(bh = bread((*dir)->i_dev, block)))
		return NULL;
	i = 0;
	de = (struct dir_entry *) bh->b_data;
	while (i < entries) {
		if ((char *)de >= BLOCK_SIZE + bh->b_data) {
			brelse(bh);
			bh = NULL;
			// ����ÿ�Ϊ�գ��������ÿ�
			if (!(block = bmap(*dir, i / DIR_ENTRIES_PER_BLOCK)) ||
				!(bh = bread((*dir)->i_dev, block))) {
				i += DIR_ENTRIES_PER_BLOCK;
				continue;
			}
			de = (struct dir_entry *) bh->b_data;
		}
		// �ļ���ƥ��
		if (de->inode == inr) {
			*res_dir = de;
			return bh;
		}
		de++;
		i++;
	}
	brelse(bh);
	return NULL;
}


// ��ӡ��ǰ����Ŀ¼
int sys_print_pwd(char* pathname, int size){
	struct m_inode *prev_inode,*cur_inode;
	struct dir_entry* de;
	struct buffer_head* bh;
	struct super_block* sb;
	const char* dir = "..";
	int i, count = 0;

	if (current->pwd == current->root){
		pathname[--size] = '/';
		++count;
	}
	if(!(cur_inode = iget(current->pwd->i_dev, current->pwd->i_num)))
		return -ENOENT;
	while (cur_inode != current->root){
		if (!(bh = find_entry(&cur_inode, dir, 2, &de))){
			iput(cur_inode);
			return -ENOENT;
		}
		if (!(prev_inode = iget(cur_inode->i_dev, de->inode))){
			iput(cur_inode);
			brelse(bh);
			return -ENOENT;
		}
		brelse(bh);
		if (!(bh = get_entry(&prev_inode, cur_inode->i_num, &de))){
		  iput(cur_inode);
	      iput(prev_inode);
		  return -ENOENT;
		}
		// ��ȡĿ¼��
		i = strlen(de->name);
		if (size < (i + 1)){
			iput(cur_inode);
			iput(prev_inode);
			brelse(bh);
			return -ERROR;
		}
		while (i-- > 0){
			pathname[--size] = de->name[i];
			++count;
		}
		pathname[--size] = '/';
		++count;
		brelse(bh);
		iput(cur_inode);
		cur_inode = prev_inode;
	}
	// root
	/*{

	}*/
	iput(cur_inode);
	return count;
}
