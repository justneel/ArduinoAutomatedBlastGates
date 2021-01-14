#ifndef Log_h
#define Log_h

#define _PRINTF_BUFFER_LENGTH_		64
#define _Stream_Obj_				Serial




#if 1
static char _pf_buffer_[_PRINTF_BUFFER_LENGTH_];
#else
extern char _pf_buffer_[_PRINTF_BUFFER_LENGTH_];
#endif


// #define debug(a,...)														\
// 	do{																		\
// 	snprintf(_pf_buffer_, sizeof(_pf_buffer_), a, ##__VA_ARGS__);			\
// 	_Stream_Obj_.print(_pf_buffer_);										\
// 	}while(0)

#define debug(a,...)														\
	do{																		\
	snprintf(_pf_buffer_, sizeof(_pf_buffer_), a"\r\n", ##__VA_ARGS__);		\
	_Stream_Obj_.print(_pf_buffer_);										\
	}while(0)



#endif