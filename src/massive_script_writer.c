#include <stdlib.h>
#include <stdio.h>
#include <string.h>


#ifdef _WIN32
typedef struct _iobuf FILE;
#else
typedef struct _IO_FILE FILE;
#endif

static char param_names[16] = {0};

#define WRITE(STR, ...) fprintf(file, STR, ##__VA_ARGS__)


#define RANDS \
	X(0f)\
	X(ff)

#define X(A) static inline int randx##A() { return rand() & 0x##A; }
RANDS
#undef X

static int num_vars = 0;
static int num_funcs = 0;

void end_line(FILE* file, const int comment_word_count)
{
	if (comment_word_count)
	{
		char* temp = alloca(4 * comment_word_count + 1);
		for (int i = 0; i < comment_word_count; ++i)
		{
			memcpy(temp + 4 * i, " XDD", 4);
		}
		temp[4 * comment_word_count] = '\0';
		WRITE(" #%s\n", temp);
	}
	else
	{
		WRITE("\n");
	}
}

void multiline_comment(FILE* file, const int num_lines_comments)
{
	for (int i = 0; i < num_lines_comments; ++i)
	{
		end_line(file, randx0f() + 1);
	}
}

void write_variable(FILE* file, const int var_num)
{
	WRITE("var var%d: int = %d", var_num, rand());
	end_line(file, randx0f());
}

void write_params(FILE* file, const int num_params)
{
	WRITE("(");
	for (int32_t i = 0; i < num_params; ++i)
	{
		if (i < num_params - 1)
		{
			WRITE("var %c, ", param_names[i]);
		}
		else
		{
			WRITE("var %c", param_names[i]);
		}
	}
	WRITE(")");
}

void write_body(FILE* file, const int num_lines)
{
	for (int i = 0; i < num_lines; ++i)
	{
		if (randx0f())
		{
			WRITE("\tvar%d = var%d + var%d", rand() % num_vars, rand() % num_vars, rand() % num_vars);
			end_line(file, randx0f());
		}
		else
		{
			const int comment_line_count = randx0f() + 2;
			for (int j = 0; j < comment_line_count; ++j)
			{
				const int comment_word_count = randx0f() + 1;
				char* temp = alloca(4 * comment_word_count + 1);
				for (int k = 0; k < comment_word_count; ++k)
				{
					memcpy(temp + 4 * k, " XDD", 4);
				} temp[4 * comment_word_count] = '\0'; WRITE("\t#%s\n", temp);
			}
		}
	}
}

void write_func(FILE* file, const int func_num)
{
	WRITE("func f%d", func_num);
	write_params(file, randx0f());
	WRITE(" -> int:\n");
	write_body(file, randxff());
	WRITE("\treturn var%d", rand() % num_vars);
	end_line(file, randx0f());
	WRITE("\n");
}

int main(int argc, char** argv)
{
	srand(22);
	for (int i = 0; i < 16; ++i)
	{
		param_names[i] = 'a' + i;
	}
	FILE* file = fopen("Massive.gd", "w");
	fprintf(file, "class Massive\nextends Node\n\n");
	num_vars = 10000;
	num_funcs = 10000;
	for (int i = 0; i < num_vars; ++i)
	{
		write_variable(file, i);
	}
	for (int i = 0; i < num_funcs; ++i)
	{
		write_func(file, i);
	}
	fclose(file);
	return 0;
}
