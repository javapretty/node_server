/*
 * Struct_Tool.h
 *  Created on: Oct 29, 2016
 *      Author: zhangyalei
 */

#ifndef STRUCT_TOOL_
#define STRUCT_TOOL_

#include <stdlib.h>

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

class Struct_Tool {
public:
	Struct_Tool();
	~Struct_Tool();
	int write_struct();

private:
	int write_to_struct(FILE *fp);
	int write_to_message(FILE *fp);
};

#endif

