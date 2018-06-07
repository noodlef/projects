#pragma once
#ifndef UTIME_H
#define UTIME_H
#include"mix_type.h"
#include <sys/types.h>	/* I know - shouldn't do this, but .. */

struct utimbuf {
	
	mix_time_t actime;
	mix_time_t modtime;
};

extern int utime(const char *filename, struct utimbuf *times);

#endif