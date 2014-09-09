/*
 * @f util/ccn-lite-pktdump.c
 * @b CCN lite - dumps CCNB, CCN-TLV and NDN-TLV encoded packets
 *               as well as RPC data structures
 *
 * Copyright (C) 2014, Christian Tschudin, University of Basel
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * File history:
 * 2014-03-29 created (merging three old dump programs)
 *
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "base64.c"

#include <arpa/inet.h>

#define USE_SUITE_CCNB
#define USE_SUITE_CCNTLV
#define USE_SUITE_CCNNDN

#include "../pkt-ccnb-dec.c"
#include "../pkt-ccntlv-dec.c"
#include "../pkt-ndntlv-dec.c"
#include "../pkt-localrpc.h"

enum {
    SUITE_CCNB = 0,
    SUITE_CCNTLV,
    SUITE_NDNTLV,
    SUITE_LOCRPC
};

void
hexdump(int lev, unsigned char *base, unsigned char *cp, int len, int rawxml, FILE* out)
{
    int i, maxi;

    while (len > 0) {
        maxi = len > 8 ? 8 : len;

        if(!rawxml) {
            fprintf(out, "%04zx  ", cp - base);
        }

        for (i = 0; i < lev+1; i++)
            fprintf(out, "  ");
        for (i = 0; i < 8; i++)
            printf(i < maxi ? "%02x " : "   ", i < maxi ? cp[i] : 0);

        if(!rawxml) {
            for (i = 79 - 6 - 2*(lev+1) - 8*3 - 12; i > 0; i--)
                fprintf(out, " ");
            fprintf(out, "  |");
            for (i = 0; i < maxi; i++, cp++)
                fprintf(out, "%c", isprint(*cp) ? *cp : '.');
            fprintf(out, "|\n");
        } else {
            fprintf(out, "\n");
        }

        len -= maxi;
    }
}

void
base64dump(int lev, unsigned char *base, unsigned char *cp, int len, int rawxml, FILE* out) {
    int encodedLen = -1;
    int i;
    for(i = 0; i < lev + 1; i++) {
        fprintf(out, "  ");
    }
    fprintf(out, "%s\n", base64_encode(cp, len, &encodedLen));
    cp += len;
    len = 0;
}

// ----------------------------------------------------------------------

int
ccnb_deheadAndPrint(int lev, unsigned char *base, unsigned char **buf,
            int *len, int *num, int *typ, int rawxml, FILE* out)
{
    int i, val = 0;

    if (*len <= 0) {
        return -1;
    }

    if(!rawxml) {
        fprintf(out, "%04zx  ", *buf - base);
    }

    for (i = 0; i < lev; i++) {
        fprintf(out, "  ");
    }
    if (**buf == 0) {
        if(!rawxml) {
            fprintf(out, "00 ");
        }
        *num = *typ = 0;
        *buf += 1;
        *len -= 1;
        return 0;
    }
    for (i = 0; i < (int)sizeof(i) && i < *len; i++) {
        unsigned char c = (*buf)[i];
        if(!rawxml) {
            fprintf(out, "%02x ", c);
        }
        if ( c & 0x80 ) {
            *num = (val << 4) | ((c >> 3) & 0xf);
            *typ = c & 0x7;
            *buf += i+1;
            *len -= i+1;
            return 0;
        }
        val = (val << 7) | c;
    }
    if(!rawxml) {
        fprintf(out, "?decoding problem?\n");
    }
    return -1;
}

static int
ccnb_must_recurse(/* work in progress */)
{
    return 0;
}

static char*
ccnb_dtag2name(int num)
{
    char *n;

    switch (num) {
    case CCN_DTAG_ANY:       n = "any"; break;
    case CCN_DTAG_NAME:      n = "name"; break;
    case CCN_DTAG_COMPONENT:     n = "component"; break;
    case CCN_DTAG_CONTENT:   n = "content"; break;
    case CCN_DTAG_SIGNEDINFO:    n = "signedinfo"; break;
    case CCN_DTAG_INTEREST:  n = "interest"; break;
    case CCN_DTAG_KEY:       n = "key"; break;
    case CCN_DTAG_KEYLOCATOR:    n = "keylocator"; break;
    case CCN_DTAG_KEYNAME:   n = "keyname"; break;
    case CCN_DTAG_SIGNATURE:     n = "signature"; break;
    case CCN_DTAG_TIMESTAMP:     n = "timestamp"; break;
    case CCN_DTAG_TYPE:      n = "type"; break;
    case CCN_DTAG_NONCE:     n = "nonce"; break;
    case CCN_DTAG_SCOPE:     n = "scope"; break;
    case CCN_DTAG_WITNESS:   n = "witness"; break;
    case CCN_DTAG_SIGNATUREBITS: n = "signaturebits"; break;
    case CCN_DTAG_FRESHNESS:     n = "freshnessSeconds"; break;
    case CCN_DTAG_FINALBLOCKID:  n = "finalblockid"; break;
    case CCN_DTAG_PUBPUBKDIGEST: n = "publisherPubKeyDigest"; break;
    case CCN_DTAG_PUBCERTDIGEST: n = "publisherCertDigest"; break;
    case CCN_DTAG_CONTENTOBJ:    n = "contentobj"; break;
    case CCN_DTAG_ACTION:    n = "action"; break;
    case CCN_DTAG_FACEID:    n = "faceID"; break;
    case CCN_DTAG_IPPROTO:   n = "ipProto"; break;
    case CCN_DTAG_HOST:      n = "host"; break;
    case CCN_DTAG_PORT:      n = "port"; break;
    case CCN_DTAG_FWDINGFLAGS:   n = "forwardingFlags"; break;
    case CCN_DTAG_FACEINSTANCE:  n = "faceInstance"; break;
    case CCN_DTAG_FWDINGENTRY:   n = "forwardingEntry"; break;
    case CCN_DTAG_MINSUFFCOMP:   n = "minSuffixComponents"; break;
    case CCN_DTAG_MAXSUFFCOMP:   n = "maxSuffixComponents"; break;
    case CCN_DTAG_SEQNO:     n = "sequenceNumber"; break;
    case CCN_DTAG_FragA:     n = "FragA (Type)"; break;  // frag Type
    case CCN_DTAG_FragB:     n = "FragB (SeqNr)"; break; // frag SeqNr
    case CCN_DTAG_FragC:     n = "FragC (Flag)"; break;  // frag h2h Flag
    case CCN_DTAG_FragD:     n = "FragD"; break;
    case CCN_DTAG_FragP:     n = "FragP (Fragment)"; break; // frag
    case CCN_DTAG_CCNPDU:    n = "ccnProtocolDataUnit"; break;

/*
    case CCNL_DTAG_MACSRC:   n = "MACsrc"; break;
    case CCNL_DTAG_IP4SRC:   n = "IP4src"; break;
    case CCNL_DTAG_FRAG:     n = "fragmentation"; break;
    case CCNL_DTAG_FACEFLAGS:    n = "faceFlags"; break;
    case CCNL_DTAG_DEBUGREQUEST: n = "debugRequest"; break;
    case CCNL_DTAG_DEBUGACTION:  n = "debugAction"; break;

    case CCNL_DTAG_FRAG2012_OLOSS:   n = "fragmentOurLoss"; break;
    case CCNL_DTAG_FRAG2012_YSEQN:   n = "fragmentYourSeqNo"; break;
*/

    default:
    n = NULL;
    }
    return n;
}

static int
ccnb_parse_lev(int lev, unsigned char *base, unsigned char **buf,
           int *len, char *cur_tag, int rawxml, FILE* out)
{
    int num, typ = -1;
    char *next_tag;

    while (ccnb_deheadAndPrint(lev, base, buf, len, &num, &typ, rawxml, out) == 0) {
        switch (typ) {
        case CCN_TT_BLOB:
        case CCN_TT_UDATA:
            if(rawxml) {
                fprintf(out, "<data ", num, num > 1 ? "s" : "");
                fprintf(out, "size=\"%i\" dt=\"binary.base64\"", num);
            } else {
                fprintf(out, " -- <data (%d byte%s)", num, num > 1 ? "s" : "");
            }
            if (ccnb_must_recurse(/* work in progress */)) {
              if(!rawxml) {
                fprintf(out, ", recursive decoding>\n");
              }
            // ...
    //      *buf += num;
    //      *len -= num;
            }
            fprintf(out, ">\n");
            if(!rawxml) {
                hexdump(lev, base, *buf, num, rawxml, out);
            } else {
                base64dump(lev, base, *buf, num, rawxml, out);
            }
    /*
            for (i = 0; i < num; i++) {
                if ((i%8) == 0) {
                fprintf(out, "\n%04zx  ", *buf - base + i);
                for (j = 0; j < lev; j++) fprintf(out, "  ");
                }
                fprintf(out, " %02x", (*buf)[i]);
            }
            fprintf(out, "\n");
    */
            *buf += num;
            *len -= num;
            int i;
            for(i = 0; i < lev ; i++) {
                fprintf(out, "  ");
            }
            fprintf(out, "</data>\n");
            break;
        case CCN_TT_DTAG:
            next_tag = ccnb_dtag2name(num);
            if (next_tag) {
                if(!rawxml) {
                    fprintf(out, " -- <%s>\n", next_tag);
                } else {
                    fprintf(out, "<%s>\n", next_tag);
                }
            }
            else {
                if(!rawxml) {
                    fprintf(out, " -- <unknown tt=%d num=%d>\n", typ, num);
                } else {
                    fprintf(out, "<unknown>\n");
                }
            }
            if (ccnb_parse_lev(lev+1, base, buf, len, next_tag, rawxml, out) < 0) {
                return -1;
            }
            if (lev == 0) {
                // base = *buf;
                if (*len > 0) {
                    fprintf(out, "\n");
                }
            }
            break;
        case 0:
            if (num == 0 || cur_tag != NULL) { // end tag
                if(!rawxml) {
                    fprintf(out, " -- </%s>\n", cur_tag);
                } else {
                    fprintf(out, "</%s>\n", cur_tag );
                }
                return 0;
            }
        default:
            if(!rawxml) {
                fprintf(out, "-- tt=%d num=%d not implemented yet\n", typ, num);
            } else {
                fprintf(out, "=%d num=%d not implemented yet\n", typ, num);
            }
            break;
        }
    }
    if(cur_tag != NULL) {
        fprintf(out, "</%s>\n", cur_tag );
    }
    return 0;
}


void
ccnb_parse(unsigned char *data, int len, int rawxml, FILE* out)
{
    unsigned char *buf = data;

    ccnb_parse_lev(0, data, &buf, &len, NULL, rawxml, out);
    if(!rawxml) {
        fprintf(out, "%04zx  pkt.end\n", buf - data);
    }
}

// ----------------------------------------------------------------------

enum {
    CTX_GLOBAL = 1,
    CTX_TOPLEVEL,
    CTX_HOP,
    CTX_NAME,
    CTX_INTEREST,
    CTX_CONTENT,
    CTX_NAMEAUTH // ?
};

static char ccntlv_recurse[][3] = {
    {CTX_TOPLEVEL, CCNX_TLV_TL_Interest, CTX_INTEREST},
    {CTX_TOPLEVEL, CCNX_TLV_TL_Object, CTX_CONTENT},
    {CTX_INTEREST, CCNX_TLV_G_Name, CTX_NAME},
    {CTX_CONTENT, CCNX_TLV_G_Name, CTX_NAME},

    {CTX_CONTENT, CCNX_TLV_C_NameAuth, CTX_CONTENT},
    {CTX_CONTENT, CCNX_TLV_C_KeyLocator, CTX_CONTENT},
    {CTX_CONTENT, CCNX_TLV_C_SigBlock, CTX_CONTENT},

    {0,0,0}
};

static char
ccntlv_must_recurse(char ctx, char typ)
{
    int i;
    for (i = 0; ccntlv_recurse[i][0]; i++) {
        if (ccntlv_recurse[i][0] == ctx && ccntlv_recurse[i][1] == typ) {
            return ccntlv_recurse[i][2];
        }
    }
    return 0;
}


// ----------------------------------------------------------------------

static char*
ccnl_ccntlv_type2name(unsigned char ctx, unsigned int type)
{
    char *cn = "globalCtx", *tn = NULL;
    static char tmp[50];

//    fprintf(stderr, "t2n %d %d\n", ctx, type);

    if (type == CCNX_TLV_G_Name) {
        tn = "Name";
    } else if (type == CCNX_TLV_G_Pad) {
        tn = "Pad";
    } else {
        switch (ctx) {
        /*
            case CTX_GLOBAL:
                cn = "global";
                switch (type) {
                case CCNX_TLV_G_Name:       tn = "Name"; break;
                case CCNX_TLV_G_Pad:        tn = "Pad"; break;
                default: break;
                }
                break;
        */
            case CTX_TOPLEVEL:
                cn = "toplevelCtx";
                switch (type) {
                case CCNX_TLV_TL_Interest:  tn = "Interest"; break;
                case CCNX_TLV_TL_Object:    tn = "Object"; break;
                default: break;
                }
                break;
            case CTX_HOP:
                cn = "hopCtx";
                switch (type) {
                case CCNX_TLV_PH_Nonce: tn = "Nonce"; break;
                case CCNX_TLV_PH_Hoplimit:  tn = "Hoplimit"; break;
                case CCNX_TLV_PH_Fragment:  tn = "Fragment"; break;
                default: break;
                }
                break;
            case CTX_NAME:
                cn = "nameCtx";
                switch (type) {
                case CCNX_TLV_N_UTF8:   tn = "UTF8"; break;
                case CCNX_TLV_N_Binary: tn = "Binary"; break;
                case CCNX_TLV_N_NameNonce:  tn = "NameNonce"; break;
                case CCNX_TLV_N_NameKey:    tn = "NameKey"; break;
                case CCNX_TLV_N_Meta:   tn = "Meta"; break;
                case CCNX_TLV_N_ObjHash:    tn = "ObjHash"; break;
                case CCNX_TLV_N_PayloadHash: tn = "PayloadHash"; break;
                default: break;
                }
                break;
            case CTX_INTEREST:
                cn = "interestCtx";
                switch (type) {
                case CCNX_TLV_I_KeyID:  tn = "KeyID"; break;
                case CCNX_TLV_I_ObjHash:    tn = "ObjHash"; break;
                case CCNX_TLV_I_Scope:  tn = "Scope"; break;
                case CCNX_TLV_I_Art:    tn = "Art"; break;
                case CCNX_TLV_I_IntLife:    tn = "IntLife"; break;
                default: break;
                }
                break;
            case CTX_CONTENT:
                cn = "contentCtx";
                switch (type) {
                case CCNX_TLV_C_KeyID:  tn = "KeyID"; break;
                case CCNX_TLV_C_NameAuth:   tn = "NameAuth"; break;
                case CCNX_TLV_C_ProtoInfo:  tn = "ProtoInfo"; break;
                case CCNX_TLV_C_Contents:   tn = "Contents"; break;
                case CCNX_TLV_C_SigBlock:   tn = "SigBlock"; break;
                case CCNX_TLV_C_Suite:  tn = "Suite"; break;
                case CCNX_TLV_C_PubKeyLoc:  tn = "PubKeyLoc"; break;
                case CCNX_TLV_C_Key:    tn = "Key"; break;
                case CCNX_TLV_C_Cert:   tn = "Cert"; break;
                case CCNX_TLV_C_KeyNameKeyID: tn = "KeyNameKeyID"; break;
                case CCNX_TLV_C_ObjInfo:    tn = "ObjInfo"; break;
                case CCNX_TLV_C_ObjType:    tn = "ObjType"; break;
                case CCNX_TLV_C_Create: tn = "Create"; break;
                case CCNX_TLV_C_Sigbits:    tn = "Sigbits"; break;
                case CCNX_TLV_C_KeyLocator: tn="KeyLocator"; break;
                default: break;
                }
                break;
            case CTX_NAMEAUTH:
                cn = "nameauthCtx";
                break;
            default:
                cn = NULL;
                break;
        }
    }
    if (tn) {
        sprintf(tmp, "%s\\%s", tn, cn);
    } else if (cn) {
        sprintf(tmp, "type=0x%04x\\%s", type, cn);
    } else {
        sprintf(tmp, "type=0x%04x\\ctx=%d", type, ctx);
    }
    return tmp;
}


static int
ccntlv_parse_sequence(int lev, unsigned char ctx, unsigned char *base,
              unsigned char **buf, unsigned int *len, char *cur_tag, int rawxml, FILE* out)
{
    unsigned int vallen, typ, i;
    unsigned char ctx2, *cp;
    char *n, tmp[100];

    while (*len > 0) {
        cp = *buf;
        if (ccnl_ccntlv_dehead(lev, base, buf, len, &typ, &vallen) < 0) {
            return -1;
        }

        if (vallen > *len) {
          fprintf(stderr, "\n%04zx ** CCNTLV length problem:\n"
              "  type=%hu, len=%hu larger than %d available bytes\n",
              *buf - base, typ, vallen, *len);
          exit(-1);
        }

        n = ccnl_ccntlv_type2name(ctx, typ);
        if (!n) {
            sprintf(tmp, "type=%hu", typ);
            n = tmp;
        }

        fprintf(out, "%04zx  ", cp - base);
        for (i = 0; i < lev; i++) {
            fprintf(out, "  ");
        }
        for (; cp < *buf; cp++) {
            fprintf(out, "%02x ", *cp);
        }
        fprintf(out, "-- <%s, len=%d>\n", n, vallen);

        ctx2 = ccntlv_must_recurse(ctx, typ);
        if (ctx2) {
            *len -= vallen;
            if (ccntlv_parse_sequence(lev+1, ctx2, base, buf, &vallen, n, rawxml, out) < 0)
            return -1;
        } else {
            hexdump(lev, base, *buf, vallen, rawxml, out);
            *buf += vallen;
            *len -= vallen;
        }
    }

    return 0;
}


void
ccntlv_201311(unsigned char *data, unsigned int len, int rawxml, FILE* out)
{
    unsigned char *buf;
    char *mp;
    struct ccnx_tlvhdr_ccnx201311_s *hp;

    hp = (struct ccnx_tlvhdr_ccnx201311_s*) data;

    fprintf(out, "%04zx  hdr.vers=%d\n",
       (unsigned char*) &(hp->version) - data, hp->version);
    if (hp->msgtype == CCNX_TLV_TL_Interest)
    mp = "Interest\\toplevelCtx";
    else if (hp->msgtype == CCNX_TLV_TL_Object)
    mp = "Object\\toplevelCtx";
    else
    mp = "unknown";
    fprintf(out, "%04zx  hdr.mtyp=0x%02x (%s)\n",
       (unsigned char*) &(hp->msgtype) - data, hp->msgtype, mp);
    fprintf(out, "%04zx  hdr.plen=%d\n",
       (unsigned char*) &(hp->payloadlen) - data, ntohs(hp->payloadlen));
    fprintf(out, "%04zx  hdr.olen=%d\n",
       (unsigned char*) &(hp->hdrlen) - data, ntohs(hp->hdrlen));

    if (8 + ntohs(hp->hdrlen) + ntohs(hp->payloadlen) != len) {
       fprintf(stderr, "length mismatch\n");
    }

    buf = data + 8;
    // dump the sequence of TLV fields of the optional header
    len = ntohs(hp->hdrlen);
    if (len > 0) {
        ccntlv_parse_sequence(0, CTX_HOP, data, &buf, &len, "header", rawxml, out);
        if (len != 0) {
            fprintf(stderr, "%d left over bytes in header\n", len);
        }
    }
    fprintf(out, "%04zx  hdr.end\n", buf - data);

    // dump the sequence of TLV fields of the payload
    len = ntohs(hp->payloadlen);
    buf = data + 8 + ntohs(hp->hdrlen);
    ccntlv_parse_sequence(0, CTX_TOPLEVEL, data, &buf, &len, "payload", rawxml, out);
    if (len != 0) {
    }

    fprintf(out, "%04zx  pkt.end\n", buf - data);
}

// ----------------------------------------------------------------------

#define NDN_TLV_MAX_TYPE 256
static char ndntlv_recurse[NDN_TLV_MAX_TYPE];

static char*
ndn_type2name(unsigned type)
{
    char *n;

    switch (type) {
    case NDN_TLV_Interest:      n = "Interest"; break;
    case NDN_TLV_Data:          n = "Data"; break;
    case NDN_TLV_Name:          n = "Name"; break;
    case NDN_TLV_NameComponent:     n = "NameComponent"; break;
    case NDN_TLV_Selectors:     n = "Selectors"; break;
    case NDN_TLV_Nonce:         n = "Nonce"; break;
    case NDN_TLV_Scope:         n = "Scope"; break;
    case NDN_TLV_InterestLifetime:  n = "InterestLifetime"; break;
    case NDN_TLV_MinSuffixComponents:   n = "MinSuffixComponents"; break;
    case NDN_TLV_MaxSuffixComponents:   n = "MaxSuffixComponents"; break;
    case NDN_TLV_PublisherPublicKeyLocator: n = "PublisherPublicKeyLocator"; break;
    case NDN_TLV_Exclude:       n = "Exclude"; break;
    case NDN_TLV_ChildSelector:     n = "ChildSelector"; break;
    case NDN_TLV_MustBeFresh:       n = "MustBeFresh"; break;
    case NDN_TLV_Any:           n = "Any"; break;
    case NDN_TLV_MetaInfo:      n = "MetaInfo"; break;
    case NDN_TLV_Content:       n = "Content"; break;
    case NDN_TLV_SignatureInfo:     n = "SignatureInfo"; break;
    case NDN_TLV_SignatureValue:    n = "SignatureValue"; break;
    case NDN_TLV_ContentType:       n = "ContentType"; break;
    case NDN_TLV_FreshnessPeriod:   n = "FreshnessPeriod"; break;
    case NDN_TLV_FinalBlockId:      n = "FinalBlockId"; break;
    case NDN_TLV_SignatureType:     n = "SignatureType"; break;
    case NDN_TLV_KeyLocator:        n = "KeyLocator"; break;
    case NDN_TLV_KeyLocatorDigest:  n = "KeyLocatorDigest"; break;
    default:
    n = NULL;
    }
    return n;
}


static int
ndn_parse_sequence(int lev, unsigned char *base, unsigned char **buf,
               int *len, char *cur_tag, int rawxml, FILE* out)
{
    int typ, vallen, i, maxi;
    unsigned char *cp;
    char *n, tmp[100];

    while (*len > 0) {
        cp = *buf;
        if (ccnl_ndntlv_dehead(buf, len, &typ, &vallen) < 0)
            return -1;
        if (vallen > *len) {
            fprintf(stderr, "\n%04zx ** NDN_TLV length problem for %s:\n"
                "  type=%hu, len=%hu larger than %d available bytes\n",
                cp - base, base, typ, vallen, *len);
            exit(-1);
        }

        n = ndn_type2name(typ);
        if (!n) {
            sprintf(tmp, "type=%hu", typ);
            n = tmp;
        }

        if(!rawxml) {
            fprintf(out, "%04zx  ", cp - base);
        }

        for (i = 0; i < lev; i++)
            fprintf(out, "  ");
        if(!rawxml) {
            for (; cp < *buf; cp++)
                fprintf(out, "%02x ", *cp);
        }

        if(rawxml) {
            fprintf(out, "<%s>\n", n);
        } else {
            fprintf(out, "-- <%s, len=%d>\n", n, vallen);
        }

        if (typ < NDN_TLV_MAX_TYPE && ndntlv_recurse[typ]) {
            *len -= vallen;
            if (ndn_parse_sequence(lev+1, base, buf, &vallen, n, rawxml, out) < 0) {
                return -1;
            }
            fprintf(out, "</%s>\n", n);
            continue;
        }

        // printf("BASE: %s\n", base);
        if(rawxml && vallen > 0) {
            fprintf(out, "<data size=\"%i\" dt=\"binary.base64\">\n", vallen);
            base64dump(lev, base, *buf, vallen, rawxml, out);
            fprintf(out, "</data>\n");


            *len -= vallen;
            *buf += vallen;
        } else {

            while (vallen > 0) {
                maxi = vallen > 8 ? 8 : vallen;
                cp = *buf;
                fprintf(out, "%04zx  ", cp - base);
                for (i = 0; i < lev+1; i++)
                    fprintf(out, "  ");
                for (i = 0; i < 8; i++, cp++)
                    fprintf(out, i < maxi ? "%02x " : "   ", *cp);
                cp = *buf;
                for (i = 79 - 6 - 2*(lev+1) - 8*3 - 12; i > 0; i--)
                fprintf(out, " ");
                fprintf(out, "  |");
                for (i = 0; i < maxi; i++, cp++)
                fprintf(out, "%c", isprint(*cp) ? *cp : '.');
                fprintf(out, "|");
                fprintf(out, "\n");

                vallen -= maxi;
                *buf += maxi;
                *len -= maxi;
            }
        }
        if(rawxml) {
            fprintf(out, "</%s>\n", n);
        }
    }
    return 0;
}

static void
ndntlv_201311(unsigned char *data, int len, int rawxml, FILE* out)
{
    unsigned char *buf = data;

    // dump the sequence of TLV fields, should start with a name TLV
    ndn_parse_sequence(0, data, &buf, &len, "payload", rawxml, out);
    if(!rawxml) {
        fprintf(out, "%04zx  pkt.end\n", buf - data);
    }
}

static void
ndn_init()
{
    ndntlv_recurse[NDN_TLV_Interest] = 1;
    ndntlv_recurse[NDN_TLV_Data] = 1;
    ndntlv_recurse[NDN_TLV_Name] = 1;
    ndntlv_recurse[NDN_TLV_Selectors] = 1;
    ndntlv_recurse[NDN_TLV_MetaInfo] = 1;
    ndntlv_recurse[NDN_TLV_SignatureInfo] = 1;
    ndntlv_recurse[NDN_TLV_KeyLocator] = 1;
}

// ----------------------------------------------------------------------

int
localrpc_parse(int lev, unsigned char *base, unsigned char **buf, int *len, int rawxml, FILE* out)
{
    int typ, vallen, i;
    unsigned char *cp;
    char *n, tmp[100], dorecurse;

    while (*len > 0) {
    cp = *buf;

    dorecurse = 0;
    if (*cp < NDN_TLV_RPC_APPLICATION) {
        sprintf(tmp, "Variable=v%02x", *cp);
        n = tmp;
        *buf += 1;
        *len -= 1;
        vallen = 0;
    } else {
        if (ccnl_ndntlv_dehead(buf, len, &typ, &vallen) < 0)
        return -1;
        if (vallen > *len) {
        fprintf(stderr, "\n%04zx ** NDN_TLV_RPC length problem:\n"
            "  type=%hu, len=%hu larger than %d available bytes\n",
            cp - base, typ, vallen, *len);
        exit(-1);
        }
        switch(typ) {
        case NDN_TLV_RPC_APPLICATION:
        n = "Application"; dorecurse = 1; break;
        case NDN_TLV_RPC_LAMBDA:
        n = "Lambda"; dorecurse = 1; break;
        case NDN_TLV_RPC_SEQUENCE:
        n = "Sequence"; dorecurse = 1; break;
        case NDN_TLV_RPC_NONNEGINT:
        n = "Integer"; break;
        case NDN_TLV_RPC_ASCII:
        n = "ASCII"; break;
        case NDN_TLV_RPC_BIN:
        n = "BinaryData"; break;
        default:
        sprintf(tmp, "Type=0x%x", typ);
        n = tmp;
        break;
        }
    }

    printf("%04zx  ", cp - base);
    for (i = 0; i < lev; i++)
        printf("  ");
    for (; cp < *buf; cp++)
        printf("%02x ", *cp);
    printf("-- <%s, len=%d>\n", n, vallen);

    if (dorecurse) {
        localrpc_parse(lev+1, base, buf, len, rawxml, out);
        continue;
    }

    if (typ == NDN_TLV_RPC_NONNEGINT) {
        printf("%04zx  ", *buf - base);
        for (i = 0; i <= lev; i++)
        printf("  ");
        printf("%ld\n", ccnl_ndntlv_nonNegInt(*buf, vallen));
    } else if (typ == NDN_TLV_RPC_ASCII) {
        printf("%04zx  ", *buf - base);
        for (i = 0; i <= lev; i++)
        printf("  ");
        strcpy(tmp, "\"");
        i = sizeof(tmp) - 6;
        if (vallen < i)
        i = vallen;
        memcpy(tmp+1, *buf, i);
        strcpy(tmp + i + 1, vallen > i ? "\"..." : "\"");
        printf("%s\n", tmp);
    } else if (vallen > 0)
        hexdump(lev, base, *buf, vallen, rawxml, out);
    *buf += vallen;
    *len -= vallen;
    }
    return 0;
}

static void
localrpc_201405(unsigned char *data, int len, int rawxml, FILE* out)
{
    unsigned char *buf = data;

    int origlen = len, typ, vallen, typ2, vallen2, len2;
    unsigned char *cp;

    if (len <= 0 || ccnl_ndntlv_dehead(&buf, &len, &typ, &vallen) < 0 ||
                    typ != NDN_TLV_RPC_APPLICATION)
    return;

    cp = buf;
    len2 = vallen;
    if (ccnl_ndntlv_dehead(&buf, &len2, &typ2, &vallen2) < 0)
    return;
    if (typ2 == NDN_TLV_RPC_NONNEGINT) { // RPC return code
    printf("0000  RPC-result\n");
    printf("%04zx    INT %ld\n", cp - data,
           ccnl_ndntlv_nonNegInt(buf, vallen2));
    buf += vallen2;
    len = origlen - (buf - data);
    localrpc_parse(1, data, &buf, &len, rawxml, out);
    } else { // RPC request
    buf = data;
    localrpc_parse(0, data, &buf, &origlen, rawxml, out);
    }

    printf("%04zx  pkt.end\n", buf - data);
}

// ----------------------------------------------------------------------

int
pkt2suite(unsigned char *data, int len)
{
    if (len < 1)
    return -1;

    switch (*data) {

    case 0x01:
    case 0x04:
    return SUITE_CCNB;

    case 0x05:
    case 0x06:
        return SUITE_NDNTLV;

    case 0x80:
        return SUITE_LOCRPC;

    case 0x00:
    if (len > 1 &&
        (data[1] == CCNX_TLV_TL_Interest || data[1] == CCNX_TLV_TL_Object))
            return SUITE_CCNTLV;
    // fall through

    default:
    break;
    }

    return -1;
}

#ifndef USE_JNI_LIB

int
main(int argc, char *argv[])
{
    int opt, rc;
    unsigned char data[64*1024];
    char *forced = "forced";
    int len, maxlen, suite = -1, rawxml = 0;
    FILE *out = stdout;

    ndn_init();
    while ((opt = getopt(argc, argv, "hs:f:")) != -1) {
        switch (opt) {
        case 's':
            suite = atoi(optarg);
            break;
        case 'f':
            rawxml = atoi(optarg);
            break;
    default:
    help:
            fprintf(stderr, "usage: %s [options] <encoded_data\n"
            "  -h           this help\n"
            "  -s SUITE     (0=ccnb, 1=ccntlv, 2=ndntlv)\n"
            "  -f FORMAT    (0=readable, 1=rawxml)\n",
            argv[0]);
            exit(1);
    }
    }

    if (argv[optind])
    goto help;

    len = 0;
    maxlen = sizeof(data);
    while (maxlen > 0) {
    rc = read(0, data+len, maxlen);
    if (rc == 0)
        break;
    if (rc < 0) {
        perror("read");
        return 1;
    }
    len += rc;
    maxlen -= rc;
    }

    if(!rawxml) {
        printf("# ccn-lite-pktdump, parsing %d byte%s\n", len, len!=1 ? "s":"");
    }

    if (len == 0) {
       goto done;
    }
    if (suite == -1) {
        suite = pkt2suite(data, len);
        forced = "auto-detected";
    }
    switch (suite) {
        case SUITE_CCNB:
            if(!rawxml) {
                printf("#   %s CCNB format\n#\n", forced);
            }
            ccnb_parse(data, len, rawxml, out);
            break;
        case SUITE_CCNTLV:
            if(!rawxml) {
                printf("#   %s CCNx TLV format (as of Mar 2014)\n#\n", forced);
            }
            ccntlv_201311(data, len, rawxml, out);
            break;
        case SUITE_NDNTLV:
            if(!rawxml) {
                printf("#   %s NDN TLV format (as of Mar 2014)\n#\n", forced);
            }
            ndntlv_201311(data, len, rawxml, out);
            break;
        case SUITE_LOCRPC:
            if(!rawxml) {
                printf("#   %s NDN TLV format, local RPC (May 2014)\n#\n", forced);
            }
            localrpc_201405(data, len, rawxml, out);
            break;
        default:
            if(!rawxml) {
                printf("#   unknown pkt format, showing plain hex\n");
            }
            hexdump(-1, data, data, len, rawxml, out);
    done:
        if(!rawxml) {
            printf("%04x  pkt.end\n", len);
        }
        break;
    }

    return 0;
}
#endif
// eof
