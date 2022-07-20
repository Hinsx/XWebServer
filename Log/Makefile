#调试，若不需要调试则注释
CPPFLAGS+=-g

OBJ=$(wildcard *.cpp)

all:$(OBJ:.cpp=.o)


include $(OBJ:.cpp=.d)

%.d: %.cpp
	set -e; rm -f $@; \
	g++ -MM $(CPPFLAGS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

.PHONY: clean	
clean:
	-rm *.o *.d