#调试，若不需要调试则注释
CPPFLAGS+=-g

#目标文件从当前目录和指定子目录中寻找
OBJ=$(wildcard *.cpp) $(wildcard Server/*.cpp) $(wildcard Log/*.cpp)

all:XWebServer 

submkdir:
	cd Log && $(MAKE);
	cd Server && $(MAKE) 

XWebServer :submkdir $(OBJ:.cpp=.o)
	g++  -g $(OBJ:.cpp=.o) -lpthread -lmysqlclient -o  XWebServer 


source=$(wildcard *.cpp)
include $(source:.cpp=.d)

%.d: %.cpp
	set -e; rm -f $@; \
	g++ -MM $(CPPFLAGS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

# 因为在server和log目录下的d，第一行.o文件不带文件路径，如果直接根据%.d模式查找.d中的.o行，只能在当前目录下找到

#sed's标识符old标识符new'<file> 输出文件,表示将file中的old字段替换为new字段，结果记录到输出文件，s表示替换，\1表示old，若只替换第一条匹配行，则可添加g选项

#伪目标稳健写法，避免出现同名文件
.PHONY: clean	
clean:
	-rm main.d main.o XWebServer Log/*.o Server/*.o  *.d*  Log/*.d*  Server/*.d*


