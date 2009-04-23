/*
 * Generated by asn1c-0.9.21 (http://lionet.info/asn1c)
 * From ASN.1 module "ObjectSetModule"
 * 	found in "../protocol.asn1"
 * 	`asn1c -fskeletons-copy`
 */

#include <asn_internal.h>

#include "ExclusionArea.h"

static asn_TYPE_member_t asn_MBR_ExclusionArea_1[] = {
	{ ATF_NOFLAGS, 0, offsetof(struct ExclusionArea, min_x),
		(ASN_TAG_CLASS_UNIVERSAL | (2 << 2)),
		0,
		&asn_DEF_INTEGER,
		0,	/* Defer constraints checking to the member type */
		0,	/* PER is not compiled, use -gen-PER */
		0,
		"min-x"
		},
	{ ATF_NOFLAGS, 0, offsetof(struct ExclusionArea, min_y),
		(ASN_TAG_CLASS_UNIVERSAL | (2 << 2)),
		0,
		&asn_DEF_INTEGER,
		0,	/* Defer constraints checking to the member type */
		0,	/* PER is not compiled, use -gen-PER */
		0,
		"min-y"
		},
	{ ATF_NOFLAGS, 0, offsetof(struct ExclusionArea, max_x),
		(ASN_TAG_CLASS_UNIVERSAL | (2 << 2)),
		0,
		&asn_DEF_INTEGER,
		0,	/* Defer constraints checking to the member type */
		0,	/* PER is not compiled, use -gen-PER */
		0,
		"max-x"
		},
	{ ATF_NOFLAGS, 0, offsetof(struct ExclusionArea, max_y),
		(ASN_TAG_CLASS_UNIVERSAL | (2 << 2)),
		0,
		&asn_DEF_INTEGER,
		0,	/* Defer constraints checking to the member type */
		0,	/* PER is not compiled, use -gen-PER */
		0,
		"max-y"
		},
	{ ATF_NOFLAGS, 0, offsetof(struct ExclusionArea, id),
		(ASN_TAG_CLASS_UNIVERSAL | (2 << 2)),
		0,
		&asn_DEF_INTEGER,
		0,	/* Defer constraints checking to the member type */
		0,	/* PER is not compiled, use -gen-PER */
		0,
		"id"
		},
	{ ATF_NOFLAGS, 0, offsetof(struct ExclusionArea, layer),
		(ASN_TAG_CLASS_UNIVERSAL | (2 << 2)),
		0,
		&asn_DEF_INTEGER,
		0,	/* Defer constraints checking to the member type */
		0,	/* PER is not compiled, use -gen-PER */
		0,
		"layer"
		},
	{ ATF_NOFLAGS, 0, offsetof(struct ExclusionArea, description),
		(ASN_TAG_CLASS_UNIVERSAL | (26 << 2)),
		0,
		&asn_DEF_VisibleString,
		0,	/* Defer constraints checking to the member type */
		0,	/* PER is not compiled, use -gen-PER */
		0,
		"description"
		},
	{ ATF_NOFLAGS, 0, offsetof(struct ExclusionArea, col),
		(ASN_TAG_CLASS_UNIVERSAL | (16 << 2)),
		0,
		&asn_DEF_Color,
		0,	/* Defer constraints checking to the member type */
		0,	/* PER is not compiled, use -gen-PER */
		0,
		"col"
		},
};
static ber_tlv_tag_t asn_DEF_ExclusionArea_tags_1[] = {
	(ASN_TAG_CLASS_UNIVERSAL | (16 << 2))
};
static asn_TYPE_tag2member_t asn_MAP_ExclusionArea_tag2el_1[] = {
    { (ASN_TAG_CLASS_UNIVERSAL | (2 << 2)), 0, 0, 5 }, /* min-x at 25 */
    { (ASN_TAG_CLASS_UNIVERSAL | (2 << 2)), 1, -1, 4 }, /* min-y at 26 */
    { (ASN_TAG_CLASS_UNIVERSAL | (2 << 2)), 2, -2, 3 }, /* max-x at 27 */
    { (ASN_TAG_CLASS_UNIVERSAL | (2 << 2)), 3, -3, 2 }, /* max-y at 28 */
    { (ASN_TAG_CLASS_UNIVERSAL | (2 << 2)), 4, -4, 1 }, /* id at 29 */
    { (ASN_TAG_CLASS_UNIVERSAL | (2 << 2)), 5, -5, 0 }, /* layer at 30 */
    { (ASN_TAG_CLASS_UNIVERSAL | (16 << 2)), 7, 0, 0 }, /* col at 33 */
    { (ASN_TAG_CLASS_UNIVERSAL | (26 << 2)), 6, 0, 0 } /* description at 31 */
};
static asn_SEQUENCE_specifics_t asn_SPC_ExclusionArea_specs_1 = {
	sizeof(struct ExclusionArea),
	offsetof(struct ExclusionArea, _asn_ctx),
	asn_MAP_ExclusionArea_tag2el_1,
	8,	/* Count of tags in the map */
	0, 0, 0,	/* Optional elements (not needed) */
	-1,	/* Start extensions */
	-1	/* Stop extensions */
};
asn_TYPE_descriptor_t asn_DEF_ExclusionArea = {
	"ExclusionArea",
	"ExclusionArea",
	SEQUENCE_free,
	SEQUENCE_print,
	SEQUENCE_constraint,
	SEQUENCE_decode_ber,
	SEQUENCE_encode_der,
	SEQUENCE_decode_xer,
	SEQUENCE_encode_xer,
	0, 0,	/* No PER support, use "-gen-PER" to enable */
	0,	/* Use generic outmost tag fetcher */
	asn_DEF_ExclusionArea_tags_1,
	sizeof(asn_DEF_ExclusionArea_tags_1)
		/sizeof(asn_DEF_ExclusionArea_tags_1[0]), /* 1 */
	asn_DEF_ExclusionArea_tags_1,	/* Same as above */
	sizeof(asn_DEF_ExclusionArea_tags_1)
		/sizeof(asn_DEF_ExclusionArea_tags_1[0]), /* 1 */
	0,	/* No PER visible constraints */
	asn_MBR_ExclusionArea_1,
	8,	/* Elements count */
	&asn_SPC_ExclusionArea_specs_1	/* Additional specs */
};

