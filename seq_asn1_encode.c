#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <malloc.h>

#include "seq_asn1.h"


//-----------------------------------------------
// private
static size_t get_length_size(size_t length)
{
	size_t res=0;
	if (length<128) {
		res=1;
	} else {
		int bytecount=1;
		while (length) {
			// this has to be adjusted for endianness!!
			length=length>>8;
			bytecount++;
		}

		res=bytecount;
	}
	return res;
}

static size_t get_node_length(SeqDerNode *root)
{
	size_t res=1; // start with identifier octet (always 1 byte)
	if(root->composition == SEQ_ASN1_CTYPE_CONSTRUCTED) {
		if(root->children) {
			size_t contentsize=seq_asn1_get_size(root->children);
			res+=get_length_size(contentsize);
			res+=contentsize;
		} else {
			res=0;
		}
	} else {
		res+=get_length_size(root->length);
		res+=root->length;
	}
	return res;
}

static size_t get_content_length(SeqDerNode *node)
{
	size_t res=0;
	if(node->composition == SEQ_ASN1_CTYPE_CONSTRUCTED) {
		res=seq_asn1_get_size(node->children);
	} else {
		res=node->length;
	}
	return res;
}

static size_t write_node_header(uint8_t* buffer, SeqDerNode *node)
{
	unsigned int index=0;
	size_t res=0;
	uint8_t idbyte=0;
	uint8_t *length;

	size_t contentlength=0, lengthsize=0;
	uint8_t lengthbyte=0;
	uint8_t *contentptr=NULL;

	// SEQ_ASN1_CLS_UNIVERSAL == 0;
	// idbyte=ASN1_CLS_UNIVERSAL<<6;
	idbyte = node->tag;
	idbyte |= (node->cls<<6);
	idbyte |= (node->composition<<5);

	memcpy(buffer,&idbyte,1);
	res=1;

	contentlength=get_content_length(node);
	lengthsize=get_length_size(contentlength);
	contentptr=((uint8_t*)&contentlength);

	if (contentlength>127){
		// decrement length size by one byte (this is the lengthbyte indicator)
		lengthsize-=1;
		length=SEQ_ASN1_MALLOC(lengthsize);
		if(!length){
			res=0;
			return res;
		}

		lengthbyte=128+lengthsize;
		// convert to BIG_ENDIAN
		for (index=0;index<lengthsize;index++){
			length[(lengthsize-1)-index]=*contentptr;
			contentptr++;
		}
		memcpy(buffer+res,&lengthbyte,1);
		res+=1;
		memcpy(buffer+res,length,lengthsize);
		res+=lengthsize;

		SEQ_ASN1_FREE(length);
	} else {
		lengthbyte=contentlength;
		memcpy(buffer+res,&lengthbyte,1);
		res+=1;
	}
	
	return res;
}

static size_t write_node_tree(uint8_t*, SeqDerNode*);

static size_t write_node_content(uint8_t *buffer, SeqDerNode *node)
{
	size_t res=0;
	if(node->composition == SEQ_ASN1_CTYPE_CONSTRUCTED) {
		res=write_node_tree(buffer,node->children);
	} else {
		memcpy(buffer,node->content,node->length);
		res = node->length;
	}
	return res;
}

static size_t write_node(uint8_t *buffer, SeqDerNode* node)
{
	size_t res=0;
	uint8_t *bufptr=buffer;

	res=write_node_header(bufptr, node);
	bufptr+=res;

	res+=write_node_content(bufptr, node);
	return res;
}

static size_t write_node_tree(uint8_t *buffer, SeqDerNode *node)
{
	size_t res=0;
	res=write_node(buffer,node);
	if (node->next) {
		res+=write_node_tree(buffer+res, node->next);
	}
	return res;
}
//-----------------------------------------------
//-----------------------------------------------
//-----------------------------------------------
// public
SeqDerNode *seq_asn1_new_node(uint8_t type)
{
	SeqDerNode *res=(SeqDerNode*)SEQ_ASN1_MALLOC(sizeof(SeqDerNode));
	if(res) {
		memset(res, 0, sizeof(SeqDerNode));
		res->tag=type;
		if(res->tag == SEQ_ASN1_SEQUENCE ||
			res->tag == SEQ_ASN1_SET){
			res->composition=SEQ_ASN1_CTYPE_CONSTRUCTED;
		}
	}
	return res;
}


SeqDerNode *seq_asn1_copy_node( SeqDerNode *in, uint8_t follow_sibling )
{
	SeqDerNode* ret = seq_asn1_new_node(in->tag);
	if(!ret)
		return NULL;

	ret->cls = in->cls;
	ret->composition = in->composition;
	ret->length = in->length;

	if(ret->length) {
		ret->content = SEQ_ASN1_MALLOC(ret->length);
		if(ret->content) {
			memcpy(ret->content, in->content, ret->length);
			ret->content_copied = 1;
		} else {
			SEQ_ASN1_FREE(ret);
			ret=NULL;
		}
	} else {
		ret->content = NULL;
	}

	if(ret && in->children) {
		ret->children = seq_asn1_copy_node(in->children, 1);
		if(!ret->children){ //Failed to allocate
			SEQ_ASN1_FREE(ret);
			ret = NULL;
		}
	}

	if(ret && in->next && follow_sibling) {
		ret->next = seq_asn1_copy_node(in->next, 1);
		if(!ret->next) {
			seq_asn1_free_tree(ret, SEQ_AP_FREECONTENT); //Free children
			ret = NULL;
		}
	}
	return ret;
}

void seq_asn1_add_sibling(SeqDerNode *sibling, SeqDerNode *newsibling)
{
	SeqDerNode *lastnode=sibling;
	while (lastnode->next) {
		lastnode=lastnode->next;
	}
	lastnode->next=newsibling;
}

void seq_asn1_add_child(SeqDerNode *parent, SeqDerNode *child)
{
	if (parent->children) {
		seq_asn1_add_sibling(parent->children,child);
	} else {
		parent->children=child;
	}
}

size_t seq_asn1_get_size(SeqDerNode *root)
{
	size_t res=0;
	res=get_node_length(root);

	if (root->next) {
		res=res+seq_asn1_get_size(root->next);
	}

	return res;
}

size_t seq_asn1_encode(uint8_t *buffer, SeqDerNode *root)
{
	size_t res=0;
	if (buffer) {
		size_t writeres=write_node_tree(buffer, root);
		res=writeres;
	} else {
		res=seq_asn1_get_size(root);
	}
	return res;
}
