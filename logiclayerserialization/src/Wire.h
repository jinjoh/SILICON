/*
 * Generated by asn1c-0.9.21 (http://lionet.info/asn1c)
 * From ASN.1 module "ObjectSetModule"
 * 	found in "../protocol.asn1"
 * 	`asn1c -fskeletons-copy`
 */

#ifndef	_Wire_H_
#define	_Wire_H_


#include <asn_application.h>

/* Including external dependencies */
#include <INTEGER.h>
#include <VisibleString.h>
#include "Color.h"
#include <asn_SEQUENCE_OF.h>
#include <constr_SEQUENCE_OF.h>
#include <constr_SEQUENCE.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
struct LogicModelConnection;

/* Wire */
typedef struct Wire {
	INTEGER_t	 from_x;
	INTEGER_t	 from_y;
	INTEGER_t	 to_x;
	INTEGER_t	 to_y;
	INTEGER_t	 diameter;
	INTEGER_t	 id;
	INTEGER_t	 layer;
	VisibleString_t	 name;
	Color_t	 col1;
	Color_t	 col2;
	struct Wire__connections {
		A_SEQUENCE_OF(struct LogicModelConnection) list;
		
		/* Context for parsing across buffer boundaries */
		asn_struct_ctx_t _asn_ctx;
	} connections;
	
	/* Context for parsing across buffer boundaries */
	asn_struct_ctx_t _asn_ctx;
} Wire_t;

/* Implementation */
extern asn_TYPE_descriptor_t asn_DEF_Wire;

#ifdef __cplusplus
}
#endif

/* Referred external types */
#include "LogicModelConnection.h"

#endif	/* _Wire_H_ */
