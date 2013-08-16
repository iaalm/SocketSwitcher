CC = $(PREFIX)g++
CXX= $(PREFIX)g++
LD = $(PREFIX)ld

CPPFLAGS = -I./include
CXXFLAGS =  -fpermissive -pthread -g
CFLAGS =  -fpermissive -pthread -g
LDFLAGS = -lm -pthread -g -TSocketSwitcher.lds

all:SocketSwitcher

lib_json/json.a:
	cd lib_json;CXX=${CXX} $(MAKE)

SocketSwitcher.lds:
	$(LD) -verbose > SocketSwitcher.lds
	sed -i '$$d' SocketSwitcher.lds 
	sed -i '1,10d' SocketSwitcher.lds
	sed -i '/\.text\ *:/i.initlist : {\n__initlist_start = .;\n*(.initlist)\n__initlist_end = .;\n}' SocketSwitcher.lds

SocketSwitcher:SocketSwitcher.o lib_json/json.a SocketSwitcher.lds
	$(CXX) $(LDFLAGS) SocketSwitcher.o lib_json/json.a  -o $@

clean:
	rm -f SocketSwitcher SocketSwitcher.lds
	find . -name \*.o -delete
	find . -name \*.a -delete

