#include "xml.h"

// jfc you look this up so often, why haven't you memorized the link yet?
// https://www.ascii-code.com/
static void end_of_string(int32_t* index, const char c0, const char* src, const int32_t n)
{
	char prev_c = c0;
	for (int32_t i = *index + 1; i < n; ++i)
	{
		const char c = src[i];
		if (c != c0 || prev_c == '\\')
		{
			prev_c = c;
		}
		else
		{
			*index = i;
			return;
		}
	}
}


// next tag ends at character following the name of a tag
// moves pos to following closing " or '
xml_ret_t xml_next_attribute(char** attribute, int32_t* a_len, char** value, int32_t* v_len, xml_t* xml)
{
	const int32_t n = xml->len;
	const char* src = xml->src;
	for (int32_t i = xml->pos; i < n; ++i)
	{
		const char c0 = src[i];
		if (c0 == ' ' || c0 == '\t' || c0 == '\n' || c0 == '\r') /* CRY ABOUT IT */ ;
		else
		{
			*attribute = src + i;
			const int32_t i0 = i;
			int32_t i1;
			for (++i; i < n; ++i)
			{
				const char c1 = src[i];
				if (('a' <= c1 && c1 <= 'z') || ('A' <= c1 && c1 <= 'Z') || c1 == '_')
				{
					i1 = i;
				}
				else if (!(c1 == '"' || c1 == '\''));
				else
				{
					*a_len = i1 - i0 + 1;
					*value = src + i + 1;
					i1 = i;
					end_of_string(&i, c1, src, n);
					*v_len = i - i1 - 1;
					xml->pos = i + 1;
					return XML_SUCCESS;
				}
			}
		}
	}
	xml->pos = n - 1;
	return XML_BAD;
}

uint8_t xml_next_elem(char** str, int32_t* len, xml_t* xml, xml_t* attributes_xml, xml_t* nested_xml)
{
	// find next < but make sure it's not </
	const int32_t n = xml->len;
	const char* src = xml->src;
	uint8_t res = XML_BAD;
	int32_t i = xml->pos;
	int32_t attr_i0;
	int32_t nest_i0;
	for (; i < n - 1; ++i)
	{
		const char c0 = src[i];
		const char c1 = src[i + 1];
		if (c0 == '"' || c0 == '\'')
		{
			end_of_string(&i, c0, src, n);
		}
		/* else if (c0 == '<' && c1 != '/' && c1 != '?') // I HATE THIS */
		else if (c0 != '<' || (c1 == '/' || c1 == '?')); // THANKS, DEMORGEN
		else
		{
			const int32_t i0 = ++i;
			*str = src + i0;
			res = XML_SUCCESS;
			for (; i < n; ++i)
			{
				const char c = src[i];
				if (!(c == ' ' || c == '>' || c== '/'|| c == '\t' || c == '\n' || c == '\r'));
				else if (!(c == '>' || c == '/'))
				{
					*len = i - i0;
					*attributes_xml = (xml_t){ .src = src + i + 1 };
					goto attributes;
				}
				else if (c == '>')
				{
					*len = i - i0;
					++i;
					*attributes_xml = (xml_t){ 0 };
					goto nested;
				}
				else // c == '/'
				{
					*len = i - i0;
					i += 2;
					*attributes_xml = (xml_t){ 0 };
					*nested_xml = (xml_t){ 0 };
					goto finish;
				}
			}
		}
	}
	*len = 0;
	xml->pos = n - 1;
	return XML_BAD;
attributes:
	res |= XML_ATTRIBUTES;
	attr_i0 = i;
	for (; i < n; ++i)
	{
		const char c = src[i];
		if (c == '"' || c== '\'')
		{
			end_of_string(&i, c, src, n);
		}
		else if (!(c == '>' || c == '/'));
		else if (c == '>')
		{
			++i;
			attributes_xml->len = i - attr_i0;
			goto nested;
		}
		else // c -- '/'
		{
			i += 2;
			attributes_xml->len = i - attr_i0;
			*nested_xml = (xml_t){ 0 };
			goto finish;
		}
	}
nested:
	res |= XML_HAS_NESTED;
	nest_i0 = i;
	int32_t count = 0;
	for (; i < n - 1; ++i)
	{
		const char c = src[i];
		if (c == '"' || c == '\'')
		{
			end_of_string(&i, c, src, n);
		}
		else if (! (c == '<' || c == '/'));
		else if (c == '<')
		{
			const char c1 = src[i + 1];
			count += (c1 != '/');
			count -= (c1 == '/');
			i += c1 == '/';
			if (count >= 0);
			else break;
		}
		else // c == '/'
		{
			--count;
		}
	}
	*nested_xml = (xml_t)
	{
		.src = src + nest_i0,
		.len = i - nest_i0
	};
	// swoos to end of closing tag
	for (; i < n; ++i)
	{
		const char c = src[i];
		if (c != '>');
		else
		{
			++i;
			break;
		}
	}
finish:
	xml->pos = i;
	return res;
}


#include <assert.h>
#include <stdio.h>
#include "mem_sys.h"

static inline void print_line(const xml_t* xml)
{
	const int32_t n = xml->len;
	const char* src = xml->src;
	for(int32_t i = xml->pos; i < n; ++i)
	{
		const char c = src[i];
		printf("%c", c);
		if (c != '\n');
		else
		{
			break;
		}
	}
}

int xml_test(int argc, char** argv)
{
	ms_handle_t ms_handle;
	int32_t file_len;
	char* xml_str = ms_map_entire_file("./doc/classes/Node.xml", &file_len, &ms_handle);
	char* elem;
	int32_t len;
	uint8_t top_res;
	xml_t xml = { .src = xml_str, .len = file_len };
	xml_t attr_xml;
	xml_t nest_xml;
	top_res = xml_next_elem(&elem, &len, &xml, &attr_xml, &nest_xml);
	printf("first_elem: '");
	for (int32_t i = 0; i < len; ++i)
	{
		printf("%c", elem[i]);
	}
	printf("'\n");
	printf("xml len: %d, xml pos: %d\n", len, xml.len, xml.pos);
	printf("attribute len: %d, nested len: %d\n", attr_xml.len, nest_xml.len);
	char* attr;
	char* value;
	int32_t a_len;
	int32_t v_len;
	while (xml_next_attribute(&attr, &a_len, &value, &v_len, &attr_xml) & XML_SUCCESS)
	{
		printf("> ");
		for (int32_t i = 0; i < a_len; ++i)
		{
			printf("%c", attr[i]);
		}
		printf(": ");
		for (int32_t i = 0; i < v_len; ++i)
		{
			printf("%c", value[i]);
		}
		printf("\n");
	}
	printf("nested:\n");
	xml_t discard = { 0 };
	uint8_t res;
	while ((res = xml_next_elem(&elem, &len, &nest_xml, &discard, &discard)) & XML_SUCCESS)
	{
		printf("> ");
		for (int32_t i = 0; i < len; ++i)
		{
			printf("%c", elem[i]);
		}
		printf(": %s attributes; %s nested\n", res & XML_ATTRIBUTES ? "has" : "no", res & XML_HAS_NESTED ? "has" : "no");
	}
	ms_munmap_file(xml_str, len, ms_handle);
	return 1 - (top_res & XML_SUCCESS);
}
