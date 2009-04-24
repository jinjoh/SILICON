/*
 * Generated by asn1c-0.9.21 (http://lionet.info/asn1c)
 * From ASN.1 module "ObjectSetModule"
 * 	found in "../protocol.asn1"
 * 	`asn1c -fskeletons-copy`
 */

#ifndef	_Via_H_
#define	_Via_H_


#include <asn_application.h>

/* Including external dependencies */
#include <INTEGER.h>
#include "ViaDirection.h"
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

/* Via */
typedef struct Via {
	INTEGER_t	 x;
	INTEGER_t	 y;
	INTEGER_t	 diameter;
	ViaDirection_t	 direction;
	INTEGER_t	 id;
	INTEGER_t	 layer;
	VisibleString_t	 name;
	Color_t	 col;
	struct Via__connections {
		A_SEQUENCE_OF(struct LogicModelConnection) list;
		
		/* Context for parsing across buffer boundaries */
		asn_struct_ctx_t _asn_ctx;
	} connections;
	
	/* Context for parsing across buffer boundaries */
	asn_struct_ctx_t _asn_ctx;
} Via_t;

/* Implementation */
extern asn_TYPE_descriptor_t asn_DEF_Via;

#ifdef __cplusplus
}
#endif

/* Referred external types */
#include "LogicModelConnection.h"

#endif	/* _Via_H_ */