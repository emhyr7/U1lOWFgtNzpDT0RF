#include "code.c"

int main(int argc, char **argv)
{
	initialize_base();

	if (argc <= 1)
	{
		REPORT_FAILURE("A source path wasn't given.\n");
		return -1;
	}

	program program;

	parser parser;
	parser_parse(argv[1], &program, &parser);
	return 0;
}
