/* SEQLABS_VTING --> */
#ifndef _SEQ_ASN1_H
#define _SEQ_ASN1_H


/*Memory defines*/
#define SEQ_ASN1_MALLOC(_x_) malloc(_x_)
#define SEQ_ASN1_FREE(_x_) free(_x_)
#define SEQ_ASN1_CALLOC(_x_, _s_) calloc(_x_,_s_)

#define SEQ_ASN1P(x)((uint8_t*)x)

//Type tags
#define SEQ_ASN1_EOC             0 //Explicit value
#define SEQ_ASN1_BOOLEAN         1 //
#define SEQ_ASN1_INTEGER         2 //
#define SEQ_ASN1_BITSTRING       3 //
#define SEQ_ASN1_OCTETSTRING     4 //
#define SEQ_ASN1_NULL            5 //
#define SEQ_ASN1_OBJECTID        6 //
#define SEQ_ASN1_OBJECTDESC      7
#define SEQ_ASN1_EXTERNAL        8
#define SEQ_ASN1_REAL            9
#define SEQ_ASN1_ENUMERATED      10
#define SEQ_ASN1_EMBEDDEDPDV     11
#define SEQ_ASN1_UTF8STRING      12
#define SEQ_ASN1_RELATIVEOID     13
#define SEQ_ASN1_RESERVED_0      14
#define SEQ_ASN1_RESERVED_1      15
#define SEQ_ASN1_SEQUENCE        16 //
#define SEQ_ASN1_SEQUENCEOF      16 //
#define SEQ_ASN1_SET             17
#define SEQ_ASN1_SETOF           17
#define SEQ_ASN1_NUMERICSTRING   18
#define SEQ_ASN1_PRINTABLESTRING 19
#define SEQ_ASN1_T61STRING       20
#define SEQ_ASN1_VIDEOTEXSTRING  21
#define SEQ_ASN1_IA5STRING       22
#define SEQ_ASN1_UTCTIME         23
#define SEQ_ASN1_GENERALIZEDTIME 24
#define SEQ_ASN1_GRAPHICSTRIN    25
#define SEQ_ASN1_VISIBLESTRING   26
#define SEQ_ASN1_GENERALSTRING   27
#define SEQ_ASN1_UNIVERSALSTRING 28
#define SEQ_ASN1_CHARACTERSTRING 29
#define SEQ_ASN1_BMPSTRING       30

#define SEQ_ASN1_CLS_UNIVERSAL   0x00
#define SEQ_ASN1_CLS_APPLICATION 0x01
#define SEQ_ASN1_CLS_CONTEXT     0x02
#define SEQ_ASN1_CLS_PRIVATE     0x03

#define SEQ_ASN1_CTYPE_PRIMITIVE   0x00
#define SEQ_ASN1_CTYPE_CONSTRUCTED 0x01

#define SEQ_ASN1_CONTEXT_EXPLICIT(_ctexplt_) _ctexplt_

//Error return values
#define SEQ_AP_OK 0
#define SEQ_AP_ERROR 1
#define SEQ_AP_ERROR_LENGTH_OVERFLOW 2
#define SEQ_AP_ERROR_LENGTH_UNKNOWN 3

//Mode defines for seq_asn1_free_tree
typedef enum {
 SEQ_AP_FREENODEONLY,
 SEQ_AP_FREECONTENT
} SeqFreeTreeMode;


typedef struct dernode SeqDerNode;
struct dernode {
	uint8_t cls;	     /*SEQ_ASN1_CLS_* */
	uint8_t composition; /*SEQ_ASN1_CTYPE_* */
	uint8_t tag;	 /*Type tag*/
	size_t length; 	 /*length of content*/
	void *content; 	 /*value for node*/
 	void *raw;	    	/*Entire buffer with tag, length etc... in it*/
	size_t rawlength;	/*length of entire buffer*/
	SeqDerNode *next;
	SeqDerNode *children;
};


typedef void (*SeqDerIterator)(SeqDerNode *node, void *additional);


// DER encoding
SeqDerNode* seq_asn1_new_node(uint8_t type);
void seq_asn1_add_sibling(SeqDerNode *sibling, SeqDerNode *newsibling);
void seq_asn1_add_child(SeqDerNode *parent, SeqDerNode *child);

size_t seq_asn1_get_size(SeqDerNode *root);
size_t seq_asn1_encode(uint8_t *buffer, SeqDerNode *root);


SeqDerNode *seq_asn1_copy_node( SeqDerNode *in, uint8_t follow_sibling );

// DER parsing
int seq_asn1_parse_der(SeqDerNode **node, uint8_t *buffer, size_t buffersize);
SeqDerNode *seq_asn1_parse_single_node( uint8_t *buffer, size_t buffersize );

int seq_asn1_get_sibling_count(SeqDerNode *node);
SeqDerNode *seq_asn1_get_sibling(SeqDerNode *node, int index);

int seq_asn1_get_child_count(SeqDerNode *node);
SeqDerNode* seq_asn1_get_child(SeqDerNode *node, int index);

// utilities
void seq_asn1_set_integer(SeqDerNode *node, int value);
int seq_asn1_get_integer(SeqDerNode *node);

//Checks the first byte for >= 0x80, adds '00' if necessary.
//Will always allocate a buffer for node and copy the value.
int seq_asn1_set_big_int(SeqDerNode *node, uint8_t *value, size_t length);
//Copies the number from node to value, removing the [00] if necessary.
//If length is not large enough to hold the number then the variable will hold the necessary length on -1 return
int seq_asn1_get_big_int(SeqDerNode *node, uint8_t *value, size_t *length);

void seq_asn1_walk_tree(SeqDerNode *node, SeqDerIterator iterator, void *additional);
void seq_asn1_free_tree(SeqDerNode *node, SeqFreeTreeMode mode);

#endif
/* SEQLABS_VTING <-- */
