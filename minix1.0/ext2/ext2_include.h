#pragma once
#ifndef EXT2_INCLUDE_H
#define EXT2_INCLUDE_H

/**
 * container_of ��ָ��Ľṹ���Ա member ��ָ�� ptr 
 * ת����ָ��ṹ��type��ָ��
 */
#define offsetof(TYPE, MEMBER)              ((size_t)&((TYPE *)0)->MEMBER)
#define container_of(ptr, type, member)     (type*)((char*)(ptr) - offsetof(type, member))


#endif