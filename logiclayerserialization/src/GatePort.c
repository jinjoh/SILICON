/*
 * Generated by asn1c-0.9.21 (http://lionet.info/asn1c)
 * From ASN.1 module "ObjectSetModule"
 * 	found in "../protocol.asn1"
 */

#include <asn_internal.h>

#include "GatePort.h"

static asn_TYPE_member_t asn_MBR_connections_3[] = {
	{ ATF_POINTER, 0, 0,
		(ASN_TAG_CLASS_UNIVERSAL | (16 << 2)),
		0,
		&asn_DEF_LogicModelConnection,
		0,	/* Defer constraints checking to the member type */
		0,	/* PER is not compiled, use -gen-PER */
		0,
		""
		},
};
static ber_tlv_tag_t asn_DEF_connections_tags_3[] = {
	(ASN_TAG_CLASS_UNIVERSAL | (16 << 2))
};
static asn_SET_OF_specifics_t asn_SPC_connections_specs_3 = {
	sizeof(struct GatePort__connections),
	offsetof(struct GatePort__connections, _asn_ctx),
	0,	/* XER encoding is XMLDelimitedItemList */
};
static /* Use -fall-defs-global to expose */
asn_TYPE_descriptor_t asn_DEF_connections_3 = {
	"connections",
	"connections",
	SEQUENCE_OF_free,
	SEQUENCE_OF_print,
	SEQUENCE_OF_constraint,
	SEQUENCE_OF_decode_ber,
	SEQUENCE_OF_encode_der,
	SEQUENCE_OF_decode_xer,
	SEQUENCE_OF_encode_xer,
	0, 0,	/* No PER support, use "-gen-PER" to enable */
	0,	/* Use generic outmost tag fetcher */
	asn_DEF_connections_tags_3,
	sizeof(asn_DEF_connections_tags_3)
		/sizeof(asn_DEF_connections_tags_3[0]), /* 1 */
	asn_DEF_connections_tags_3,	/* Same as above */
	sizeof(asn_DEF_connections_tags_3)
		/sizeof(asn_DEF_connections_tags_3[0]), /* 1 */
	0,	/* No PER visible constraints */
	asn_MBR_connections_3,
	1,	/* Single element */
	&asn_SPC_connections_specs_3	/* Additional specs */
};

static asn_TYPE_member_t asn_MBR_GatePort_1[] = {
	{ ATF_NOFLAGS, 0, offsetof(struct GatePort, port_id),
		(ASN_TAG_CLASS_UNIVERSAL | (2 << 2)),
		0,
		&asn_DEF_INTEGER,
		0,	/* Defer constraints checking to the member type */
		0,	/* PER is not compiled, use -gen-PER */
		0,
		"port-id"
		},
	{ ATF_NOFLAGS, 0, offsetof(struct GatePort, connections),
		(ASN_TAG_CLASS_UNIVERSAL | (16 << 2)),
		0,
		&asn_DEF_connections_3,
		0,	/* Defer constraints checking to the member type */
		0,	/* PER is not compiled, use -gen-PER */
		0,
		"connections"
		},
};
static ber_tlv_tag_t asn_DEF_GatePort_tags_1[] = {
	(ASN_TAG_CLASS_UNIVERSAL | (16 << 2))
};
static asn_TYPE_tag2member_t asn_MAP_GatePort_tag2el_1[] = {
    { (ASN_TAG_CLASS_UNIVERSAL | (2 << 2)), 0, 0, 0 }, /* port-id at 72 */
    { (ASN_TAG_CLASS_UNIVERSAL | (16 << 2)), 1, 0, 0 } /* connections at 73 */
};
static asn_SEQUENCE_specifics_t asn_SPC_GatePort_specs_1 = {
	sizeof(struct GatePort),
	offsetof(struct GatePort, _asn_ctx),
	asn_MAP_GatePort_tag2el_1,
	2,	/* Count of tags in the map */
	0, 0, 0,	/* Optional elements (not needed) */
	-1,	/* Start extensions */
	-1	/* Stop extensions */
};
asn_TYPE_descriptor_t asn_DEF_GatePort = {
	"GatePort",
	"GatePort",
	SEQUENCE_free,
	SEQUENCE_print,
	SEQUENCE_constraint,
	SEQUENCE_decode_ber,
	SEQUENCE_encode_der,
	SEQUENCE_decode_xer,
	SEQUENCE_encode_xer,
	0, 0,	/* No PER support, use "-gen-PER" to enable */
	0,	/* Use generic outmost tag fetcher */
	asn_DEF_GatePort_tags_1,
	sizeof(asn_DEF_GatePort_tags_1)
		/sizeof(asn_DEF_GatePort_tags_1[0]), /* 1 */
	asn_DEF_GatePort_tags_1,	/* Same as above */
	sizeof(asn_DEF_GatePort_tags_1)
		/sizeof(asn_DEF_GatePort_tags_1[0]), /* 1 */
	0,	/* No PER visible constraints */
	asn_MBR_GatePort_1,
	2,	/* Elements count */
	&asn_SPC_GatePort_specs_1	/* Additional specs */
};

