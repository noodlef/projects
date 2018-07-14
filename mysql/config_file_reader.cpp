/*
* ConfigFileReader.cpp
*
*  Created on: 2013-7-2
*      Author: ziteng@mogujie.com
*/
#include"util.h"
#include"config_file_reader.h"
#include<map>
#include<string.h>

using namespace std;

config_file_reader::config_file_reader(const char* filename)
{
	_load_config_file(filename);
}

config_file_reader::~config_file_reader()
{
	/* do nothing */;
}

char* config_file_reader::get_config_name(const char* name)
{
	if (!m_load_ok) {
		log("trying to get %s from a \
				     noexsitent config file!", name);
		return NULL;
	}
	char* value = NULL;
	map<string, string>::iterator it = m_config_map.find(name);
	if (it != m_config_map.end()) {
		value = (char*)it->second.c_str();
	}
	return value;
}

int config_file_reader::set_config_value(const char* name, const char*  value)
{
	if (!m_load_ok) {
		log("trying to update %s to a \
				     noexsitent config file!", name);
		return -1;
	}
	map<string, string>::iterator it = m_config_map.find(name);
	if (it != m_config_map.end())
	{
		it->second = value;
	}
	else
	{
		m_config_map.insert(make_pair(name, value));
	}
	return _update_config_file();
}

	void config_file_reader::_pharse_line(char* line)
	{
		char* p = strchr(line, '=');
		if (p == NULL)
			return;

		*p = 0;
		char* key = _trim_space(line);
		char* value = _trim_space(p + 1);
		if (key && value)
		{
			m_config_map.insert(make_pair(key, value));
		}
	}

	char* config_file_reader::_trim_space(char* name)
	{
		int len = strlen(name);
		if (len <= 0)
			return NULL;
		// remove starting space or tab
		char* start_pos = name;
		while (len > 0 && ((*start_pos == ' ') ||
			(*start_pos == '\t')))
		{
			len--;
			start_pos++;
		}

		if (len <= 0)
			return NULL;

		// remove ending space or tab
		char* end_pos = name + strlen(name) - 1;
		while (len > 0 &&
			((*end_pos == ' ') || (*end_pos == '\t')))
		{
			*end_pos = 0;
			len--;
			end_pos--;
		}

		if (len <= 0)
			return NULL;

		return start_pos;
	}

	void config_file_reader::_load_config_file(const char* filename)
	{
		m_config_file.clear();
		m_config_file.append(filename);
		FILE* fp = fopen(filename, "r");
		if (!fp)
		{
			log("can not open %s,errno = %d", filename, errno);
			return;
		}
		// 将配置信息读入 map 
		char buf[256];
		for (;;)
		{
			char* p = fgets(buf, 256, fp);
			if (!p) {
				if (ferror(fp))
					log("load config file failed!");
				break;
			}
			// remove \n at the end
			size_t len = strlen(buf);
			if (buf[len - 1] == '\n')
				buf[len - 1] = 0;
			// remove string start with #
			char* ch = strchr(buf, '#');
			if (ch)
				*ch = 0;
			if (strlen(buf) == 0)
				continue;
			// 解析该行文本内容
			_pharse_line(buf);
		}
		fclose(fp);
		m_load_ok = true;
	}

	int config_file_reader::_update_config_file(const char*filename)
	{
		FILE* fp = NULL;
		if (filename == NULL)
		{
			fp = fopen(m_config_file.c_str(), "w");
		}
		else
		{
			fp = fopen(filename, "w");
		}
		if (fp == NULL)
		{
			log("can't update config file!");
			return -1;
		}
		char szPaire[256];
		map<string, string>::iterator it = m_config_map.begin();
		for (; it != m_config_map.end(); it++)
		{
			memset(szPaire, 0, sizeof(szPaire));
			snprintf(szPaire, sizeof(szPaire), "%s=%s\n", it->first.c_str(), it->second.c_str());
			uint32_t ret = fwrite(szPaire, strlen(szPaire), 1, fp);
			if (ret != 1)
			{
				log("can't update config file!");
				fclose(fp);
				return -1;
			}
		}
		fclose(fp);
		return 0;

	}
