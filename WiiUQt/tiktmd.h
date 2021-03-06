#ifndef TIKTMD_H
#define TIKTMD_H

#include "includes.h"

#define ES_SIG_RSA4096		0x10000
#define ES_SIG_RSA2048		0x10001
#define ES_SIG_ECDSA		0x10002

typedef quint32 sigtype;
typedef sigtype sig_header;
typedef sig_header signed_blob;

typedef quint8 sha1[20];
typedef quint8 aeskey[16];

typedef struct _sig_rsa2048 {
	sigtype type;
	quint8 sig[256];
	quint8 fill[60];
} __attribute__((packed)) sig_rsa2048;

typedef struct _sig_rsa4096 {
	sigtype type;
	quint8 sig[512];
	quint8 fill[60];
}  __attribute__((packed)) sig_rsa4096;

typedef struct _sig_ecdsa {
	sigtype type;
	quint8 sig[60];
	quint8 fill[64];
}  __attribute__((packed)) sig_ecdsa;

typedef char sig_issuer[0x40];

typedef struct _tiklimit {
	quint32 tag;
	quint32 value;
} __attribute__((packed)) tiklimit;

typedef struct _tik {
	sig_issuer issuer;
	quint8 fill[63]; //TODO: not really fill
	aeskey cipher_title_key;
	quint8 fill2;
	quint64 ticketid;
	quint32 devicetype;
	quint64 titleid;
	quint16 access_mask;
	quint8 reserved[0x3c];
	quint8 cidx_mask[0x40];
	quint16 padding;
	tiklimit limits[8];
} __attribute__((packed)) tik;

typedef struct _tmd_content {
	quint32 cid;
	quint16 index;
	quint16 type;
	quint64 size;
	sha1 hash;
}  __attribute__((packed)) tmd_content;

typedef struct _tmd {
	sig_issuer issuer;              //0x140
	quint8 version;                             //0x180
	quint8 ca_crl_version;              //0x181
	quint8 signer_crl_version;  //0x182
	quint8 fill2;                               //0x183
	quint64 sys_version;                //0x184
	quint64 title_id;                   //0x18c
	quint32 title_type;                 //0x194
	quint16 group_id;                   //0x198
	quint16 zero;                               //0x19a
	quint16 region;                             //0x19c
	quint8 ratings[16];                 //0x19e
	quint8 reserved[12];                //0x1ae
	quint8 ipc_mask[12];
	quint8 reserved2[18];
	quint32 access_rights;
	quint16 title_version;
	quint16 num_contents;
	quint16 boot_index;
	quint16 fill3;
	// content records follow
	// C99 flexible array
	tmd_content contents[];
} __attribute__((packed)) tmd;
/*
typedef struct _tmd_view_content
{
    quint32 cid;
    quint16 index;
    quint16 type;
    quint64 size;
} __attribute__((packed)) tmd_view_content;

typedef struct _cert_header {
	sig_issuer issuer;
	quint32 cert_type;
	char cert_name[64];
	quint32 cert_id; //???
} __attribute__((packed)) cert_header;

typedef struct _cert_rsa2048 {
	sig_issuer issuer;
	quint32 cert_type;
	char cert_name[64];
	quint32 cert_id;
	quint8 modulus[256];
	quint32 exponent;
	quint8 pad[0x34];
} __attribute__((packed)) cert_rsa2048;

typedef struct _cert_rsa4096 {
	sig_issuer issuer;
	quint32 cert_type;
	char cert_name[64];
	quint32 cert_id;
	quint8 modulus[512];
	quint32 exponent;
	quint8 pad[0x34];
} __attribute__((packed)) cert_rsa4096;

typedef struct _cert_ecdsa {
	sig_issuer issuer;
	quint32 cert_type;
	char cert_name[64];
	quint32 cert_id;		// ng key id
	quint8 r[30];
	quint8 s[30];
	quint8 pad[0x3c];
} __attribute__((packed)) cert_ecdsa;
*/
//just a quick class to try to keep the rest of the code from getting full of the same shit over and over
class Ticket
{
public:
	Ticket( const QByteArray &stuff = QByteArray(), bool fixKeyIndex = true );

    //expose the tik data to the rest of the code so it cas read directly from the p_tmd instead of having to add a function to access all the data
    //the pointer should be good until "data" is changed
    const tik *payload(){ return p_tik; }

    quint64 Tid();
    bool SetTid( quint64 tid );

    QByteArray DecryptedKey();
    quint32 SignedSize();
    bool FakeSign();

    //get the ticket data
    const QByteArray Data(){ return data; }

private:
    quint32 payLoadOffset;

    //let data hold the entire tik data
    QByteArray data;
    QByteArray decKey;

    //point the p_tik to the spot in "data" we want
    void SetPointer();

    //this is just a pointer to the actual good stuff in "data".
    //whenever data is changes, this pointer will become invalid and needs to be reset
    tik *p_tik;
};

class Tmd
{
public:
    Tmd( const QByteArray &stuff = QByteArray() );

    //expose the tmd data to the rest of the code so it cas read directly from the p_tmd instead of having to add a function to access all the data
    //the pointer should be good until "data" is changed
    const tmd *payload(){ return p_tmd; }

    //get a string containing the u32 of cid i
    QString Cid( quint16 i );

    //get a btyearray with the hosh of the given conten
    QByteArray Hash( quint16 i );

    //returned in host endian
	quint64 Size( quint16 i );
	quint16 Type( quint16 i );
	quint16 BootIndex();
	quint16 Index( quint16 i );
    quint64 Tid();
    quint64 IOS();
    quint16 Gid();
	quint32 AccessFlags();


    //gets the number of contents
    quint16 Count();

    //title version
    quint16 Version();

    //functions to edit the TMD
    //you probably want to call FakeSign() after using these
    bool SetTid( quint64 tid );
    bool SetVersion( quint16 v );
    bool SetType( quint16 i, quint16 type );
    bool SetSize( quint16 i, quint32 size );
    bool SetHash( quint16 i, const QByteArray &hash );
    bool SetIOS( quint64 ios );
	bool SetAhb( bool remove = true );
	bool SetDiskAccess( bool allow = true );

    bool FakeSign();

    //returns a qbytearray containing the tmd, from the rsa signature through the last content
    const QByteArray Data(){ return data; }

    //print some tmd info to qDebug()
    void Dbg();


    quint32 SignedSize();
private:
    quint32 payLoadOffset;

    //let data hold the entire tmd data
    QByteArray data;

    //point the p_tmd to the spot in "data" we want
    void SetPointer();

    //this is just a pointer to the actual good stuff in "data".
    //whenever data is changed, this pointer will become invalid and needs to be reset
    tmd *p_tmd;
};

enum
{
    TIKTMD_ERROR_SUCCESS = 0,
    TIKTMD_ERROR_SIG_TYPE,
    TIKTMD_ERROR_SUB_TYPE,
    TIKTMD_ERROR_RSA_FAKESIGNED,
    TIKTMD_ERROR_RSA_HASH,
    TIKTMD_ERROR_RSA_TYPE_UNKNOWN,
    TIKTMD_ERROR_RSA_TYPE_MISMATCH,
    TIKTMD_ERROR_CERT_NOT_FOUND,
    TIKTMD_ERROR_COUNT
};

//checks the signatures in a tmd/ticket
//returns 1 of the above enums
int check_cert_chain( const QByteArray &data );

#endif // TIKTMD_H
