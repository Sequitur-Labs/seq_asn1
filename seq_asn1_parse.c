#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <malloc.h>

#include "seq_asn1.h"

//-----------------------------------------------
// private
static SeqDerNode *new_node( void )
{
	SeqDerNode *res=(SeqDerNode*)SEQ_ASN1_MALLOC(sizeof(SeqDerNode));
	if(!res){
		return NULL;
	}
	memset(res, 0, sizeof(SeqDerNode));
	return res;
}

static int fill_content_length(size_t *length, size_t *lengthsize, uint8_t *buffer, size_t bufferindex)
{
	int res=SEQ_AP_OK;

	*length=0;
	*lengthsize=0;

	if (buffer[bufferindex]<128) {
		*length=(size_t)buffer[bufferindex];
		*lengthsize=1;
	} else {
		if (buffer[bufferindex]>0x80) {
			// get length byte length
			size_t numbytes=buffer[bufferindex]-128;
			size_t clen=0;
			size_t bufferptr;

			if (numbytes<=sizeof(size_t)) {
				// convert to native endianness
				for (bufferptr=1;bufferptr<=numbytes;bufferptr++) {
					clen=(clen<<8)+buffer[bufferindex+bufferptr];
				}

				*length=clen;
				*lengthsize=numbytes+1;
			} else {
				res=SEQ_AP_ERROR_LENGTH_OVERFLOW;
			}
		} else {
			res=SEQ_AP_ERROR_LENGTH_UNKNOWN;
		}
	}
	
	return res;
}

static int fill_node(SeqDerNode *node, size_t *nodesize, uint8_t *buffer, size_t bufferindex)
{
	int res=SEQ_AP_OK;
	size_t lengthsize;
	*nodesize=0;

	//Treat options differently
	node->cls=(buffer[bufferindex]&0xC0)>>6;
	node->composition=(buffer[bufferindex]&0x20)>>5;
	node->tag=buffer[bufferindex]&0x1F;

	*nodesize+=1;

	res=fill_content_length(&node->length, &lengthsize, buffer, bufferindex+*nodesize);
	if (res==SEQ_AP_OK) {
		*nodesize+=lengthsize;
		node->content=&(buffer[bufferindex+*nodesize]);
		*nodesize+=node->length;
	}

	node->raw=&buffer[bufferindex];
	node->rawlength=*nodesize;
	return res;
}

//-----------------------------------------------
// public
int seq_asn1_parse_der(SeqDerNode **destnode, uint8_t *buffer, size_t buffersize)
{
	int res=SEQ_AP_OK;
	size_t nodesize=0;
	size_t bufferptr=0;

	SeqDerNode* node=new_node();
	SeqDerNode* parent=node;

	if(!parent) {
		return -1;
	}

	while (bufferptr<buffersize && node) {

		res=fill_node(node,&nodesize,buffer,bufferptr);
		if (res) {
			break;
		}

		bufferptr+=nodesize;

		if (node->composition) {
			res = seq_asn1_parse_der(&node->children,node->content,node->length);
			if (res) {
				break;
			}
		}

		//Don't overflow buffer
		if (bufferptr<buffersize) {
			node->next=new_node();
			node=node->next;
		}
	}

	if (res) {
		seq_asn1_free_tree(parent, SEQ_AP_FREENODEONLY);
		parent=0;
	}

	*destnode=parent;
	return res;
}

SeqDerNode *seq_asn1_parse_single_node( uint8_t *buffer, size_t buffersize )
{
	int res=SEQ_AP_OK;
	size_t nodesize=0;
	size_t bufferptr=0;

	SeqDerNode *node=new_node();
	if(node) {
		res=fill_node(node,&nodesize,buffer,bufferptr);
		if (res !=SEQ_AP_OK) {
			free(node);
			node = NULL;
		}
	}

	return node;
}

int seq_asn1_get_sibling_count(SeqDerNode *node)
{
	int res=0;
	if (node) {
		SeqDerNode *ptr=node->next;
		res=1;
		while (ptr) {
			ptr=ptr->next;
			res++;
		}
	}
	return res;
}

SeqDerNode *seq_asn1_get_sibling(SeqDerNode *node, int index)
{
	SeqDerNode *res=0;
	if (node) {
		SeqDerNode *ptr=node;
		int derindex=0;

		while (ptr) {
			if (derindex==index) {
				res=ptr;
				break;
			}
			ptr=ptr->next;
			derindex++;
		}
	}
	return res;
}

int seq_asn1_get_child_count(SeqDerNode *node)
{
	return seq_asn1_get_sibling_count(node->children);
}

SeqDerNode *seq_asn1_get_child(SeqDerNode *node, int index)
{
	return seq_asn1_get_sibling(node->children,index);
}
