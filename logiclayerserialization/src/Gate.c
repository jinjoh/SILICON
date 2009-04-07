/*
 * Generated by asn1c-0.9.21 (http://lionet.info/asn1c)
 * From ASN.1 module "ObjectSetModule"
 * 	found in "../protocol.asn1"
 */

#include <asn_internal.h>

#include "Gate.h"

static asn_TYPE_member_t asn_MBR_ports_10[] = {
	{ ATF_POINTER, 0, 0,
		(ASN_TAG_CLASS_UNIVERSAL | (16 << 2)),
		0,
		&asn_DEF_GatePort,
		0,	/* Defer constraints checking to the member type */
		0,	/* PER is not compiled, use -gen-PER */
		0,
		""
		},
};
static ber_tlv_tag_t asn_DEF_ports_tags_10[] = {
	(ASN_TAG_CLASS_UNIVERSAL | (16 << 2))
};
static asn_SET_OF_specifics_t asn_SPC_ports_specs_10 = {
	sizeof(struct Gate__ports),
	offsetof(struct Gate__ports, _asn_ctx),
	0,	/* XER encoding is XMLDelimitedItemList */
};
static /* Use -fall-defs-global to expose */
asn_TYPE_descriptor_t asn_DEF_ports_10 = {
	"ports",
	"ports",
	SEQUENCE_OF_free,
	SEQUENCE_OF_print,
	SEQUENCE_OF_constraint,
	SEQUENCE_OF_decode_ber,
	SEQUENCE_OF_encode_der,
	SEQUENCE_OF_decode_xer,
	SEQUENCE_OF_encode_xer,
	0, 0,	/* No PER support, use "-gen-PER" to enable */
	0,	/* Use generic outmost tag fetcher */
	asn_DEF_ports_tags_10,
	sizeof(asn_DEF_ports_tags_10)
		/sizeof(asn_DEF_ports_tags_10[0]), /* 1 */
	asn_DEF_ports_tags_10,	/* Same as above */
	sizeof(asn_DEF_ports_tags_10)
		/sizeof(asn_DEF_ports_tags_10[0]), /* 1 */
	0,	/* No PER visible constraints */
	asn_MBR_ports_10,
	1,	/* Single element */
	&asn_SPC_ports_specs_10	/* Additional specs */
};

static asn_TYPE_member_t asn_MBR_Gate_1[] = {
	{ ATF_NOFLAGS, 0, offsetof(struct Gate, min_x),
		(ASN_TAG_CLASS_UNIVERSAL | (2 << 2)),
		0,
		&asn_DEF_INTEGER,
		0,	/* Defer constraints checking to the member type */
		0,	/* PER is not compiled, use -gen-PER */
		0,
		"min-x"
		},
	{ ATF_NOFLAGS, 0, offsetof(struct Gate, min_y),
		(ASN_TAG_CLASS_UNIVERSAL | (2 << 2)),
		0,
		&asn_DEF_INTEGER,
		0,	/* Defer constraints checking to the member type */
		0,	/* PER is not compiled, use -gen-PER */
		0,
		"min-y"
		},
	{ ATF_NOFLAGS, 0, offsetof(struct Gate, max_x),
		(ASN_TAG_CLASS_UNIVERSAL | (2 << 2)),
		0,
		&asn_DEF_INTEGER,
		0,	/* Defer constraints checking to the member type */
		0,	/* PER is not compiled, use -gen-PER */
		0,
		"max-x"
		},
	{ ATF_NOFLAGS, 0, offsetof(struct Gate, max_y),
		(ASN_TAG_CLASS_UNIVERSAL | (2 << 2)),
		0,
		&asn_DEF_INTEGER,
		0,	/* Defer constraints checking to the member type */
		0,	/* PER is not compiled, use -gen-PER */
		0,
		"max-y"
		},
	{ ATF_NOFLAGS, 0, offsetof(struct Gate, gate_id),
		(ASN_TAG_CLASS_UNIVERSAL | (2 << 2)),
		0,
		&asn_DEF_INTEGER,
		0,	/* Defer constraints checking to the member type */
		0,	/* PER is not compiled, use -gen-PER */
		0,
		"gate-id"
		},
	{ ATF_NOFLAGS, 0, offsetof(struct Gate, id),
		(ASN_TAG_CLASS_UNIVERSAL | (2 << 2)),
		0,
		&asn_DEF_INTEGER,
		0,	/* Defer constraints checking to the member type */
		0,	/* PER is not compiled, use -gen-PER */
		0,
		"id"
		},
	{ ATF_NOFLAGS, 0, offsetof(struct Gate, layer),
		(ASN_TAG_CLASS_UNIVERSAL | (2 << 2)),
		0,
		&asn_DEF_INTEGER,
		0,	/* Defer constraints checking to the member type */
		0,	/* PER is not compiled, use -gen-PER */
		0,
		"layer"
		},
	{ ATF_NOFLAGS, 0, offsetof(struct Gate, name),
		(ASN_TAG_CLASS_UNIVERSAL | (26 << 2)),
		0,
		&asn_DEF_VisibleString,
		0,	/* Defer constraints checking to the member type */
		0,	/* PER is not compiled, use -gen-PER */
		0,
		"name"
		},
	{ ATF_NOFLAGS, 0, offsetof(struct Gate, ports),
		(ASN_TAG_CLASS_UNIVERSAL | (16 << 2)),
		0,
		&asn_DEF_ports_10,
		0,	/* Defer constraints checking to the member type */
		0,	/* PER is not compiled, use -gen-PER */
		0,
		"ports"
		},
	{ ATF_NOFLAGS, 0, offsetof(struct Gate, master_orientation),
		(ASN_TAG_CLASS_UNIVERSAL | (10 << 2)),
		0,
		&asn_DEF_TemplateOrientationType,
		0,	/* Defer constraints checking to the member type */
		0,	/* PER is not compiled, use -gen-PER */
		0,
		"master-orientation"
		},
};
static ber_tlv_tag_t asn_DEF_Gate_tags_1[] = {
	(ASN_TAG_CLASS_UNIVERSAL | (16 << 2))
};
static asn_TYPE_tag2member_t asn_MAP_Gate_tag2el_1[] = {
    { (ASN_TAG_CLASS_UNIVERSAL | (2 << 2)), 0, 0, 6 }, /* min-x at 61 */
    { (ASN_TAG_CLASS_UNIVERSAL | (2 << 2)), 1, -1, 5 }, /* min-y at 62 */
    { (ASN_TAG_CLASS_UNIVERSAL | (2 << 2)), 2, -2, 4 }, /* max-x at 63 */
    { (ASN_TAG_CLASS_UNIVERSAL | (2 << 2)), 3, -3, 3 }, /* max-y at 64 */
    { (ASN_TAG_CLASS_UNIVERSAL | (2 << 2)), 4, -4, 2 }, /* gate-id at 65 */
    { (ASN_TAG_CLASS_UNIVERSAL | (2 << 2)), 5, -5, 1 }, /* id at 66 */
    { (ASN_TAG_CLASS_UNIVERSAL | (2 << 2)), 6, -6, 0 }, /* layer at 67 */
    { (ASN_TAG_CLASS_UNIVERSAL | (10 << 2)), 9, 0, 0 }, /* master-orientation at 70 */
    { (ASN_TAG_CLASS_UNIVERSAL | (16 << 2)), 8, 0, 0 }, /* ports at 69 */
    { (ASN_TAG_CLASS_UNIVERSAL | (26 << 2)), 7, 0, 0 } /* name at 68 */
};
static asn_SEQUENCE_specifics_t asn_SPC_Gate_specs_1 = {
	sizeof(struct Gate),
	offsetof(struct Gate, _asn_ctx),
	asn_MAP_Gate_tag2el_1,
	10,	/* Count of tags in the map */
	0, 0, 0,	/* Optional elements (not needed) */
	-1,	/* Start extensions */
	-1	/* Stop extensions */
};
asn_TYPE_descriptor_t asn_DEF_Gate = {
	"Gate",
	"Gate",
	SEQUENCE_free,
	SEQUENCE_print,
	SEQUENCE_constraint,
	SEQUENCE_decode_ber,
	SEQUENCE_encode_der,
	SEQUENCE_decode_xer,
	SEQUENCE_encode_xer,
	0, 0,	/* No PER support, use "-gen-PER" to enable */
	0,	/* Use generic outmost tag fetcher */
	asn_DEF_Gate_tags_1,
	sizeof(asn_DEF_Gate_tags_1)
		/sizeof(asn_DEF_Gate_tags_1[0]), /* 1 */
	asn_DEF_Gate_tags_1,	/* Same as above */
	sizeof(asn_DEF_Gate_tags_1)
		/sizeof(asn_DEF_Gate_tags_1[0]), /* 1 */
	0,	/* No PER visible constraints */
	asn_MBR_Gate_1,
	10,	/* Elements count */
	&asn_SPC_Gate_specs_1	/* Additional specs */
};

