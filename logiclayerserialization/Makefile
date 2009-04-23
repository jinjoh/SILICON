ASN_MODULE_SOURCES=	\
	src/ExclusionArea.c \
	src/Annotation.c \
	src/Color.c \
	src/LogicModelConnection.c \
	src/Wire.c	\
	src/Gate.c	\
	src/ViaDirection.c	\
	src/Via.c	\
	src/GateTemplate.c	\
	src/PortType.c \
	src/GatePort.c \
	src/GateTemplatePort.c \
	src/TemplateOrientationType.c \
	src/Object.c	\
	src/FileContent.c 

ASN_MODULE_HEADERS=	\
	src/ExclusionArea.h \
	src/Annotation.h \
	src/Color.h \
	src/LogicModelConnection.h \
	src/Wire.h	\
	src/Gate.h	\
	src/ViaDirection.h	\
	src/Via.h	\
	src/GateTemplate.h	\
	src/PortType.h \
	src/GatePort.h \
	src/GateTemplatePort.h \
	src/TemplateOrientationType.h \
	src/Object.h	\
	src/FileContent.h

ASN_MODULE_HEADERS+=src/ENUMERATED.h
ASN_MODULE_SOURCES+=src/ENUMERATED.c
ASN_MODULE_HEADERS+=src/INTEGER.h
ASN_MODULE_HEADERS+=src/NativeEnumerated.h
ASN_MODULE_SOURCES+=src/INTEGER.c
ASN_MODULE_SOURCES+=src/NativeEnumerated.c
ASN_MODULE_HEADERS+=src/NativeInteger.h
ASN_MODULE_SOURCES+=src/NativeInteger.c
ASN_MODULE_HEADERS+=src/VisibleString.h
ASN_MODULE_SOURCES+=src/VisibleString.c
ASN_MODULE_HEADERS+=src/asn_SEQUENCE_OF.h
ASN_MODULE_SOURCES+=src/asn_SEQUENCE_OF.c
ASN_MODULE_HEADERS+=src/asn_SET_OF.h
ASN_MODULE_SOURCES+=src/asn_SET_OF.c
ASN_MODULE_HEADERS+=src/constr_CHOICE.h
ASN_MODULE_SOURCES+=src/constr_CHOICE.c
ASN_MODULE_HEADERS+=src/constr_SEQUENCE.h
ASN_MODULE_SOURCES+=src/constr_SEQUENCE.c
ASN_MODULE_HEADERS+=src/constr_SEQUENCE_OF.h
ASN_MODULE_SOURCES+=src/constr_SEQUENCE_OF.c
ASN_MODULE_HEADERS+=src/constr_SET_OF.h
ASN_MODULE_SOURCES+=src/constr_SET_OF.c
ASN_MODULE_HEADERS+=src/asn_application.h
ASN_MODULE_HEADERS+=src/asn_system.h
ASN_MODULE_HEADERS+=src/asn_codecs.h
ASN_MODULE_HEADERS+=src/asn_internal.h
ASN_MODULE_HEADERS+=src/OCTET_STRING.h
ASN_MODULE_SOURCES+=src/OCTET_STRING.c
ASN_MODULE_HEADERS+=src/BIT_STRING.h
ASN_MODULE_SOURCES+=src/BIT_STRING.c
ASN_MODULE_SOURCES+=src/asn_codecs_prim.c
ASN_MODULE_HEADERS+=src/asn_codecs_prim.h
ASN_MODULE_HEADERS+=src/ber_tlv_length.h
ASN_MODULE_SOURCES+=src/ber_tlv_length.c
ASN_MODULE_HEADERS+=src/ber_tlv_tag.h
ASN_MODULE_SOURCES+=src/ber_tlv_tag.c
ASN_MODULE_HEADERS+=src/ber_decoder.h
ASN_MODULE_SOURCES+=src/ber_decoder.c
ASN_MODULE_HEADERS+=src/der_encoder.h
ASN_MODULE_SOURCES+=src/der_encoder.c
ASN_MODULE_HEADERS+=src/constr_TYPE.h
ASN_MODULE_SOURCES+=src/constr_TYPE.c
ASN_MODULE_HEADERS+=src/constraints.h
ASN_MODULE_SOURCES+=src/constraints.c
ASN_MODULE_HEADERS+=src/xer_support.h
ASN_MODULE_SOURCES+=src/xer_support.c
ASN_MODULE_HEADERS+=src/xer_decoder.h
ASN_MODULE_SOURCES+=src/xer_decoder.c
ASN_MODULE_HEADERS+=src/xer_encoder.h
ASN_MODULE_SOURCES+=src/xer_encoder.c
ASN_MODULE_HEADERS+=src/per_support.h
ASN_MODULE_SOURCES+=src/per_support.c
ASN_MODULE_HEADERS+=src/per_decoder.h
ASN_MODULE_SOURCES+=src/per_decoder.c
ASN_MODULE_HEADERS+=src/per_encoder.h
ASN_MODULE_SOURCES+=src/per_encoder.c


TARGET = logiclayerserialization.a
CFLAGS += -I. -Isrc/
OBJS=${ASN_MODULE_SOURCES:.c=.o}


all: $(TARGET)

$(TARGET): ${OBJS}
	ar rs $(TARGET) ${OBJS}

.SUFFIXES:
.SUFFIXES: .c .o

.c.o:
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	rm -f $(TARGET)
	rm -f $(OBJS)

regen: regenerate-from-asn1-source

regenerate-from-asn1-source:
	asn1c -c++ ../protocol.asn1

