/*
 * OpenBTS provides an open source alternative to legacy telco protocols and 
 * traditionally complex, proprietary hardware systems.
 *
 * Copyright 2014 Range Networks, Inc.
 *
 * This software is distributed under the terms of the GNU Affero General 
 * Public License version 3. See the COPYING and NOTICE files in the main 
 * directory for licensing information.
 *
 * This use of this software may be subject to additional restrictions.
 * See the LEGAL file in the main directory for details.
 */

#include "AsnHelper.h"
//#include "ByteVector.h"	included from AsnHelper.h
////#include "SgsnBase.h"		// For the layer 3 logging facility.
#include <ctype.h>
#include <Configuration.h>
#include "URRC.h"
extern ConfigurationTable gConfig;	// Where is this thing?

// (pat) This variable controls asn debug statements.
// It is used in the ASN/makefile to over-ride the default ASN debugger.
extern "C" {
int rn_asn_debug = 1;
};

namespace ASN {

//#include "ENUMERATED.h"	included from AsnHelper.h
//#include "BIT_STRING.h"	included from AsnHelper.h
#include "Digit.h"
#include "asn_SEQUENCE_OF.h"
};

namespace UMTS {

// Return true on success, false on failure.
// Make the ByteVector large enough to hold the expected encoded message,
// and the ByteVector size will be shrink wrapped around the result before return.
// If the descr is specified, an error message is printed on failure before return.
/**
 * uperEncodeToBV函数的功能是将一个ASN.1类型的结构体编码为UPER格式的二进制数据，
 * 并将结果存储在一个ByteVector对象中。如果编码成功，函数返回true，否则返回false。
 * 如果在编码过程中出现错误，函数会打印一个错误消息。
 */
bool uperEncodeToBV(ASN::asn_TYPE_descriptor_t *td,void *sptr, ByteVector &result, const std::string descr)
{
	rn_asn_debug = gConfig.getNum("UMTS.Debug.ASN");
	ASN::asn_enc_rval_t rval = uper_encode_to_buffer(td,sptr,result.begin(),result.allocSize());

	if (rval.encoded < 0) {
		LOG(ALERT) << "ASN encoder failed encoding '"<<descr<<"' into buf bytesize="<<result.size();
		return false;
	}
	// (pat) rval.encoded is number of bits, despite documentation
	// at asn_enc_rval_t that claims it is in bytes.
	result.setSizeBits(rval.encoded);
	return true;
}

// Decode an Asn message and return whatever kind of message pops out.
/**
 * `uperDecodeFromByteV` 函数的作用是解码一个 ASN 消息，并返回解码后的消息。
 * 该函数接受两个参数：
 * 一个是 `asn_TYPE_descriptor_t` 类型的指针，表示要解码的 ASN 类型；
 * 另一个是 `ByteVector` 类型的引用，表示要解码的字节流。
 * 函数返回一个 `void` 指针，指向解码后的消息。如果解码失败，则返回 `NULL`。
 */
void *uperDecodeFromByteV(ASN::asn_TYPE_descriptor_t *asnType,ByteVector &bv)
{
	void *result = NULL;
	ASN::asn_dec_rval_s rval = uper_decode_complete(NULL,  // optional stack size
		asnType, &result,
		bv.begin(), bv.size()	// per buffer size is in bytes.
		);
		//0,	// No skip bits
		//0);	// No unused bits. (There may be but we dont tell per.)

	if (rval.code != ASN::RC_OK) {
		// What else should we say about this?
		LOG(ERR) << "incoming message could not be decoded.";
		return 0;
	}
	if (rval.consumed != bv.size()) {
		LOG(INFO) << "incoming message consumed only" << rval.consumed << " of " << bv.size() << " bytes.";
	}
	return result;
}

// Same as uperDecodeFromByteV but work on a BitVector
/**
 * `uperDecodeFromBitV` 函数的作用是将一个 `BitVector` 类型的位向量转换为一个字节向量，
 * 并调用 `uperDecodeFromByteV` 函数对其进行解码。
 * 该函数接受两个参数：
 * 一个是 `asn_TYPE_descriptor_t` 类型的指针，表示要解码的 ASN 类型；
 * 另一个是 `BitVector` 类型的引用，表示要解码的位向量。
 * 函数返回一个 `void` 指针，指向解码后的消息。如果解码失败，则返回 `NULL`。
 */
void *uperDecodeFromBitV(ASN::asn_TYPE_descriptor_t *asnType,BitVector &in)
{
	ByteVector bv(in);
	return uperDecodeFromByteV(asnType,bv);
}

// Set the ASN BIT_STRING_t to an allocated buffer of the proper size.
// User must determine the proper size for the ASN message being used.
/**
 * `setAsnBIT_STRING` 函数的作用是设置一个ASN::BIT_STRING_t类型的变量，
 * 使其指向一个已分配好的缓冲区，并设置缓冲区的大小和未使用的位数。
 * 该函数接受三个参数：
 * 一个是ASN::BIT_STRING_t` 类型的指针，表示要设置的 `BIT_STRING` 变量；
 * 另一个是 `uint8_t` 类型的指针，表示已分配好的缓冲区；
 * 最后一个参数是一个整数，表示 `BIT_STRING` 的总位数。
 * 函数会根据总位数计算出缓冲区的大小，并设置未使用的位数。
 */
void setAsnBIT_STRING(ASN::BIT_STRING_t *result,uint8_t *buf, unsigned numBits)
{
	result->buf = buf;
	result->size = (numBits+7)/8;
	result->bits_unused = (numBits%8) ? (8-(numBits%8)) : 0;
}

/**
 * allocAsnBIT_STRING 函数的作用是分配一个ASN::BIT_STRING_t 类型的变量，
 * 并设置其指向一个已分配好的缓冲区，并设置缓冲区的大小和未使用的位数。
 * 该函数接受一个整数参数 `numBits`，表示 `BIT_STRING` 的总位数。
 * 函数会根据总位数计算出缓冲区的大小，并使用 `calloc` 函数分配缓冲区。
 * 然后，函数会调用 `setAsnBIT_STRING` 函数设置 `BIT_STRING` 变量的指针、缓冲区和未使用的位数，并返回该变量。
 */
ASN::BIT_STRING_t allocAsnBIT_STRING(unsigned numBits)
{
	ASN::BIT_STRING_t result;
	setAsnBIT_STRING(&result,(uint8_t*)calloc(1,(7+numBits)/8),numBits);
	return result;
}

/** Copy a string of ASCII digits into an ASN.1 SEQUENCE OF DIGIT. 
 * setASN1SeqOfDigits函数的作用是将一个字符串转换为 ASN.1 SEQUENCE OF DIGIT 类型的结构体。
 * 该函数接受两个参数，第一个参数是一个 `void` 指针，表示要设置的 ASN.1 SEQUENCE OF DIGIT 类型的结构体；
 * 第二个参数是一个 `const char*` 类型的指针，表示要转换的字符串。
 * 函数会将字符串中的每个数字转换为一个ASN::Digit_t类型的结构体，
 * 并将这些结构体添加到 ASN.1 SEQUENCE OF DIGIT 类型的结构体中。
*/
void setASN1SeqOfDigits(void *seq, const char* digit)
{
	ASN::asn_sequence_empty(seq);
	while (*digit != '\0') {
		ASN::Digit_t* d = (ASN::Digit_t*)calloc(1,sizeof(ASN::Digit_t));
		assert(d);
		*d = *digit++ - '0';
		int ret = ASN::ASN_SEQUENCE_ADD(seq,d);
		assert(ret==0);
	}
}

// Convert an integral value to an ASN ENUMERATED struct.
// Note: if you use this in an assignment, it will not free the previous value, if any.
// Example:
//		ENUMERATED_t someAsnEnumeratedValue;			// In asn somewhere.
//		someAsnEnumeratedValue = toAsnEnumerated(3);	// In our code.
/**
  函数的功能是将一个无符号整数转换为 ASN ENUMERATED 结构体
  它使用 asn_long2INTEGER 函数将整数值转换为 ENUMERATED_t 结构体，并返回结果。
  注意，如果在赋值时使用此函数，则不会释放先前的值（如果有）。
 */
ASN::ENUMERATED_t toAsnEnumerated(unsigned value)
{
	ASN::ENUMERATED_t result;
	memset(&result,0,sizeof(result));
	asn_long2INTEGER(&result,value);
	return result;
}

/**
 * 该函数的功能是将 ASN ENUMERATED 结构体转换为 long 类型。
 * 它使用 asn_INTEGER2long 函数将 ENUMERATED_t 结构体转换为 long 类型，并返回结果。
 * 如果转换失败，将记录错误日志并返回未定义的结果。
 */
long asnEnum2long(ASN::ENUMERATED_t &thing)
{
	long result;
	if (asn_INTEGER2long(&thing,&result)) {
		// We should not get this; it indicates a drastic encoding error in the UE.
		// If we do get it, will have to add arguments to figure out where it happened.
		LOG(ERR) << "failure converting asn ENUMERATED type to long";
	}
	return result;
}


// The argument must be A_SEQUENCE_OF(Digit_t) or equivalent.
// Digit_t is typedefed to long.
// 这行代码定义了一个名为 asn_sequence_of_long 的类型别名，
// 它是一个 A_SEQUENCE_OF(long) 序列，即一个 long 类型的序列。
// 这个类型别名通常用于 ASN.1 编码和解码中，用于表示一个由 long 类型组成的序列。
typedef A_SEQUENCE_OF(long) asn_sequence_of_long;

/**
 * 该函数的功能是将 ASN.1 序列 A_SEQUENCE_OF(Digit_t) 转换为 ByteVector 类型。
 * 它首先将 void 指针参数转换为 asn_sequence_of_long 类型，然后遍历该序列并将其转换为 ByteVector 类型。
 * 在遍历序列时，它将 Digit_t 类型（在此处为 long）转换为 uint8_t 类型，并将其存储在 ByteVector 中。
 */
AsnSeqOfDigit2BV::AsnSeqOfDigit2BV(void*arg)
		: ByteVector(((asn_sequence_of_long*)arg)->count)	// 继承自 ByteVector, 调用有参构造函数初始化基类部分
{
	asn_sequence_of_long *list = (asn_sequence_of_long*)arg;
	int cnt = size();
	uint8_t *bytes = begin();
	for (int i = 0; i < cnt; i++) {
		// The Digit_t is typedefed to long, and the longs are allocated.
		// (It couldnt be any more wasteful; pretty amusing that this is to compress the structures.)
		bytes[i] = (uint8_t) *(list->array[i]);
	}
}

//=============== Functions for AsnEnumMap ====================================

// Call back for ASN print function.
// 该函数是一个 ASN.1 解码回调函数，用于解析 ENUMERATED 类型的值并将其转换为 long 类型。
// 它将传递给 ASN.1 解码器的缓冲区转换为字符串，并从字符串中提取数字部分，
// 然后将其转换为 long 类型并存储在应用程序特定密钥中。
// 它与 GGSN 或 SGSN 没有直接关系，但是在解析 GTP 协议消息时可能会使用 ASN.1 编码和解码。
static int mycb(const void *buf, size_t size, void *application_specific_key)
{
	// Format of the value is "enumvalue (actualvalue)", eg: "5 (dat20)"
	const char *cp = (const char*)buf;
	cp = strchr(cp,'(');
	while (*cp != 0 && !isdigit(*cp)) { cp++; }
	long *presult = (long*) application_specific_key;
	*presult = atoi(cp);
	//printf("mycp buffer=%s %ld\n",(const char*)buf,*presult);
	return 0;
}


/**
 * 该函数的功能是将枚举类型的值加载到 AsnEnumMap 对象中。
 * 它使用传递的最大枚举值来计算枚举值的数量，并为存储枚举值的数组分配内存。
 * 然后，它使用 asn_long2INTEGER 函数将枚举值转换为 ENUMERATED_t 结构体，
 * 并使用 asn_TYPE_descriptor_t 的 print_struct 函数将其转换为字符串。
 * 在转换为字符串时，它使用回调函数 mycb 将枚举值存储在 mActualValues 数组中。
 */
void AsnEnumMap::asnLoadEnumeratedValues(ASN::asn_TYPE_descriptor_t &asnp, unsigned maxEnumValue)
{
	mNumValues = (1+maxEnumValue);
	mActualValues = (long*) malloc(mNumValues*sizeof(long));
	// Note: n is a long because that is what is expected.
	for (long n = 0; n <= (long)maxEnumValue; n++) {
		ASN::ENUMERATED_t foo;
		memset(&foo,0,sizeof(foo));
		ASN::asn_long2INTEGER(&foo, n);
		// In the original asn description, the actual values are just strings,
		// not the actual integral values that we need, but the actual
		// value is (almost) always included in the string.
		// The asn compiler does not provide any native way to get the enum strings out,
		// but we can use the print_struct which does a call-back to us with the info.
		asnp.print_struct(&asnp,(const void*) &foo,0,mycb,(void*)&mActualValues[n]);
	}
}

/**
 * 该函数的功能是将 AsnEnumMap 对象中存储的枚举值和其对应的整数值打印到标准输出。
 * 它遍历 mActualValues 数组并将每个枚举值和其对应的整数值打印到控制台。
 * 如果枚举值的数量大于 8，则在每 8 个值之后打印一个换行符。
 */
void AsnEnumMap::dump()
{
	for (unsigned enumValue = 0; enumValue < mNumValues; enumValue++) {
		printf("%u=>%ld ",enumValue,mActualValues[enumValue]);
		if (enumValue && enumValue % 8 == 0) printf("\n");
	}
	printf("\n");
}

// Return the asn enum value for an actual value.
// This is slow, but only happens at setup.
// Return "close" value if none match.
/**
 * 该函数的功能是在 AsnEnumMap 对象中查找给定的实际值，并返回其对应的枚举值。
 * 它遍历 mActualValues 数组并比较每个值与给定的实际值是否相等。
 * 如果找到匹配的值，则返回其对应的枚举值。如果没有找到匹配的值，则返回最接近的枚举值
 */
int AsnEnumMap::findEnum(long actual)
{
	unsigned closeindex = 0;
	long closediff = 0x7fffffff;
	for (unsigned enumValue = 0; enumValue < mNumValues; enumValue++) {
		if (actual == mActualValues[enumValue]) return enumValue;
		int diff = mActualValues[enumValue] - actual;
		if (diff < 0) diff = - diff;
		if (diff < closediff) { closediff = diff; closeindex = enumValue; }
	}
	//std::cout << "warning: enum value " << actual << " does not match, using: " << mActualValues[closeindex] <<"\n";
	return closeindex;
}

// Call back for ASN print function.
/**
 * 该函数是一个 ASN.1 解码回调函数，用于解析 ENUMERATED 类型的值并将其转换为字符串。
 * 它将传递给 ASN.1 解码器的缓冲区转换为字符串，并从字符串中提取数字部分，
 * 然后将其存储在应用程序特定密钥中。
 */
static int mycb2(const void *buf, size_t size, void *application_specific_key)
{
	const char *cp = (const char*)buf;	// Gotta love c++.
	// Format of the value is "enumvalue (actualvalue)", eg: "5 (dat20)"
	std::ostringstream *ssp = (std::ostringstream*)application_specific_key;
	ssp->write(cp,size);
	return 0;
}

// Like asn_fprint but return the result in a C++ string.
/**
 * 该函数名为 `asn2string`，它的功能是将 ASN.1 编码的结构体转换为字符串。
 * 它使用 `asn_TYPE_descriptor_t` 结构体的 `print_struct` 函数将结构体转换为字符串，
 * 并使用回调函数 `mycb2` 将字符串存储在 `std::ostringstream` 对象中。
 * 最后，它将 `std::ostringstream` 对象转换为 `std::string` 对象并返回。
 */
std::string asn2string(ASN::asn_TYPE_descriptor_t *asnp, const void *struct_ptr)
{
	std::ostringstream ss;
	asnp->print_struct(asnp,struct_ptr,0,mycb2,(void*)&ss);
	return ss.str();
}

/**
 * 这个函数的功能是将ASN.1编码的消息转换为可读的字符串，并将其记录在日志中。
 * 如果调试标志被设置，它还会将消息记录到MGLOG和LOGWATCH中。
 * 这个函数与GGSN或SGSN没有直接关系，但是它可能被用于记录与这些设备通信相关的消息。
 */
void asnLogMsg(unsigned rbid, ASN::asn_TYPE_descriptor_t *asnp, const void *struct_ptr,
	const char *comment,
	UEInfo *uep,		// Or NULL if none.
	uint32_t urnti)		// If uep is NULL, put this in the log instead.
{
	int debug = gConfig.getNum("UMTS.Debug.Messages");
	if (debug || IS_LOG_LEVEL(INFO)) {
		// This C++ IO paradigm is so crappy.
		std::string readable = asn2string(asnp,struct_ptr);
		std::string id = uep ? uep->ueid() : format(" urnti=0x%x",urnti);
		_LOG(INFO) << (comment?comment:"") <<id<<LOGVAR(rbid) <<" "<< readable.c_str();
		if (debug && comment) {
			////MGLOG(comment <<id<<LOGVAR(rbid));
			LOGWATCH(comment <<id<<LOGVAR(rbid));
		}
	}
}

//void AsnBitString::finish(ASN::BIT_STRING_t *ptr)
//{
//	if (ptr->bits_unused) setSizeBits(ptr->size*8 - ptr->bits_unused);
//}
//
//AsnBitString::AsnBitString(ASN::BIT_STRING_t *ptr) :
//	ByteVector(ptr->buf,ptr->size)
//{
//	finish(ptr);
//}
//
//AsnBitString::AsnBitString(ASN::BIT_STRING_t &ref) :
//	ByteVector(ref.buf,ref.size)
//{
//	finish(ptr);
//}

};	// namespace UMTS
