#include"file_sys.h"
#include"fcntl.h"
#include<string.h>
#include"stat.h"
#include"mix_erro.h"
#include"kernel.h"         // for suser()
#include "mix_const.h"     // for I_REGULAR
// 由文件名查找对应 i 节点的内部函数
static struct m_inode * _namei(const char * filename, struct m_inode * base,
	int follow_links);

// 文件访问模式宏
// 004--read, 002--write, 006--read and write
#define ACC_MODE(x) ("\004\002\006\377"[(x)&O_ACCMODE])

/*
* comment out this line if you want names > NAME_LEN chars to be
* truncated. Else they will be disallowed.
*/
/* #define NO_TRUNCATE */
// rwx -- 000
#define MAY_EXEC 1                   // 可执行权限
#define MAY_WRITE 2                  // 可写权限
#define MAY_READ 4                   // 可读权限





// 检查文件的读写权限
//  000    000      000
// 宿主    组      其他
// para : mask 访问属性屏蔽码，取 MAY_EXEC, MAY_WRITE, MAY_READ, 或其组合
// ret  : 允许--1， 不允许--0
static int permission(struct m_inode * inode, int mask)
{
	int mode = inode->i_mode;

	/* special case: not even root can read/write a deleted file */
	if (inode->i_dev && !inode->i_nlinks)
		return 0;
	// 获得文件宿主权限
	else if (current->euid == inode->i_uid)
		mode >>= 6;
	// 获得组成员权限
	else if (current->egid == inode->i_gid)
		mode >>= 3;
	// suser()判断是否为超级用户
	if (((mode & 0007 & mask) == mask) || suser() )
		return 1;
	return 0;
}

// 比较文件名函数
// ret : 相同--1， 不同--0
static int match(int len, const char* name, struct dir_entry *de)
{
	int i;
	if (!de || !de->inode || len > NAME_LEN)
		return 0;
	/* "" means "." ---> so paths like "/usr/lib//libc.a" work */
	// 将路径名中的空路径当成( .)，例如 lib//test.c, 等同为路径 lib/test.c
	if (!len && (de->name[0] == '.') && (de->name[1] == '\0'))
		return 1;
	// 目录文件名超过 len, 则不相等
	if (len < NAME_LEN && de->name[len])
		return 0;
	// 调用标准库函数，如果两个字符串相同 则返回 0
	i = strncmp(name, de->name, len);
	if (i)
		return 0;
	return 1;
}


// 在指定的目录中找到一个与名字匹配的目录项，
// 返回一个含有找到目录项的高速缓冲块以及目录项本身
// para : dir -- 指定目录， name -- 文件名， namelen -- 文件长度
// return : 包含找到目录项的高速缓冲块， res_dir -- 找到的目录项
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
	// 计算目录项个数
	entries = (*dir)->i_size / (sizeof(struct dir_entry));
	*res_dir = NULL;

	// 处理特殊情况， 如果在在根目录下查找名字为( ..)的目录项
	// 则将 (..) 当作 (.) 处理
	if (namelen == 2 && get_fs_byte(name) == '.' && get_fs_byte(name + 1) == '.') {
		// 在一个伪根节点的’..‘, 如同’.‘
		if ((*dir) == current->root)
			namelen = 1;
		// 在一个安装点上的查找名字为(..)的目录项，
		// 将导致目录 i 节点交换到被安装文件系统的目录 i 节点
		else if ((*dir)->i_num == ROOT_INO) {
			sb = get_super((*dir)->i_dev);
			if (sb->s_imount) {
				iput(*dir);
				(*dir) = sb->s_imount;
				(*dir)->i_count++;
			}
		}
	}
	// 现在开始正常操作
	// 首先判断目录文件的第一个数据块是否有数据
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
			// 如果该块为空，就跳过该块
			if (!(block = bmap(*dir, i / DIR_ENTRIES_PER_BLOCK)) ||
				!(bh = bread((*dir)->i_dev, block))) {
				i += DIR_ENTRIES_PER_BLOCK;
				continue;
			}
			de = (struct dir_entry *) bh->b_data;
		}
		// 文件名匹配
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

// 向指定目录下添加指定文件名的目录项
// para : dir -- 目录 i 节点， name -- 文件名， 
// return : 包含添加了目录项的高速缓冲块， res_dir -- 添加的目录项
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
	// 获得第一个数据快的磁盘块编号
	if (!(block = dir->i_zone[0]))
		return NULL;
	if (!(bh = bread(dir->i_dev, block)))
		return NULL;
	// i 目录项编号
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
		// 如果目录项标号已经超过文件i节点所指出的目录文件长度
		// 则说明整个目录文件中没有由于删除文件留下的空目录项
		// 因此增加目录文件长度，并把新目录项添加在目录文件的末尾
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


// 查找符号链接的 i 节点
// para : dir -- 目录i 节点， inode -- 目录项 i 节点
static struct m_inode * follow_link(struct m_inode * dir, struct m_inode * inode)
{
	struct buffer_head * bh;
	// dir 为空 ，就使用进程任务结构中的根 i 节点
	if (!dir) {
		dir = current->root;
		dir->i_count++;
	}
	if (!inode) {
		iput(dir);
		return NULL;
	}
	// 判断 inode 是否为符号链接 i 节点
	if (!S_ISLNK(inode->i_mode)) {
		iput(dir);                               
		return inode;
	}
	// __asm__("mov %%fs,%0":"=r" (fs));
	// 读出符号连接的完整路径名
	if ( !inode->i_zone[0] ||
		!(bh = bread(inode->i_dev, inode->i_zone[0]))) {
		iput(dir);
		iput(inode);
		return NULL;
	}
	iput(inode);
	//__asm__("mov %0,%%fs"::"r" ((unsigned short)0x10));
	// 获得路径名指向的 i 节点
	inode = _namei(bh->b_data, dir, 0);
	//__asm__("mov %0,%%fs"::"r" (fs));
	brelse(bh);
	return inode;
}

// 根据给出的路径名进行搜索，直到达到最顶端的目录
// 例如： 给定路径名 pathname = home/noodle/test.c
// 则函数返回 test.c 所处目录的 i 节点，即目录noodle的 i 节点
// para : inode -- 路径名搜索的起始目录 i 节点
static struct m_inode * get_dir(const char * pathname, struct m_inode * inode)
{
	char c;
	const char * thisname;
	struct buffer_head * bh;
	int namelen, inr;
	struct dir_entry * de;
	struct m_inode * dir;
	// 没给定起始目录，就将进程的当前工作目录作为起始目录
	if (!inode) {
		inode = current->pwd; // 进程的当前工作目录
		inode->i_count++;
	}
	// 如果给定的是绝对路径（路径名的第一个字符为 /），
	// 就从当前进程设置的文件系统根节点开始搜索
	if ((c = get_fs_byte(pathname)) == '/') {
		iput(inode);
		inode = current->root;
		pathname++;
		inode->i_count++;
	}
	// 每次循环处理一个目录名
	while (1) {
		thisname = pathname;
		// 是否为目录，且可执行
		if (!S_ISDIR(inode->i_mode) || !permission(inode, MAY_EXEC)) {
			iput(inode);
			return NULL;
		}
		// 获取一个目录名
		for (namelen = 0; (c = get_fs_byte(pathname++)) && (c != '/'); namelen++)
			/* nothing */;
		// 是否到达最顶层目录
		if (!c)
			return inode;
		// 根据目录名获得 目录项
		if (!(bh = find_entry(&inode, thisname, namelen, &de))) {
			iput(inode);
			return NULL;
		}
		inr = de->inode;
		brelse(bh);
		dir = inode;
		// 得到目录 i 节点
		if (!(inode = iget(dir->i_dev, inr))) {
			iput(dir);
			return NULL;
		}
		// 如果是符号连接，找到符号链接所指向的 i 节点
		if (!(inode = follow_link(dir, inode)))
			return NULL;
	}
}

// 根据路径名找到文件的目录 i 节点 和 文件名
// para : pathname -- 路径名， namelen -- 返回的文件名长度
//        name -- 指向文件名， base -- 路径搜索的起始目录
// return ： 文件的所处的目录 i 节点
// 例如　：　pathname = home/noodle/test.c , 则返回 noodle 所对应的 i 节点
// name 指向 test.c
static struct m_inode * dir_namei(const char * pathname,
	int * namelen, const char ** name, struct m_inode * base)
{
	char c;
	const char * basename;
	struct m_inode * dir;
	// 获得文件所处目录的 i 节点
	if (!(dir = get_dir(pathname, base)))
		return NULL;
	basename = pathname;
	// 获得文件名
	while (c = get_fs_byte(pathname++))
		if (c == '/')
			basename = pathname;
	*namelen = pathname - basename - 1;
	*name = basename;
	return dir;
}

// 取指定路径名文件的 i 节点内部函数
// para : pathname -- 路径名， base -- 搜索起点目录 i 节点，
//        follow_links -- 是否跟随符号链接（ 1--需要，0--不需要）
// return : 返回对应的 i 节点
struct m_inode * _namei(const char * pathname, struct m_inode * base,
	int follow_links)
{
	const char * basename;
	int inr, namelen;
	struct m_inode * inode;
	struct buffer_head * bh;
	struct dir_entry * de;
	// 获得路径最顶层目录 i 节点和 文件名
	if (!(base = dir_namei(pathname, &namelen, &basename, base)))
		return NULL;
	// 处理特殊情况 例如： /usr/
	if (!namelen)			
		return base;
	// 找到文件对应的目录项
	bh = find_entry(&base, basename, namelen, &de);
	if (!bh) {
		iput(base);
		return NULL;
	}
	inr = de->inode;
	brelse(bh);
	// 获得文件 i 节点
	if (!(inode = iget(base->i_dev, inr))) {
		iput(base);
		return NULL;
	}
	// 是否跟随符号链接
	if (follow_links)
		inode = follow_link(base, inode);
	else
		iput(base);
	// 修改最后访问该 i 节点时间
	inode->i_atime = CURRENT_TIME;
	inode->i_dirt = 1;
	return inode;
}

// 取指定路径名的 i 节点，不跟随符号链接
// 从进程所设置的根节点开始搜索
struct m_inode * lnamei(const char * pathname)
{
	return _namei(pathname, NULL, 0);
}

// 取指定路径名的 i 节点，跟随符号链接
// 从进程所设置的根节点开始搜索
struct m_inode * namei(const char * pathname)
{
	return _namei(pathname, NULL, 1);
}

// 完整的文件打开程序
// para : pathname -- 文件名， flag -- 文件读写权限（只读，只写，读写，...）
//        mode -- 文件的访问许可标志， res_inode -- 文件的 i 节点
// return : 0 -- 成功， 否则返回出错码
int open_namei(const char * pathname, int flag, int mode,
	struct m_inode ** res_inode)
{
	const char * basename;
	int inr, dev, namelen;
	struct m_inode * dir, *inode;
	struct buffer_head * bh;
	struct dir_entry * de;
	// 如果设置了文件截 0 标志，且文件以只读模式打开，则添加文件可写权限
	if ((flag & O_TRUNC) && !(flag & O_ACCMODE))
		flag |= O_WRONLY;
	// 当前进程的文件访问屏蔽码
	mode &= 0777 & ~current->umask;
	// 添加普通文件标志
	mode |= I_REGULAR;
	if (!(dir = dir_namei(pathname, &namelen, &basename, NULL)))
		return -ENOENT;
	// 如果文件名长度为0 ，且操作不是 读 写 执行  创建 和 截 0 
	// 则认为是打开目录文件，直接返回目录 i 节点
	if (!namelen) {			/* special case: '/usr/' etc */
		if (!(flag & (O_ACCMODE | O_CREAT | O_TRUNC))) {
			*res_inode = dir;
			return 0;
		}
		iput(dir);
		return -EISDIR;
	}
	bh = find_entry(&dir, basename, namelen, &de);
	// bh ==0 , 表示文件不存在， 新建文件
	if (!bh) {
		// 判断是否 o_creat = 1
		if (!(flag & O_CREAT)) {
			iput(dir);
			return -ENOENT;
		}
		// 判断对文件所在的目录有写的权限
		if (!permission(dir, MAY_WRITE)) {
			iput(dir);
			return -EACCES;
		}
		inode = new_inode(dir->i_dev);                   // ？？？？？？？？？？？？？注意
		if (!inode) {
			iput(dir);
			return -ENOSPC;
		}
		// 设置文件的用户id
		inode->i_uid = current->euid;
		inode->i_mode = mode;
		inode->i_dirt = 1;
		// 添加目录项
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
	// 文件存在
	inr = de->inode;
	dev = dir->i_dev;
	brelse(bh);
	if (flag & O_EXCL) {
		iput(dir);
		return -EEXIST;
	}
	// 跟随符号链接
	if (!(inode = follow_link(dir, iget(dev, inr))))
		return -EACCES;
	// 如果是目录文件且文件访问标志是写或者读写，（目录文件不可进行写操作）
	// 或者没有文件访问权限，返回错误
	if ((S_ISDIR(inode->i_mode) && (flag & O_ACCMODE)) ||
		!permission(inode, ACC_MODE(flag))) {
		iput(inode);
		return -EPERM;
	}
	inode->i_atime = CURRENT_TIME;
	// o_trunc 文件截为 0 标志
	if (flag & O_TRUNC)
		truncate(inode);
	*res_inode = inode;
	return 0;
}
 


// 创建一个设备特殊文件或普通文件节点
// 该函数创建名称为 filename 的文件系统节点，普通文件，设备文件
// para : filename -- 路径名, mode -- 文件类型和访问许可，dev -- 设备号
// return ： 0 -- 成功，否则返回出错码
int sys_mknod(const char * filename, int mode, int dev)
{
	const char * basename;
	int namelen;
	struct m_inode * dir, *inode;
	struct buffer_head * bh;
	struct dir_entry * de;
	// 判断是否超级用户
	if (!suser())
		return -EPERM;
	// 找到路径名最顶端的 i 节点
	if (!(dir = dir_namei(filename, &namelen, &basename, NULL)))
		return -ENOENT;
	// 要创建文件的文件名长度是否为空
	if (!namelen) {
		iput(dir);
		return -ENOENT;
	}
	// 判断是否满足权限要求
	if (!permission(dir, MAY_WRITE)) {
		iput(dir);
		return -EPERM;
	}
	// 查找当前目录下是否存在同名文件
	bh = find_entry(&dir, basename, namelen, &de);
	if (bh) {
		brelse(bh);
		iput(dir);
		return -EEXIST;
	}
	// 获取 i 节点
	inode = new_inode(dir->i_dev);
	if (!inode) {
		iput(dir);
		return -ENOSPC;
	}
	inode->i_mode = mode;
	// 判断是否设备文件
	if (S_ISBLK(mode) || S_ISCHR(mode))
		inode->i_zone[0] = dev;
	inode->i_mtime = inode->i_atime = CURRENT_TIME;
	inode->i_dirt = 1;
	// 添加目录项
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


// 创建一个目录
// para : pathname -- 路径名， mode -- 属性和权限
// return : 0 -- 成功， 否则返回出错码
int sys_mkdir(const char * pathname, int mode)
{
	const char * basename;
	int namelen;
	struct m_inode * dir, *inode;
	struct buffer_head * bh, *dir_block;
	struct dir_entry * de;
    // 路径名的顶层目录 
	if (!(dir = dir_namei(pathname, &namelen, &basename, NULL)))
		return -ENOENT;
	// 要创建的目录名不能为空
	if (!namelen) {
		iput(dir);
		return -ENOENT;
	}
	// 权限检查
	if (!permission(dir, MAY_WRITE)) {
		iput(dir);
		return -EPERM;
	}
	// 检查是否存在同名文件
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
	// 设置目录文件的大小，因为目录文件一定包含两个目录项 (.)和(..)
	// 因此文件大小为两个目录项的长度
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
	// 创建默认目录项 . 和 ..
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
	// 添加目录项 
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


// 检查指定的目录是否为空的子程序
// para : inode -- 目录 i 节点
// return ： 1 -- 目录是空目录
static int empty_dir(struct m_inode * inode)
{
	int nr, block;
	int len;
	struct buffer_head * bh;
	struct dir_entry * de;
    // 根据目录文件的长度获得其包含目录项的的个数
	len = inode->i_size / sizeof(struct dir_entry);
	if (len<2 || !inode->i_zone[0] ||
		!(bh = bread(inode->i_dev, inode->i_zone[0]))) {
		printk("warning - bad directory on dev %04x\n", inode->i_dev);
		return 0;
	}
	// 检查第一个和第二个目录项的内容是否正确
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
		// 不为 0 表示存在文件
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


// 删除一个空目录
// para : name -- 路径名
// return : 0 -- 成功，否则返回错误码
int sys_rmdir(const char * name)
{
	const char * basename;
	int namelen;
	struct m_inode * dir, *inode;
	struct buffer_head * bh;
	struct dir_entry * de;
    // 目录文件所在的目录 i 节点
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
	// 如果设置了受限标志，则仅根用户或者该目录的创建者才可以删除该目录
	if ((dir->i_mode & S_ISVTX) && !suser() &&                          // && current->euid
		inode->i_uid != current->euid) {
		iput(dir);
		iput(inode);
		brelse(bh);
		return -EPERM;
	}
	// i 节点引用计数大于1 表示 有符号链接
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


// 删除文件名对应的目录项
// 如果该文件是最后一个链接， 且没有进程正在打开该文件，则删除该文件
// return : 0 -- 成功， 否则返回出错码
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

// 建立符号链接
// para : oldname -- 原路径名， newname -- 新路径名
// return ： 0 -- 成功， 否则返回出错码
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

// 为文件建立一个文件名目录项
// 为已经存在的文件建立一个新链接，硬链接
// para : oldname -- 原路径名， newname -- 新路径名
// return ： 0 -- 成功， 否则返回出错码
int sys_link(const char * oldname, const char * newname)
{
	struct dir_entry * de;
	struct m_inode * oldinode, *dir;
	struct buffer_head * bh;
	const char * basename;
	int namelen;
	// 获得源文件的 i 节点，其不能是目录文件
	oldinode = namei(oldname);
	if (!oldinode)
		return -ENOENT;
	if (S_ISDIR(oldinode->i_mode)) {
		iput(oldinode);
		return -EPERM;
	}
	// 获得新路径的顶层目录
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
	// 两个路径要在同一个设备下
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

// 打印目录内的文件名
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
			// 如果该块为空，就跳过该块
			if (!(block = bmap(dir, i / DIR_ENTRIES_PER_BLOCK)) ||
				!(bh = bread((dir)->i_dev, block))) {
				i += DIR_ENTRIES_PER_BLOCK;
				continue;
			}
			de = (struct dir_entry *) bh->b_data;
		}
		// 打印目录项
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
	// 打开目录文件
	if((error = open_namei(pathname, flag, 0, &new_pwd)) < 0)
		return error;
	iput(current->pwd);
	current->pwd = new_pwd;
	return 1;
}



// 在指定的目录中找到一个 相应i节点号 的目录项，
// 返回一个含有找到目录项的高速缓冲块以及目录项本身
// para : dir -- 指定目录，inr -- i节点号
// return : 包含找到目录项的高速缓冲块， res_dir -- 找到的目录项
static struct buffer_head * get_entry(struct m_inode ** dir,
	unsigned int inr, struct dir_entry ** res_dir)
{
	int entries;
	int block, i;
	struct buffer_head * bh;
	struct dir_entry * de;

	// 计算目录项个数
	entries = (*dir)->i_size / (sizeof(struct dir_entry));
	*res_dir = NULL;
	// 首先判断目录文件的第一个数据块是否有数据
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
			// 如果该块为空，就跳过该块
			if (!(block = bmap(*dir, i / DIR_ENTRIES_PER_BLOCK)) ||
				!(bh = bread((*dir)->i_dev, block))) {
				i += DIR_ENTRIES_PER_BLOCK;
				continue;
			}
			de = (struct dir_entry *) bh->b_data;
		}
		// 文件名匹配
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


// 打印当前工作目录
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
		// 提取目录名
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
