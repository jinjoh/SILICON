/*
 * Generated by asn1c-0.9.21 (http://lionet.info/asn1c)
 * From ASN.1 module "ObjectSetModule"
 * 	found in "../protocol.asn1"
 */

#ifndef	_ExclusionArea_H_
#define	_ExclusionArea_H_


#include <asn_application.h>

/* Including external dependencies */
#include <INTEGER.h>
#include <VisibleString.h>
#include "Color.h"
#include <constr_SEQUENCE.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ExclusionArea */
typedef struct ExclusionArea {
	INTEGER_t	 min_x;
	INTEGER_t	 min_y;
	INTEGER_t	 max_x;
	INTEGER_t	 max_y;
	INTEGER_t	 id;
	INTEGER_t	 layer;
	VisibleString_t	 description;
	Color_t	 col;
	
	/* Context for parsing across buffer boundaries */
	asn_struct_ctx_t _asn_ctx;
} ExclusionArea_t;

/* Implementation */
extern asn_TYPE_descriptor_t asn_DEF_ExclusionArea;

#ifdef __cplusplus
}
#endif

#endif	/* _ExclusionArea_H_ */
