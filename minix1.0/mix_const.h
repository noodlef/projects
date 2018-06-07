#pragma once
#ifndef CONST_H
#define CONST_H


// i 节点中 i_mode 字段的各个标志位
#define I_TYPE          0170000      // 类型屏蔽码       - 1111 0000 0000 0000
#define I_DIRECTORY	    0040000      // 目录             - 0100 0000 0000 0000 
#define I_REGULAR       0100000      // 正规文件         - 1000 0000 0000 0000
#define I_BLOCK_SPECIAL 0060000      // 块设备           - 0110 0000 0000 0000
#define I_CHAR_SPECIAL  0020000      // 字符设备         - 0010 0000 0000 0000
#define I_NAMED_PIPE	0010000      // 命名管道         - 0001 0000 0000 0000
#define I_SET_UID_BIT   0004000      // 在执行时设置有效用户 id  - 0000 1000 0000 0000 
#define I_SET_GID_BIT   0002000      // 在执行时设置有效组 id    - 0000 0100 0000 0000   
#endif
