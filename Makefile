##############################################################################
#                                                                            #
#        默认为release模式编译，debug模式请使用参数'MODE=DEBUG'              #
#                                                                            #
##############################################################################

###在这里添加源文件目录###
SRCDIR=	./\

###这里定义目标文件目录###
OBJDIR =./obj/

TARGET_NAME=node_server

BIN=./

INCLUDE=-I/usr/local/include/nodelib/base\
		-I/usr/local/include/nodelib/network\
		-I/usr/local/include/nodelib/node\
		-I/usr/local/include/nodelib/struct\
		-I/usr/local/include/nodelib/v8\
		-I/usr/local/include/nodelib/xml\
		$(addprefix -I, $(SRCDIR))

LIBDIR=-L./

LIB=-lnodelib\
	-lv8\
	-lv8_libplatform\
	-lcurl\
	-lcrypto\
	-ljsoncpp\
	-ldl\
	$(MONGO_LIB)

CC=g++

DEPENS=-MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)"

ifeq ($(MODE), DEBUG)
DEBUG_FLAG = -O0 -g3
else
DEBUG_FLAG = -O3
endif

CONDITION=

COM_FLAG=-Wall -c -fmessage-length=0 -std=c++11

LDFLAG=

BIN_TARGET=$(OBJDIR)bin/$(TARGET_NAME)

SRCS=$(wildcard $(addsuffix *.cpp, $(SRCDIR)))

OBJECTS:=$(addprefix $(OBJDIR), $(subst ./,,$(SRCS:.cpp=.o)))

.PHONY:all mkobjdir clean config

all:mkobjdir $(BIN_TARGET)

-include $(OBJECTS:.o=.d)

$(BIN_TARGET):$(OBJECTS)
	@echo "Linking target $@"
	$(CC) $(LDFLAG) -o $@ $^ $(LIBDIR) $(LIB)
	@echo " "
	@echo "Finished building target: $(TARGET_NAME)"
	@echo " "
	@-cp -f $(BIN_TARGET) $(BIN)
	@-sh make_plugin.sh

$(OBJDIR)%.o:%.cpp
ifeq ($(MODE), DEBUG)
	@echo "Building DEBUG MODE target $@"
else
	@echo "Building RELEASE MODE target $@"
endif
	$(CC) $(INCLUDE) $(DEBUG_FLAG) $(COM_FLAG) $(DEPENS) $(CONDITION) -o "$(@)" "$(<)"
	@echo " "

mkobjdir:
	@test -d $(OBJDIR) || (mkdir $(OBJDIR) && mkdir $(OBJDIR)bin $(addprefix $(OBJDIR), $(subst ./,,$(SRCDIR))))

config:
	@tar -zvx -f doc/nodelib.tar.gz -C doc
	@mv doc/nodelib/libnodelib.so /usr/local/lib64/
	@cp -rf doc/nodelib/* /usr/local/include/nodelib
	@rm -rf doc/nodelib/
	@echo "install nodelib file success"
	@tar -zvx -f doc/v8.tar.gz -C doc
	@cp -rf doc/v8/include/* /usr/local/include/include
	@cp -rf doc/v8/*.so /usr/local/lib64
	@cp -rf doc/v8/*.bin ./
	@rm -rf doc/v8/
	@ldconfig

clean:
	-rm -rf $(OBJDIR)
