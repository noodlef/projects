#pragma once
#ifndef _OSTYPE_H
#define OSTYPE_H
// OS dependant type definition
	#include <sys/epoll.h>
	#include <pthread.h>
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <sys/ioctl.h>
	#include <netinet/in.h>
	#include <netinet/tcp.h>
	#include <arpa/inet.h>
	#include <netdb.h>
	#include <errno.h>
	#include <unistd.h>
	#include <fcntl.h>
	#include <stdint.h>		          // define int8_t ...
	#include <signal.h>
	#include <unistd.h>
	#define closesocket close
	#define ioctlsocket ioctl
#include <stdexcept>                 // for exception

/*
 * hash_map<string>
 */
#ifdef __GNUC__                      // for gun compiler
        #include<hash_map>
	using namespace __gnu_cxx;
	namespace __gnu_cxx {
		template<> struct hash<std::string> {
			size_t operator()(const std::string& x) const {
				return hash<const char*>()(x.c_str());
			}
		};
}
#else
	#include <hash_map>
	using namespace stdext;
#endif

#ifdef _WIN32
	#ifndef _WIN32
		typedef char			int8_t;
		typedef short			int16_t;
		typedef int				int32_t;
		typedef	long long		int64_t;
		typedef unsigned char	uint8_t;
		typedef unsigned short	uint16_t;
		typedef unsigned int	uint32_t;
		typedef	unsigned long long	uint64_t;
	#endif//_WIN32
	typedef int				socklen_t;

#else
	typedef int	SOCKET;
	typedef int BOOL;
	#ifndef  __APPLE__
		const int TRUE = 1;
		const int FALSE = 0;
	#endif
	const int SOCKET_ERROR = -1;
	const int INVALID_SOCKET = -1;
#endif


typedef unsigned char	uchar_t;
typedef int				net_handle_t;
typedef int				conn_handle_t;
typedef int             socket_t;

// 
enum {
	NETLIB_OK = 0,
	NETLIB_ERROR = -1
};

#define  NETLIB_INVALID_HANDLE -1

// 回调函数的参数 msg 的取值类型
enum
{
	NETLIB_MSG_CONNECT = 1,
	NETLIB_MSG_CONFIRM,
	NETLIB_MSG_READ,
	NETLIB_MSG_WRITE,
	NETLIB_MSG_CLOSE,
	NETLIB_MSG_TIMER,
	NETLIB_MSG_LOOP
};

const uint32_t INVALID_UINT32 = (uint32_t)-1;
const uint32_t INVALID_VALUE = 0;

// 回调函数指针类型 callback_t
typedef void(*callback_t)(void* callback_data, uint8_t msg, uint32_t handle, void* pParam);
#endif

