#pragma once
#ifndef CONST_H
#define CONST_H


// i �ڵ��� i_mode �ֶεĸ�����־λ
#define I_TYPE          0170000      // ����������       - 1111 0000 0000 0000
#define I_DIRECTORY	    0040000      // Ŀ¼             - 0100 0000 0000 0000 
#define I_REGULAR       0100000      // �����ļ�         - 1000 0000 0000 0000
#define I_BLOCK_SPECIAL 0060000      // ���豸           - 0110 0000 0000 0000
#define I_CHAR_SPECIAL  0020000      // �ַ��豸         - 0010 0000 0000 0000
#define I_NAMED_PIPE	0010000      // �����ܵ�         - 0001 0000 0000 0000
#define I_SET_UID_BIT   0004000      // ��ִ��ʱ������Ч�û� id  - 0000 1000 0000 0000 
#define I_SET_GID_BIT   0002000      // ��ִ��ʱ������Ч�� id    - 0000 0100 0000 0000   
#endif
