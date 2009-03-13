/*
 * Generated by asn1c-0.9.21 (http://lionet.info/asn1c)
 * From ASN.1 module "ObjectSetModule"
 * 	found in "../protocol.asn1"
 */

#ifndef	_Object_H_
#define	_Object_H_


#include <asn_application.h>

/* Including external dependencies */
#include "Wire.h"
#include "Via.h"
#include "Gate.h"
#include "GateTemplate.h"
#include <constr_CHOICE.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Dependencies */
typedef enum Object_PR {
	Object_PR_NOTHING,	/* No components present */
	Object_PR_wire,
	Object_PR_via,
	Object_PR_gate,
	Object_PR_gate_template
} Object_PR;

/* Object */
typedef struct Object {
	Object_PR present;
	union Object_u {
		Wire_t	 wire;
		Via_t	 via;
		Gate_t	 gate;
		GateTemplate_t	 gate_template;
	} choice;
	
	/* Context for parsing across buffer boundaries */
	asn_struct_ctx_t _asn_ctx;
} Object_t;

/* Implementation */
extern asn_TYPE_descriptor_t asn_DEF_Object;

#ifdef __cplusplus
}
#endif

#endif	/* _Object_H_ */
