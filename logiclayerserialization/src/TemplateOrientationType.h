/*
 * Generated by asn1c-0.9.21 (http://lionet.info/asn1c)
 * From ASN.1 module "ObjectSetModule"
 * 	found in "../protocol.asn1"
 * 	`asn1c -fskeletons-copy`
 */

#ifndef	_TemplateOrientationType_H_
#define	_TemplateOrientationType_H_


#include <asn_application.h>

/* Including external dependencies */
#include <ENUMERATED.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Dependencies */
typedef enum TemplateOrientationType {
	TemplateOrientationType_undefined	= 0,
	TemplateOrientationType_normal	= 1,
	TemplateOrientationType_flipped_up_down	= 2,
	TemplateOrientationType_flipped_left_right	= 3,
	TemplateOrientationType_flipped_both	= 4
} e_TemplateOrientationType;

/* TemplateOrientationType */
typedef ENUMERATED_t	 TemplateOrientationType_t;

/* Implementation */
extern asn_TYPE_descriptor_t asn_DEF_TemplateOrientationType;
asn_struct_free_f TemplateOrientationType_free;
asn_struct_print_f TemplateOrientationType_print;
asn_constr_check_f TemplateOrientationType_constraint;
ber_type_decoder_f TemplateOrientationType_decode_ber;
der_type_encoder_f TemplateOrientationType_encode_der;
xer_type_decoder_f TemplateOrientationType_decode_xer;
xer_type_encoder_f TemplateOrientationType_encode_xer;

#ifdef __cplusplus
}
#endif

#endif	/* _TemplateOrientationType_H_ */