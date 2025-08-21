#ifndef XML_H
#define XML_H

// NOTE: I assume the xml is perfect (its generated)
// ya im out here zero-copying :^)

#include <stdint.h>

typedef struct xml_t
{
	const char* src;
	int32_t pos;
	int32_t len;
} xml_t;

typedef enum __attribute__((packed)) xml_ret_t
{
	XML_BAD			= 0x00,
	XML_SUCCESS		= 0x01,
	XML_ATTRIBUTES	= 0x02,
	XML_HAS_NESTED	= 0x04
}	xml_ret_t;

xml_ret_t xml_next_attribute(char** attribute, int32_t* a_len, char** value, int32_t* v_len, xml_t* xml);

uint8_t xml_next_elem(char** str, int32_t* len, xml_t* xml, xml_t* attributes_xml, xml_t* nested_xml);

#endif /* End XML_H */
