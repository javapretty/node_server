#ifndef __MSG_TOOL__
#define __MSG_TOOL__

#include <stdlib.h>
#include "Struct_Manager.h"
#include "Log.h"

#define BEGIN_IMPLEMENT "\nfunction %s() {\n"
#define END_IMPLEMENT "}\n"
#define RESET_ZERO "\tthis.%s = 0;\n"
#define RESET_STRING "\tthis.%s = \"\";\n"
#define RESET_VECTOR "\tthis.%s = new Array();\n"
#define RESET_MAP "\tthis.%s = new Map();\n"
#define RESET_STRUCT "\tthis.%s = new %s();\n"
#define BEGIN_MESSAGE "if (typeof Msg == \"undefined\") {\n"\
										"\tvar Msg = {};\n"
#define MESSAGE_BODY "\tMsg.%s = %d;\n"
#define END_MESSAGE END_IMPLEMENT

class Msg_Tool {
public:
	Msg_Tool();
	~Msg_Tool();
	int write_struct();
private:
	int write_to_struct(FILE *fp);
	int write_to_message(FILE *fp);
private:
};

#endif

