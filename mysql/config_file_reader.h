#pragma once
/*
* ConfigFileReader.h ， 读服务器配置文件的类
*
* Created on: 2013-7-2
*     Author: ziteng@mogujie.com
*/

#ifndef CONFIGFILEREADER_H_
#define CONFIGFILEREADER_H_

// 包含所需的头文件
#include<map>
#include"util.h"
class config_file_reader
{
private:
	bool m_load_ok;
	map<string, string> m_config_map;
	string m_config_file;
public:
	config_file_reader(const char* filename);
	~config_file_reader();
	char* get_config_name(const char* name);
	int set_config_value(const char* name, const char*  value);
	
private:
	void _pharse_line(char* line);
	
	char* _trim_space(char* name);
	
	void _load_config_file(const char* filename);
	

	int _update_config_file(const char*filename = NULL);

};

#endif /* CONFIGFILEREADER_H_ */

