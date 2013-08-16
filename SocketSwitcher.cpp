#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <assert.h>
#include <sys/socket.h>
#include <stdarg.h>
#include <string>
#include <map>
#include <mysocket.h>
#include "SocketSwitcher.h"

extern init_function __initlist_start;
extern init_function __initlist_end;
std::map<std::string,callback_function> callback_tab;

bool regist_action(std::string action,callback_function fun){
	bool flag = true;
	if(callback_tab[action])
		flag = false;
	callback_tab[action] = fun;
	return flag;
}
bool sendback(int fd,msg_t msg){
	Json::FastWriter writer;
	std::string str = writer.write(msg);
	str.append("\n");
	send(fd,str.c_str(),str.length(),0);
	if(errno == EBADF){
		errno = 0;
		return false;
	}
	return true;
}

void log(enum level_t level,char *format, ...){
	if(level > REPORT_LEVEL)
		return ;
	va_list args;
	va_start(args,format);
		vfprintf(stderr,format,args);
		fprintf(stderr,"\n");
	va_end(args);
}

class sock{
	private:
		static sock *instance;
		int listen_socket;
		sock(int port){assert_perror((listen_socket = open_listenfd(port)) < 0);instance = this;}
	public:
		static int init(msg_t msg){new sock(msg["ccu"]["port"].asInt());return 0;}
		static sock* getInstance(){return instance;}
		int accept_id(){
			struct sockaddr addr;
			socklen_t len;
			return accept(listen_socket,&addr,&len);
		}
};
sock* sock::instance;

#define BUF_MAX 1000
static inline int get_str_from_fd(int fd,std::string& str){
	str = "";
	char buf[BUF_MAX+1] = "",*ptr = NULL;
	int ret = 0,len = 0;
	while(!ptr){
		len += ret;
		str.append(buf);
		ret = recv(fd,buf,BUF_MAX,0);
		if(ret == 0)
			return 0;
		buf[ret] = '\0';
		log(INFO,"[SYS INFO]\t%s",buf);
		(ptr = strstr(buf,"\n\n")) ||
			(ptr = strstr(buf,"\r\n\r\n")) ||
			(ptr = strstr(buf,"\r\r")) ||
			(ptr = strstr(buf,"\n")) ||
			(ptr = strstr(buf,"\r\n"));
	}
	str.append(buf,ptr-buf);
	return (len + ptr - buf);
}

#undef BUF_MAX

#define BUF_MAX 1000
static inline msg_t get_config_from_file(char* name){
	Json::Reader reader;
	msg_t msg;
	std::string str;
	char buf[BUF_MAX];
	int ret;
	int fd = open(name,O_RDONLY);
	while((ret = read(fd,buf,BUF_MAX)) == BUF_MAX)
		str.append(buf,BUF_MAX);
	str.append(buf,ret);
	reader.parse(str,msg,false);
	return msg;
}
#undef BUF_MAX
static inline int get_msg_source(int &id){
	id = sock::getInstance()->accept_id();
	return 1;
}
static inline int get_msg(msg_t& msg,int id){
	int ret;
	std::string str;
	Json::Reader reader;

	msg.clear();
	assert_perror(id < 0);
	if(!get_str_from_fd(id,str))
		return 0;
	reader.parse(str,msg,false);

	return 1;
}
static inline void* process_msg_thread(void *data){
	msg_t msg;
	int id = (int) data;

	while(get_msg(msg,id)){	//the difference between single and multi
		if(callback_tab.find(msg["action"].asString()) == callback_tab.end()){
			log(WARN,"[SYS WARN]\twrong action!");
			continue;
		}
		callback_tab[msg["action"].asString()](id,msg["param"]);
	}

	close(id);
	pthread_exit(0);
}
static inline void process_msg(int id){
	pthread_t thread;
	pthread_create(&thread,NULL,process_msg_thread,(void*) id);
	pthread_detach(thread);
}

void list(int fd,msg_t msg){
	std::map<std::string,callback_function>::iterator it,end = callback_tab.end();
	Json::Value result;
	result["result"] = "list";
	for(it = callback_tab.begin();it != end;it++){
		result["param"].append(it->first);
	}
	sendback(fd,result);
}
void close(int fd,msg_t msg){
	close(fd);
	pthread_exit(0);
}

int main(int argc, const char *argv[])
{
	init_function *fun;
	int id;
	//init
	signal(SIGPIPE,SIG_IGN);
	callback_tab["list"] = list;
	callback_tab["close"] = close;
	msg_t config = get_config_from_file("config.json");
	sock::init(config);
	for(fun = &__initlist_start;fun < &__initlist_end;fun++)
		assert_perror((*fun)(config) < 0);

	//main message loop
	while(get_msg_source(id)){
		process_msg(id);
	}

	return 0;
}
