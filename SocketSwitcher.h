#ifndef _CCU_
#define _CCU_

#include <json/value.h>
#include <json/reader.h>
#include <json/writer.h>

typedef Json::Value msg_t;
typedef void (*callback_function)(int,msg_t);
typedef int (*init_function)(msg_t);
#define INIT_FUNCTION(fn)  static init_function initcall_##fn __attribute_used__ __attribute__((__section__(".initlist"))) = fn
enum level_t{NONE,ERR,WARN,INFO};
#define REPORT_LEVEL INFO

bool sendback(int,msg_t);
bool regist_action(std::string,callback_function);
void log(enum level_t level,char *format, ...);
#endif
