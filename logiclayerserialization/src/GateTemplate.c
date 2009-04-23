/*
 * Generated by asn1c-0.9.21 (http://lionet.info/asn1c)
 * From ASN.1 module "ObjectSetModule"
 * 	found in "../protocol.asn1"
 * 	`asn1c -fskeletons-copy`
 */

#include <asn_internal.h>

#include "GateTemplate.h"

static asn_TYPE_member_t asn_MBR_ports_11[] = {
	{ ATF_POINTER, 0, 0,
		(ASN_TAG_CLASS_UNIVERSAL | (16 << 2)),
		0,
		&asn_DEF_GateTemplatePort,
		0,	/* Defer constraints checking to the member type */
		0,	/* PER is not compiled, use -gen-PER */
		0,
		""
		},
};
static ber_tlv_tag_t asn_DEF_ports_tags_11[] = {
	(ASN_TAG_CLASS_UNIVERSAL | (16 << 2))
};
static asn_SET_OF_specifics_t asn_SPC_ports_specs_11 = {
	sizeof(struct GateTemplate__ports),
	offsetof(struct GateTemplate__ports, _asn_ctx),
	0,	/* XER encoding is XMLDelimitedItemList */
};
static /* Use -fall-defs-global to expose */
asn_TYPE_descriptor_t asn_DEF_ports_11 = {
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
	asn_DEF_ports_tags_11,
	sizeof(asn_DEF_ports_tags_11)
		/sizeof(asn_DEF_ports_tags_11[0]), /* 1 */
	asn_DEF_ports_tags_11,	/* Same as above */
	sizeof(asn_DEF_ports_tags_11)
		/sizeof(asn_DEF_ports_tags_11[0]), /* 1 */
	0,	/* No PER visible constraints */
	asn_MBR_ports_11,
	1,	/* Single element */
	&asn_SPC_ports_specs_11	/* Additional specs */
};

static asn_TYPE_member_t asn_MBR_GateTemplate_1[] = {
	{ ATF_NOFLAGS, 0, offsetof(struct GateTemplate, gate_id),
		(ASN_TAG_CLASS_UNIVERSAL | (2 << 2)),
		0,
		&asn_DEF_INTEGER,
		0,	/* Defer constraints checking to the member type */
		0,	/* PER is not compiled, use -gen-PER */
		0,
		"gate-id"
		},
	{ ATF_NOFLAGS, 0, offsetof(struct GateTemplate, master_image_min_x),
		(ASN_TAG_CLASS_UNIVERSAL | (2 << 2)),
		0,
		&asn_DEF_INTEGER,
		0,	/* Defer constraints checking to the member type */
		0,	/* PER is not compiled, use -gen-PER */
		0,
		"master-image-min-x"
		},
	{ ATF_NOFLAGS, 0, offsetof(struct GateTemplate, master_image_min_y),
		(ASN_TAG_CLASS_UNIVERSAL | (2 << 2)),
		0,
		&asn_DEF_INTEGER,
		0,	/* Defer constraints checking to the member type */
		0,	/* PER is not compiled, use -gen-PER */
		0,
		"master-image-min-y"
		},
	{ ATF_NOFLAGS, 0, offsetof(struct GateTemplate, master_image_max_x),
		(ASN_TAG_CLASS_UNIVERSAL | (2 << 2)),
		0,
		&asn_DEF_INTEGER,
		0,	/* Defer constraints checking to the member type */
		0,	/* PER is not compiled, use -gen-PER */
		0,
		"master-image-max-x"
		},
	{ ATF_NOFLAGS, 0, offsetof(struct GateTemplate, master_image_max_y),
		(ASN_TAG_CLASS_UNIVERSAL | (2 << 2)),
		0,
		&asn_DEF_INTEGER,
		0,	/* Defer constraints checking to the member type */
		0,	/* PER is not compiled, use -gen-PER */
		0,
		"master-image-max-y"
		},
	{ ATF_NOFLAGS, 0, offsetof(struct GateTemplate, short_name),
		(ASN_TAG_CLASS_UNIVERSAL | (26 << 2)),
		0,
		&asn_DEF_VisibleString,
		0,	/* Defer constraints checking to the member type */
		0,	/* PER is not compiled, use -gen-PER */
		0,
		"short-name"
		},
	{ ATF_NOFLAGS, 0, offsetof(struct GateTemplate, description),
		(ASN_TAG_CLASS_UNIVERSAL | (26 << 2)),
		0,
		&asn_DEF_VisibleString,
		0,	/* Defer constraints checking to the member type */
		0,	/* PER is not compiled, use -gen-PER */
		0,
		"description"
		},
	{ ATF_NOFLAGS, 0, offsetof(struct GateTemplate, fill_col),
		(ASN_TAG_CLASS_UNIVERSAL | (16 << 2)),
		0,
		&asn_DEF_Color,
		0,	/* Defer constraints checking to the member type */
		0,	/* PER is not compiled, use -gen-PER */
		0,
		"fill-col"
		},
	{ ATF_NOFLAGS, 0, offsetof(struct GateTemplate, frame_col),
		(ASN_TAG_CLASS_UNIVERSAL | (16 << 2)),
		0,
		&asn_DEF_Color,
		0,	/* Defer constraints checking to the member type */
		0,	/* PER is not compiled, use -gen-PER */
		0,
		"frame-col"
		},
	{ ATF_NOFLAGS, 0, offsetof(struct GateTemplate, ports),
		(ASN_TAG_CLASS_UNIVERSAL | (16 << 2)),
		0,
		&asn_DEF_ports_11,
		0,	/* Defer constraints checking to the member type */
		0,	/* PER is not compiled, use -gen-PER */
		0,
		"ports"
		},
};
static ber_tlv_tag_t asn_DEF_GateTemplate_tags_1[] = {
	(ASN_TAG_CLASS_UNIVERSAL | (16 << 2))
};
static asn_TYPE_tag2member_t asn_MAP_GateTemplate_tag2el_1[] = {
    { (ASN_TAG_CLASS_UNIVERSAL | (2 << 2)), 0, 0, 4 }, /* gate-id at 119 */
    { (ASN_TAG_CLASS_UNIVERSAL | (2 << 2)), 1, -1, 3 }, /* master-image-min-x at 120 */
    { (ASN_TAG_CLASS_UNIVERSAL | (2 << 2)), 2, -2, 2 }, /* master-image-min-y at 121 */
    { (ASN_TAG_CLASS_UNIVERSAL | (2 << 2)), 3, -3, 1 }, /* master-image-max-x at 122 */
    { (ASN_TAG_CLASS_UNIVERSAL | (2 << 2)), 4, -4, 0 }, /* master-image-max-y at 123 */
    { (ASN_TAG_CLASS_UNIVERSAL | (16 << 2)), 7, 0, 2 }, /* fill-col at 126 */
    { (ASN_TAG_CLASS_UNIVERSAL | (16 << 2)), 8, -1, 1 }, /* frame-col at 127 */
    { (ASN_TAG_CLASS_UNIVERSAL | (16 << 2)), 9, -2, 0 }, /* ports at 128 */
    { (ASN_TAG_CLASS_UNIVERSAL | (26 << 2)), 5, 0, 1 }, /* short-name at 124 */
    { (ASN_TAG_CLASS_UNIVERSAL | (26 << 2)), 6, -1, 0 } /* description at 125 */
};
static asn_SEQUENCE_specifics_t asn_SPC_GateTemplate_specs_1 = {
	sizeof(struct GateTemplate),
	offsetof(struct GateTemplate, _asn_ctx),
	asn_MAP_GateTemplate_tag2el_1,
	10,	/* Count of tags in the map */
	0, 0, 0,	/* Optional elements (not needed) */
	-1,	/* Start extensions */
	-1	/* Stop extensions */
};
asn_TYPE_descriptor_t asn_DEF_GateTemplate = {
	"GateTemplate",
	"GateTemplate",
	SEQUENCE_free,
	SEQUENCE_print,
	SEQUENCE_constraint,
	SEQUENCE_decode_ber,
	SEQUENCE_encode_der,
	SEQUENCE_decode_xer,
	SEQUENCE_encode_xer,
	0, 0,	/* No PER support, use "-gen-PER" to enable */
	0,	/* Use generic outmost tag fetcher */
	asn_DEF_GateTemplate_tags_1,
	sizeof(asn_DEF_GateTemplate_tags_1)
		/sizeof(asn_DEF_GateTemplate_tags_1[0]), /* 1 */
	asn_DEF_GateTemplate_tags_1,	/* Same as above */
	sizeof(asn_DEF_GateTemplate_tags_1)
		/sizeof(asn_DEF_GateTemplate_tags_1[0]), /* 1 */
	0,	/* No PER visible constraints */
	asn_MBR_GateTemplate_1,
	10,	/* Elements count */
	&asn_SPC_GateTemplate_specs_1	/* Additional specs */
};

