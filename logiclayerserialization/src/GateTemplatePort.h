/*
 * Generated by asn1c-0.9.21 (http://lionet.info/asn1c)
 * From ASN.1 module "ObjectSetModule"
 * 	found in "../protocol.asn1"
 * 	`asn1c -fskeletons-copy`
 */

#ifndef	_GateTemplatePort_H_
#define	_GateTemplatePort_H_


#include <asn_application.h>

/* Including external dependencies */
#include <INTEGER.h>
#include <VisibleString.h>
#include "PortType.h"
#include "Color.h"
#include <constr_SEQUENCE.h>

#ifdef __cplusplus
extern "C" {
#endif

/* GateTemplatePort */
typedef struct GateTemplatePort {
	INTEGER_t	 id;
	VisibleString_t	 port_name;
	PortType_t	 port_type;
	INTEGER_t	 relative_x_coord;
	INTEGER_t	 relative_y_coord;
	Color_t	 col;
	INTEGER_t	 diameter;
	
	/* Context for parsing across buffer boundaries */
	asn_struct_ctx_t _asn_ctx;
} GateTemplatePort_t;

/* Implementation */
extern asn_TYPE_descriptor_t asn_DEF_GateTemplatePort;

#ifdef __cplusplus
}
#endif

#endif	/* _GateTemplatePort_H_ */