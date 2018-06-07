// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#include "muduo_latch.h"

using namespace muduo;

count_down_latch::count_down_latch(int count)
	: _mutex(), _condition(_mutex), _count(count)
{ }

void count_down_latch::wait()
{
	mutex_lock_guard lock(_mutex);
	while (_count > 0)
	{
		_condition.wait();
	}
}

void count_down_latch::count_down()
{
	mutex_lock_guard lock(_mutex);
	--_count;
	if (_count == 0)
	{
		_condition.notify_all();
	}
}

int count_down_latch::get_count() const
{
	mutex_lock_guard lock(_mutex);
	return _count;
}
