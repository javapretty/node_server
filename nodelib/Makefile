##############################################################################
#                                                                            #
#        默认为release模式编译，debug模式请使用参数'MODE=DEBUG'              #
#        默认不编译MongoDB功能，想编译的话请使用参数'MONGO=TRUE'             #
#                                                                            #
##############################################################################

###在这里添加源文件目录###
SRCDIR=	./\
		./base/\
		./db/\
		./network/\
		./node/\
		./struct/\
		./v8/\
		./xml/

###这里定义目标文件目录###
OBJDIR =./.obj/

TARGET_NAME=libnodelib.so

BIN=./

INCLUDE=$(addprefix -I, $(SRCDIR))

LIBDIR=-L./

LIB=-lv8\
	-lv8_libplatform\
	-lcurl\
	-lcrypto\
	-ldl\
	-lz\
	-ljsoncpp\
	-lmysqlcppconn\
	$(MONGO_LIB)

CC=g++

DEPENDS=-MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)"

ifeq ($(MODE), DEBUG)
BUILD_FLAG = -O0 -g3
else
BUILD_FLAG = -O3
endif

ifeq ($(MONGO), TRUE)
CONDITION=-D MONGO_DB_IMPLEMENT
MONGO_LIB=-lmongoclient\
		  -lboost_filesystem\
		  -lboost_thread-mt
else
CONDITION=
MONGO_LIB=
endif

COM_FLAG=-Wall -fPIC -shared -c -fmessage-length=0 -std=c++11

LDFLAG=-shared

BIN_TARGET=$(OBJDIR)bin/$(TARGET_NAME)

SRCS=$(wildcard $(addsuffix *.cpp, $(SRCDIR)))

OBJECTS:=$(addprefix $(OBJDIR), $(subst ./,,$(SRCS:.cpp=.o)))

.PHONY:all mkobjdir clean config

all:mkobjdir $(BIN_TARGET)

-include $(OBJECTS:.o=.d)

$(BIN_TARGET):$(OBJECTS)
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
	$(CC) $(INCLUDE) $(BUILD_FLAG) $(COM_FLAG) $(DEPENDS) $(CONDITION) -o "$(@)" "$(<)"
	@echo " "

mkobjdir:
	@test -d $(OBJDIR) || (mkdir $(OBJDIR) && mkdir $(OBJDIR)bin $(addprefix $(OBJDIR), $(subst ./,,$(SRCDIR))))

clean:
	-rm -rf $(OBJDIR)