// jsobj.h

#pragma once

#include "../stdafx.h"
#include "pdfile.h"

#pragma pack(push)
#pragma pack(1)

/* BinData = binary data types. 
   EOO = end of object
*/
//enum JSType { EOO = 0, Number=1, String=2, Object=3, Array=4, BinData=5, Undefined=6, jstOID=7, Bool=8, Date=9 , jstNULL=10 };
enum JSType {  EOO = 0, Number=1, String=2, Object=3, Array=4, BinData=5,
			   Undefined=6, jstOID=7, Bool=8, Date=9, jstNULL=10, RegEx=11 };

/* subtypes of BinData. 
   bdtCustom and above are ones that the JS compiler understands, but are
   opaque to the database.
*/
enum BinDataType { Function=1, ByteArray=2, bdtCustom=128 };

/*	Object id's are optional for JSObjects.  
	When present they should be the first object member added.
*/
struct OID { 
	long long a;
	unsigned b;
	bool operator==(const OID& r) { return a==r.a&&b==r.b; }
};

/* marshalled js object format:

   <unsigned totalSize> {<byte JSType><cstring FieldName><Data>}* EOO
      totalSize includes itself.

   Data:
   	 Bool: <byte>
     EOO: nothing follows
     Undefined: nothing follows
     OID: an OID object
     Number: <double>
     String: <unsigned32 strsizewithnull><cstring>
	 Date:   <8bytes>
	 Regex:  <cstring regex><cstring options>
     Object: a nested object, leading with its entire size, which terminates with EOO.
     Array:  same as object
     BinData:
       <unsigned len>
       <byte subtype>
       <byte[len] data>
*/

/* db operation message format 

   unsigned opid;         // arbitary; will be echoed back
   byte operation;
   
   Update:
      int reserved;
      string collection;  // name of the collection (namespace)
      a series of JSObjects terminated with a null object (i.e., just EOO)
   Insert:
      int reserved;
      string collection;
      a series of JSObjects terminated with a null object (i.e., just EOO)
   Query: see query.h
*/

#pragma pack(pop)

/* <type><fieldName    ><value>
   -------- size() ------------
         -fieldNameSize-
                        value()
   type()
*/
class Element {
	friend class JSElemIter;
public:
	JSType type() {return (JSType) * data ; }
	bool eoo() {return type() == EOO; }
	int size();

	// raw data be careful;
	const char * value() { return (data + fieldNameSize + 1) ; }

	//Only valid for string, object, array:
	int valuestrsize() {
		return *((int *) value());
	}

	//for strings. also gives you start of the data for an embedded object
	const char * valuestr() { return value() + 4;}
	void * embeddedObject() {
		valuestr();
	}

	bool operator == (Element & r) {
		int sz = size();
		return sz == r.size() &&
				memcmp(data, r.data, sz) == 0;
	}

private:
	Element(const char *d) : data(d) {
		fieldNameSize = eoo() ? 0 : strlen(data + 1) + 1;
		totalSize = -1;
	}

	const char * data;
	int fieldNameSize;
	int totalSize;
};

class JSObj {
	friend class JSElementIter;
public:
	JSObj(const char *_data) : data(_data) {
		size = *((int*) data);
	}
	JSObj(Record *r) { 
		size = r->netLength();
		data = r->data;
	}

	const char *objdata() { return data + 4; } // skip the length field.
	int objsize() { return size - 4; }

	OID* getOID() {
		const char *p = objdata();
		if( *p != jstOID )
			return 0;
		return (OID *) ++p;
	}

	int size;
	const char *data;
};

class JSElemIter {
public:
	JSElemIter(JSObj & jso) {
		pos = jso.objdata();
		theend = jso.objdata() + jso.objsize();
	}

	bool more() {return pos < theend;}
	Element next() {
		Element e(pos);
		pos += e.size();
		return e;
	}

private:
	const char * pos;
	const char * theend;
};

class JSMatcher {
public:
	JSMatcher(JSObj & pattern);

	bool matches(JSObj & j);

private:
	JSObj & jsobj;
	vector<Element> toMatch;
	int n;
};
