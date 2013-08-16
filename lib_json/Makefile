CPPFLAGS=-I../include
CXXFLAGS=-fPIC -g
OBJECT=json_reader.o json_value.o json_writer.o
json.a:$(OBJECT)
	$(CXX) -shared $(OBJECT) -o json.a
clean:
	rm -f $(OBJECT) json.a
