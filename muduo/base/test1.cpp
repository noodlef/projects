#include<boost/ptr_container/ptr_vector.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
#include "log_stream.h"
int f()
{
    //typedef muduo::detail::fixed_buffer<muduo::detail::klarge_buffer> buffer_t;
    typedef int buffer_t;
    typedef boost::ptr_vector<buffer_t> buffer_vector;
    typedef buffer_vector::auto_type buffer_ptr;
    buffer_vector buffers();
    //buffer_t* p = new buffer_t();
    //buffer_ptr p(new buffer_t);
    //p->reset(new buffer_t);
   // boost::ptr_vector<int> v;
    //v.push_back(new int(1));
   // v.push_back(new int(2));
}
