#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <malloc.h>

#include "seq_asn1.h"

//-----------------------------------------------
// private
static void asn1_flip(uint8_t *dst, uint8_t *src, size_t len)
{
	size_t index;
	for (index=0;index<len;index++) {
		dst[index]=src[len-1-index];
	}
}

#define END_LITTLE 0
#define END_BIG 1

//-----------------------------------------------
//-----------------------------------------------
// public
void seq_asn1_set_integer(SeqDerNode *node, unsigned int value)
{
	uint8_t buffer[sizeof(value)]={0};
	size_t len=sizeof(buffer);

	// little endian

	asn1_flip(buffer,(uint8_t*)&value,len);

	//New buffer allocated internally
	seq_asn1_set_big_int(node, buffer, len);
}

int seq_asn1_get_integer(SeqDerNode *node, unsigned int *value)
{
	int res=0;
	uint8_t buffer[sizeof(*value)]={0};
	size_t len = sizeof(buffer);

	if(!node || !value){
		return -1;
	}

	res = seq_asn1_get_big_int(node, buffer, &len);

	if(res == 0) { //Success
		// little endian
		*value=0;
		asn1_flip((uint8_t*)value, buffer, len);
	}

	return res;
}


/*
From the specification:
8.3 Encoding of an integer value
  8.3.1 The encoding of an integer value shall be primitive. The contents octets shall consist of one or more octets.
  8.3.2 If the contents octets of an integer value encoding consist of more than one octet, then the bits of the first octet and bit 8 of the second octet:
    a) shall not all be ones; and
    b) shall not all be zero.
    NOTE – These rules ensure that an integer value is always encoded in the smallest possible number of octets.
*/
//Checks the first byte for >= 0x80, adds '00' if necessary.
//Strips unnecessary 00 bytes from the front.
//Will always allocate a buffer for node and copy the value.
int seq_asn1_set_big_int(SeqDerNode *node, uint8_t *value, size_t length)
{
	int res=0;
	int offset=0;
	if(!node || !value){
		return -1;
	}

	//Shall not be all zero
	while(!value[0] && (length > 1) && !(value[1] & 0x80)){
		value++;
		length--;
	}

	//Ensure not all 1s
	if(value[0] >= 0x80){
		offset++;
	}

	node->tag=SEQ_ASN1_INTEGER;
	node->length=length+offset;

	//allocate at set to '0'
	node->content = SEQ_ASN1_CALLOC(node->length, sizeof(uint8_t));
	if(node->content) {
		uint8_t *buf = (uint8_t*)node->content;
		memcpy(buf+offset, value, length);
		node->content_copied = 1;
	} else {
		res=-1;
	}
	return res;
}

//Copies the number from node to value, removing the [00] if necessary.
//If length is not large enough to hold the number then the variable will hold the necessary length on -1 return
int seq_asn1_get_big_int(SeqDerNode *node, uint8_t *value, size_t *length)
{
	int res=0;
	size_t vlen=0;
	int index=0;
	uint8_t *bigint = NULL;
	if(!node || !value || !length) {
		return -2;
	}

	bigint = (uint8_t*)node->content;
	vlen = node->length;

	while((bigint[index] == 0x00) && (vlen > 1)) {
		index++;
		vlen--;
	}

	if(*length < vlen) {
		*length=vlen;
		return -1;
	}

	*length = vlen; //How much is actually copied.
	memcpy(value, bigint+(node->length-vlen), vlen);
	return res;
}


void seq_asn1_walk_tree(SeqDerNode *node, SeqDerIterator iterator, void *additional)
{
	if (node) {
		if (node->next) {
			seq_asn1_walk_tree(node->next,iterator,additional);
		}

		if (node->children) {
			seq_asn1_walk_tree(node->children,iterator,additional);
		}

		iterator(node,additional);
	}
}



static void free_node(SeqDerNode *node, void *additional)
{
	SeqFreeTreeMode* mode=(SeqFreeTreeMode*)additional;
	if ((*mode==SEQ_AP_FREECONTENT) || (node->content_copied != 0)){
		SEQ_ASN1_FREE(node->content);
	}

	SEQ_ASN1_FREE(node);
}

void seq_asn1_free_tree(SeqDerNode *node, SeqFreeTreeMode mode)
{
	seq_asn1_walk_tree(node,free_node,(void*)&mode);
}













