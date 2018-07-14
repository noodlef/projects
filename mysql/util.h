#ifndef _UTIL_H_
#define _UTIL_H_

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<strings.h>
#include<stdint.h>
#include<map>
#include<string>
#include<errno.h>
#include<errno.h>
#include<errno.h>

#include<assert.h>
#include<sys/stat.h>

#define log  printf

using std::string;
using std::map;

class CStrExplode
{
public:
	CStrExplode(char* str, char seperator);
	virtual ~CStrExplode();

	uint32_t GetItemCnt() { return m_item_cnt; }
	char* GetItem(uint32_t idx) { return m_item_list[idx]; }
private:
	uint32_t	m_item_cnt;
	char** 		m_item_list;
};

#endif
