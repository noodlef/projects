#pragma once
#ifndef EXT2_INCLUDE_H
#define EXT2_INCLUDE_H

/**
 * container_of 将指向的结构体成员 member 的指针 ptr 
 * 转换成指向结构体type的指针
 */
#define offsetof(TYPE, MEMBER)              ((size_t)&((TYPE *)0)->MEMBER)
#define container_of(ptr, type, member)     (type*)((char*)(ptr) - offsetof(type, member))


#endif