##############################################################################
#                                                                            #
#        默认为release模式编译，debug模式请使用参数'MODE=DEBUG'              #
#                                                                            #
##############################################################################

###在这里添加源文件目录###
SRCDIR=	./\

###这里定义目标文件目录###
OBJDIR =../../obj/log_obj/

TARGET_NAME=logplugin.so

BIN=../

INCLUDE=-I/usr/local/include/nodelib/base\
		-I/usr/local/include/nodelib/db\
		-I/usr/local/include/nodelib/network\
		-I/usr/local/include/nodelib/node\
		-I/usr/local/include/nodelib/struct\
		-I/usr/local/include/nodelib/v8\
		-I/usr/local/include/nodelib/xml\
		$(addprefix -I, $(SRCDIR))

LIBDIR=-L./

LIB=-lv8\
	-lv8_libplatform\
	-lmysqlcppconn\

CC=g++

DEPENDS=-MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)"

ifeq ($(MODE), DEBUG)
BUILD_FLAG = -O0 -g3
else
BUILD_FLAG = -O3
endif

COM_FLAG=-Wall -c -fmessage-length=0 -std=c++11 -fPIC

LDFLAG=-shared

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

$(OBJDIR)%.o:%.cpp
ifeq ($(MODE), DEBUG)
	@echo "Building DEBUG MODE target $@"
else
	@echo "Building RELEASE MODE target $@"
endif
	$(CC) $(INCLUDE) $(BUILD_FLAG) $(COM_FLAG) $(DEPENDS) -o "$(@)" "$(<)"
	@echo " "

mkobjdir:
	@test -d $(OBJDIR) || (mkdir $(OBJDIR) && mkdir $(OBJDIR)bin $(addprefix $(OBJDIR), $(subst ./,,$(SRCDIR))))

clean:
	-rm -rf $(OBJDIR)
