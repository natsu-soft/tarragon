#include "xml.h"
#include <stdlib.h>
#include <string.h>

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
xml_ret_t next_attribute(char** attribute, int32_t* a_len, char** value, int32_t* v_len, xml_t* xml)
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
					xml->pos = i;
					return XML_SUCCESS;
				}
			}
		}
	}
	xml->pos = n - 1;
	return XML_BAD;
}

uint8_t next_elem(char** str, int32_t* len, xml_t* xml, xml_t* attributes_xml, xml_t* nested_xml)
{
	// find next < but make sure it's not </
	const int32_t n = xml->len;
	const char* src = xml->src;
	for (int32_t i = xml->pos; i < n - 1; ++i)
	{
		const char c0 = src[i];
		const char c1 = src[i + 1];
		if (c0 == '"' || c0 == '\'')
		{
			end_of_string(&i, c0, src, n);
		}
		else if (c0 != '<' || c1 == '/');
		else
		{
			*str = src + i + 1;
			uint8_t res = XML_SUCCESS;
			int32_t attr_pos;
			for (++i; i < n; ++i)
			{
				const char c = src[i];
				if (!(c == ' ' || c == '>' || c== '/'|| c == '\t' || c == '\n' || c == '\r'));
				else if (!(c == '>' || c == '/'))
				{
					res |= XML_ATTRIBUTES;
					attr_pos = i + 1;
					*attributes_xml = (xml_t) { .src = src + i + 1 };
				}
				else
				{
					const int8_t nested_flag = c == '>';
					res |= nested_flag << 2;
					i += 1 - nested_flag;
					*attributes_xml = (xml_t) { 0 };
				}
				if (res & XML_ATTRIBUTES)
				{
					for (++i; i < n; ++i)
					{
						const char d = src[i];
						if (d == '"' || d == '\'')
						{
							end_of_string(&i, d, src, n);
						}
						else if (!(d == '>' || d == '/'));
						else
						{
							const int8_t nested_flag = d == '>';
							res |= nested_flag << 2;
							i += 1 - nested_flag;
							attributes_xml->len = i - attr_pos;
							break;
						}
					}
				}
				if (res & XML_HAS_NESTED)
				{
					++i;
					const int32_t nest_pos = i;
					*nested_xml = (xml_t){ .src = src + nest_pos };
					int32_t count = 0;
					for (; i < n; ++i)
					{
						const char d = src[i];
						if (d == '"' || d == '\'')
						{
							end_of_string(&i, d, src, n);
						}
						else if (!(d == '<' || d == '/'));
						else if (d == '<')
						{
							const char next_char = src[i + 1];
							count += (next_char != '/') - (next_char == '/');
							i += next_char == '/';
						}
						else if (d == '/')
						{
							--count;
						}
						if (count >= 0);
						else
						{
							nested_xml->len = i - nest_pos;
							break;
						}
					}
				}
				xml->pos = i;
				return res;
			}
		}
	}
	xml->pos = n - 1;
	return XML_BAD;
}
