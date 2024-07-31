/* Copyright (c) 2015, 2024, Oracle and/or its affiliates.*/
/* All rights reserved.*/

/*
   NAME
     jzntypes.h - JSON Public types

   DESCRIPTION

   RELATED DOCUMENTS
 
   EXPORT FUNCTION(S)

   INTERNAL FUNCTION(S)

   EXAMPLES

   NOTES
     This is a public (to customers) header file and world-viewable.
     The file can include only public headers.

   MODIFIED   (MM/DD/YY)
   dmcmahon    01/05/24 - Bug 36170748: add JZNCOLLATION_VECTOR
   dmcmahon    12/30/23 - Bug 36148907: opaque sub-structure for scalar union
   zliu        12/18/23 - bug 36132471: vector type into json xdk 
   dmcmahon    09/13/23 - Bug 35802662: jznScalarProperties
   dmcmahon    09/01/23 - Bug 35770383: JZN_STEP_NESTED
   dmcmahon    06/29/23 - Bug 35549471: move jznuHashCtx from jzn.h
   dmcmahon    08/19/22 - Bug 34501699: JsonStepLocation move flags
   dmcmahon    08/09/22 - Bug 34477101: JZN_BINFMT_LEN == 48
   dmcmahon    07/15/22 - Bug 34391191: swap collation of object and array
   dmcmahon    05/20/22 - Bug 34182736: JZNVAL_DOC_NODE
   dmcmahon    05/07/22 - Bug 34152043: Clean up location flags
   dmcmahon    04/16/22 - Bug 34078793: carrier type JZNVAL_ANSI_DATE
   dmcmahon    03/29/22 - Bug 34077835: define JZN_NULL as (void *)0
   sriksure    02/14/22 - Bug 33845235: Cast mode of schema validation
   dmcmahon    01/16/22 - Bug 33758217: JZNVAL_ANY_TYPE
   dmcmahon    01/02/22 - Bug 33731151: move jznDomSortPath from jzn0.h
   dmcmahon    12/05/21 - Bug 33637994: JZNVAL_ORA_BOOLEAN for DTYBOL
   dmcmahon    11/05/21 - Bug 33541968: add JsonStepLocation
   dmcmahon    06/19/21 - Bug 33022120: Add JZNCOLLATION_HASH for anyNode
   dmcmahon    06/10/21 - Bug 32988631: JZNCOLLATION byte code changes
   dmcmahon    06/08/21 - Bug 32973954: add JZNCOLLATION_END_ARRAY
   dmcmahon    06/06/21 - Bug 32969293: minkey/maxkey/array-start collations
   dmcmahon    06/04/21 - Bug 32967328: collation types for array/object
   dmcmahon    05/19/21 - Bug 32904319: add jznCollationType
   dmcmahon    01/04/21 - Bug 32340636: add support for JSON comments
   dmcmahon    07/17/20 - Bug 31748678: use JsonDateTime for JZNVAL_TSTAMP
   dmcmahon    08/12/20 - Bug 31747292: OID/UUID in-line
   dmcmahon    06/28/20 - Define limits
   dmcmahon    06/04/20 - Move definition of jznFatalErrHandler to jznerr.h
   sriksure    03/24/20 - Bug 31075323: Add JZNVAL type, setter for LdiInterval
   sriksure    02/20/20 - Bug 30918101: Unpickle native binary values
   zliu        02/17/20 - bug 30922936: add JZN_ORA_TIMESTAMP_TZ_LEN
   zliu        04/07/19 - Bug 29599067: add JZN_ID_MAX_LEN
   dmcmahon    04/06/19 - Add JZNVAL_ID
   zliu        03/13/19 - Bug 29514638: enable JZNVAL_CANONICAL_FLOAT &
                          JZNVAL_CANONICAL_DOUBLE
   dmcmahon    11/22/18 - Bug 28962560: JZNVAL_BASE64 "signal" type
   zliu        11/10/18 - Bug 28908389: oson needs to handle 7 byte oralce 
                          timestamp storage format, add JZN_ORA_TIMESTAMP7_LEN
   sriksure    09/27/18 - Bug 28717127: Rationalize includes in public jzn*.h
   zliu        08/27/18 - Bug 28559663: add JZN_ORA_FLOAT_LEN, 
                          JZN_ORA_DOUBLE_LEN,JZN_ORA_DATE_LEN,
                          JZN_ORA_TIMESTAMP_LEN,JZN_ORA_YEAR_MON_DUR_LEN,
                          JZN_ORA_DAY_SEC_DUR_LEN   
   sriksure    08/23/18 - Bug 28546488: XDK C JSON APIs enhancements
   dmcmahon    07/08/18 - bug 28300790: JZNVAL_UUID, JZN_BSON_UUID_LEN
   dmcmahon    07/08/18 - bug 28300790: JZNVAL_UUID, JZN_BSON_UUID_LEN
   zliu        02/21/18 - Bug 27574330: add JZN_BSON_OID_LEN, 
                                        JZN_BSON_DEC128_LEN
   dmcmahon    01/29/18 - bug 27450657: decimal128 type
   bhammers    10/23/17 - bug 26767740: add JZNEV_COL_SIBL_NUL
   dmcmahon    04/24/17 - Bug 25945093: JZNVAL_ORA_TIME
   dmcmahon    07/15/16 - Fix position to accomodate large files
   dmcmahon    07/10/15 - Bug 21365846: moved error codes to jznerr.h
   dmcmahon    07/01/15 - DOM referential integrity errors
   bhammers    05/28/15 - add clob and blob
   bhammers    03/31/15 - added JZNERR_DOM_KEY_NOT_EXIST
   dmcmahon    03/12/15 - Creation

*/

#ifndef JZNTYPES_ORACLE
# define JZNTYPES_ORACLE

#ifndef ORATYPES
# include <oratypes.h> 
#endif

#ifndef ORASTRUC
# include <orastruc.h>
#endif

#ifndef XML_ORACLE
# include <xml.h>
#endif

#ifndef JZNERR_ORACLE
# include <jznerr.h>
#endif

/*---------------------------------------------------------------------------
                     PUBLIC TYPES AND CONSTANTS
  ---------------------------------------------------------------------------*/

/* This should be used to terminate varargs lists */
#define JZN_NULL  ((void *)0)

/*
** Limits
*/
#define JZN_MAX_ARRAY_SLICES  0x4000 /* Maximum array subscripts            */
#define JZN_MAX_PATH_LENGTH   4000   /* Maximum length of a path string     */
#define JZN_MAX_FIELDNAME     256    /* Matches JZN0DOM_FIELDNAME_MAX       */
#define JZN_MAX_DEPTH         1024   /* Maximum number of nested containers */

#define JZN_BSON_DEC128_LEN          16
#define JZN_BSON_OID_LEN             12
#define JZN_BSON_UUID_LEN            16

/* storage length for fixed-length scalar datatypes encoding in OSON */
/* The storage length is taken from Oracle core encoding of scalar datatypes */
/* WARNING: disk persistent data compatible with Oracle scalar datatype
 * storage format and length. 
 */
#define JZN_ORA_DATE_LEN              7 
/* Oracle timestamp datatype has two valid lengths: 7 or 11 
 * 7 bytes is for timestamp without fractional second component
 * 11 bytes is for timestamp with non-zero fractional second component
 *
 * According to oracore3/include/ldi0.h:
 *
 * LDISTAMPZEROARRAYLEN  = 7 which is
 * (2 bytes for year, 1 for month, day, hour, minute, second )
 *
 * LDISTAMPARRAYLEN  = 11 which is
 * (2 bytes for year, 1 for month, day, hour, minute, second, 
 *  4 for fractional seconds)
 */
#define JZN_ORA_TIMESTAMP7_LEN       JZN_ORA_DATE_LEN
#define JZN_ORA_TIMESTAMP_LEN        11
#define JZN_ORA_TIMESTAMP_TZ_LEN     13
#define JZN_ORA_YEAR_MON_DUR_LEN      5
#define JZN_ORA_DAY_SEC_DUR_LEN      11
#define JZN_ORA_BOOLEAN_LEN           1
#define JZN_ORA_FLOAT_LEN             4 
#define JZN_ORA_DOUBLE_LEN            8
#define JZN_ORA_NUM_MAX_LEN          22 /* LNXM_NUM */
#define JZN_ID_MAX_LEN               127

#define JZN_INLINE_MAX_LEN           15

/*
** Max size of any "binary" type:
**   OraNum, date/timestamp/tstz, durations, OID/UUID, float/double
*/
#define JZN_ORA_MAX_BIN_LEN          24

/*
** Large enough for any Oracle date/time/number/interval type.
** Large enough for any standard formatted string of said types.
*/
#define JZN_BINFMT_LEN               48

typedef sword jsonerr; /* ### Why bother defining this as a type? ### */

/*
** JSON Event types, max Value is 15 because we store event in 4 bits.
** If you need more, no problem, but jznEng.c needs to be modified!
*/
typedef enum jznevent
{
  JZNEV_START_OBJECT   = 0,
  JZNEV_END_OBJECT     = 1,
  JZNEV_START_PAIR     = 2,
  JZNEV_KEY            = 2,
  JZNEV_END_PAIR       = 3,
  JZNEV_START_ARRAY    = 4,
  JZNEV_END_ARRAY      = 5,
  JZNEV_ITEM           = 6,
  JZNEV_ERROR          = 7,
  JZNEV_END            = 8,
  JZNEV_POS            = 9,
  JZNEV_INVALID        = 10,
  JZNEV_START_COMPOUND = 11,  /* Start of BSON composite value */
  JZNEV_END_COMPOUND   = 12,  /* End of BSON composite value */
  JZNEV_ROW_SEPARATOR  = 13,  /* separate values belonging to different rows */
  JZNEV_COL_NO_VALUE   = 14,  /* no value (null column value is json table)  */
  JZNEV_COL_SIBL_NUL   = 15,  /* NULL value for sibling branch */
  JZNEV_COMMENT_BEFORE = 16,
  JZNEV_COMMENT_AFTER  = 17
} jznevent;

/*
** JSON Value types
*/
typedef enum jznvaltype
{
    JZNVAL_UNKNOWN = 0,
    /*
    ** JSON "native" types (all represented as strings)
    */
    JZNVAL_NONE    = 1,   /* set if this isn't a value, e.g. a field name */
    JZNVAL_NULL    = 2,   /* literal null */
    JZNVAL_STRING  = 3,   /* quoted string */
    JZNVAL_NUMBER  = 4,   /* unquoted numeric value */
    JZNVAL_FALSE   = 5,   /* literal false */
    JZNVAL_TRUE    = 6,   /* literal true */
    /*
    ** JSON Type system extensions:
    */
    JZNVAL_SINT    = 7,   /* Signed 32-bit integer - BSON, Avro, Protobuf */
    JZNVAL_SLONG   = 8,   /* Signed 64-bit integer - BSON, Avro, Protobuf */
    JZNVAL_UINT    = 9,   /* Unsigned 32-bit integer - Protobuf */
    JZNVAL_ULONG   = 10,  /* Unsigned 64-bit integer - Protobuf */
    JZNVAL_FLOAT   = 11,  /* 32-bit float - Avro, Protobuf */
                          /* Oracle canonical float DTYBFLOAT is handled
                           * by storing it in oradbfltbin[4]
                           */
    JZNVAL_DOUBLE  = 12,  /* 64-bit float - BSON, Avro, Protobuf */
                          /* Oracle canonical double DTYBDOUBLE is handled
                           * by storing it in oradbdvbin[8]
                           */
    JZNVAL_BINARY  = 13,  /* byte array - BSON, Avro, Protobuf */
    JZNVAL_TSTAMP  = 14,  /* UTC timestamp, in-line form */
    JZNVAL_OID     = 15,  /* MongoDB 12-byte binary object ID */
    JZNVAL_PTR     = 15,  /* MongoDB 12-byte pointer (deprecated) */
    JZNVAL_UUID    = 16,  /* UUID - 16 byte raw */
    /*
    ** ### Mongo also has:
    ** ###   binary sub-types
    ** ###   type for symbol (deprecated)
    ** ###   type for pointer (deprecated)
    ** ###   type for Javascript code
    ** ###   type for internal time stamp
    ** ###   types for min/max keys
    ** ###   compound regex (expression, options)
    ** ###   compound code  (JS code, document)
    */
    /*
    ** Oracle native types
    */
    /* Oracle Compress Tree JSON Binary (OSON) for SQL Scalar datatype */
    /* Note Oracle OSON uses this Oracle native binary encoding that
     * provides efficent code paths to sql/json path engine without any
     * type comparsion.
     * Note the underlying binary value accessible from
     * jzn_sval_union is jzn_sval_union.bval.value and 
     * jzn_val_union.bval.valuelength
     * For in-memory cached OSON byte buffer, the jzn_sval_union.bval.value
     * directly points to the underlying OCGBSON byte  buffer.
     * For large OSON bytes, the jzn_sval_union.bval.value
     * points to obinval.oibintypbuf[].
     */
    JZNVAL_ORA_NUMBER        = 17, /* Oracle DTYNUM format   */
    JZNVAL_ORA_DATE          = 18, /* Oracle DTYDAT format   */
    JZNVAL_ORA_TIMESTAMP     = 19, /* Oracle DTYSTAMP format */
    JZNVAL_ORA_TIMESTAMPTZ   = 20, /* Oracle DTYSTZ format   */
    JZNVAL_ORA_YEARMONTH_DUR = 21, /* Oracle DTYIYM format   */
    JZNVAL_ORA_DAYSECOND_DUR = 22, /* Oracle DTYIDS format   */

    JZNVAL_DECIMAL           = 27, /* DECIMAL 128 in BID form */
    JZNVAL_DEC128BID         = 27, /* BSON decimal128 BID     */
    /* Note we encode Mongo BSON sb4/sb8/dec128 using Oracle number.
     *
     * The reason is because the entire Oracle ecosystem RDBMS/OCI
     * stacks uses Oracle number format. Using Oracle number format minmizes
     * number format conversions for efficient SQL/JSON support.
     * To name a few:
     *  json_value (returning number requries just memcpy)
     *  sql/json path engine Range comparision just follows Oracle memcmp() 
     *  conversion.
     *
     * If someday, Oracle ecosystem natively supports sb4/sb8/deciaml128 
     * througout its RDBMS/OCI stack, then we can revisist this.
     *
     * However, to keep the BSON fidelity of sb4/sb8/dec128 so that we
     * can recover the provenance of those integer/dec128 datatypes,
     * we use following 3 jznvaltype to remember its type fidelity.
     */
    JZNVAL_SINT_ORANUM       = 28, /* Original Signed 32-bit integer
                                    * encoded using ORA number format
                                    */
    JZNVAL_SLONG_ORANUM      = 29, /* Original Signed 64-bit integer
                                    * encoded using ORA number format
                                    */
    JZNVAL_DEC128BID_ORANUM  = 30, /* Original dec128 format
                                    * encoded using ORA number format
                                    */
    JZNVAL_ID                = 31, /* ID (length + 0-127 bytes)       */

    /*
    ** These types are storable in OSON
    */
    JZNVAL_CANONICAL_FLOAT   = 38, /* Oracle float, canonical format  */
    JZNVAL_CANONICAL_DOUBLE  = 39, /* Oracle double, canonical format */
    JZNVAL_VECTOR            = 45, /* vector type binary (node)       */
    /*
    ** If new storable types are added, assign numbers from the highest
    ** value below.
    */

    /*
    ** These types are used only for conveying information to scalar
    ** constructor and serialization calls. They cannot be true events
    ** from the parsers and decoders, nor can they appear as DOM node
    ** scalar types. (See JZNVAL_CLOB, JZNVAL_BLOB for similar examples.)
    */
    JZNVAL_OPAQUE            = 23, /* DOM instance (PL/SQL only) */
    JZNVAL_CLOB              = 24, /* CLOB (used for setClob)    */
    JZNVAL_CHR_STREAM        = 24, /* orastream * UTF8 string    */
    JZNVAL_BLOB              = 25, /* BLOB (used for setBlob)    */
    JZNVAL_BIN_STREAM        = 25, /* orastream * RAW data       */
    JZNVAL_ORA_TIME          = 26, /* Oracle DTYTIME format  */
                                   /* ### Do we ever need DEC128DPD? ### */
    JZNVAL_OCI_NUMBER        = 32, /* VARNUM (length byte + OraNum)   */
    JZNVAL_OCI_DATE          = 33, /* ldxg structure date+time        */
    JZNVAL_OCI_DATETIME      = 34, /* LdiDateTime structure           */
    JZNVAL_OCI_STRING        = 35, /* Pro*C style LONG VARCHAR struct */
    JZNVAL_OCI_RAW           = 36, /* Pro*C style LONG VARRAW struct  */
    JZNVAL_BASE64            = 37, /* binary to render in base-64     */

    /* This is another conveyance type */
    JZNVAL_OCI_INTERVAL      = 40, /* LdiInterval structure           */
    /* This type is storable in OSON */
    JZNVAL_ORA_BOOLEAN       = 41, /* Oracle out-of-line boolean      */
    /* More marker types */
    JZNVAL_ANY_TYPE          = 42, /* memcmp-able poly type image     */
    JZNVAL_ANSI_DATE         = 43, /* Oracle date without time        */
    JZNVAL_DOC_NODE          = 44, /* DOM doc+node pair               */

    JZNVAL_VEC_STREAM        = 46, /* orastream * VECTOR bin data     */
    JZNVAL_DOUBLE_ARRAY      = 47, /* array of BINARY_DOUBLE          */
    JZNVAL_FLOAT_ARRAY       = 48  /* array of BINARY_FLOAT           */
    /*
    ** Do not exceed a maximum of 63 for a code value.
    ** Some JSON facilities encode the type in a 6-bit field.
    */
} jznvaltype;

/*
** These types order scalar domains versus each other.
*/
typedef enum jznCollationType
{
   JZNCOLLATION_UNKNOWN         = 0,
   JZNCOLLATION_END_ARRAY       = 0,
   JZNCOLLATION_END_OBJECT      = 0,
   JZNCOLLATION_MINKEY          = 1,
   JZNCOLLATION_NULL            = 2,
   JZNCOLLATION_NUMBER          = 3,  /* All numeric types */
   JZNCOLLATION_STRING          = 4,
   JZNCOLLATION_BINARY          = 7,
   JZNCOLLATION_ID              = 8,  /* Includes OID and UUID */
   JZNCOLLATION_BOOLEAN         = 9,  /* Both false and true */
   JZNCOLLATION_DATETIME        = 10, /* date/timestamp/tstz */
   /* Type 13 reserved for MaxKey */
   JZNCOLLATION_YMINTERVAL      = 20,
   JZNCOLLATION_DSINTERVAL      = 21,

   /* Types up to 127 reserved for simple scalars */
   JZNCOLLATION_VECTOR          = 128,

   /* These must be near the end of the range */
   JZNCOLLATION_HASH            = 240, /* SHA256 replacement */
   JZNCOLLATION_START_OBJECT    = 250,
   JZNCOLLATION_START_ARRAY     = 251,
   JZNCOLLATION_FIELD_NAME      = 254,
   JZNCOLLATION_MAXKEY          = 255
} jznCollationType;

#define jznValTypeToCollationType(_jznvaltyp, _collationtyp) \
do                                             \
{                                              \
  switch((_jznvaltyp))                         \
  {                                            \
  case JZNVAL_NULL:                            \
    (_collationtyp) = JZNCOLLATION_NULL;       \
    break;                                     \
  case JZNVAL_FALSE:                           \
  case JZNVAL_TRUE:                            \
    (_collationtyp) = JZNCOLLATION_BOOLEAN;    \
    break;                                     \
  case JZNVAL_DEC128BID_ORANUM:                \
  case JZNVAL_DEC128BID:                       \
  case JZNVAL_NUMBER:                          \
  case JZNVAL_ORA_NUMBER:                      \
  case JZNVAL_SINT:                            \
  case JZNVAL_SINT_ORANUM:                     \
  case JZNVAL_SLONG:                           \
  case JZNVAL_SLONG_ORANUM:                    \
  case JZNVAL_UINT:                            \
  case JZNVAL_ULONG:                           \
  case JZNVAL_CANONICAL_FLOAT:                 \
  case JZNVAL_FLOAT:                           \
  case JZNVAL_DOUBLE:                          \
  case JZNVAL_CANONICAL_DOUBLE:                \
    (_collationtyp) = JZNCOLLATION_NUMBER;     \
    break;                                     \
  case JZNVAL_STRING:                          \
    (_collationtyp) = JZNCOLLATION_STRING;     \
    break;                                     \
  case JZNVAL_BINARY:                          \
    (_collationtyp) = JZNCOLLATION_BINARY;     \
    break;                                     \
  case JZNVAL_VECTOR:                          \
    (_collationtyp) = JZNCOLLATION_VECTOR;     \
    break;                                     \
  case JZNVAL_UUID:                            \
  case JZNVAL_OID:                             \
  case JZNVAL_ID:                              \
    (_collationtyp) = JZNCOLLATION_ID;         \
    break;                                     \
  case JZNVAL_TSTAMP:                          \
  case JZNVAL_ORA_DATE:                        \
  case JZNVAL_ORA_TIMESTAMP:                   \
  case JZNVAL_ORA_TIMESTAMPTZ:                 \
    (_collationtyp) = JZNCOLLATION_DATETIME;   \
    break;                                     \
  case JZNVAL_ORA_YEARMONTH_DUR:               \
    (_collationtyp) = JZNCOLLATION_YMINTERVAL; \
    break;                                     \
  case JZNVAL_ORA_DAYSECOND_DUR:               \
    (_collationtyp) = JZNCOLLATION_DSINTERVAL; \
    break;                                     \
  default:                                     \
    (_collationtyp) = JZNCOLLATION_UNKNOWN;    \
    break;                                     \
  }                                            \
} while(0)

/*
** Extended types
** - Standard JSON types + extended types in OSON
*/
typedef enum jznExtnType
{
  JZN_EXTNTYPE_UNKNOWN  = 0,
  JZN_EXTNTYPE_NULL,
  JZN_EXTNTYPE_OBJECT,
  JZN_EXTNTYPE_ARRAY,
  JZN_EXTNTYPE_STRING,
  JZN_EXTNTYPE_NUMBER,
  JZN_EXTNTYPE_BOOLEAN,
  JZN_EXTNTYPE_INTEGER,
  JZN_EXTNTYPE_BINARY,
  JZN_EXTNTYPE_FLOAT,
  JZN_EXTNTYPE_DOUBLE,
  JZN_EXTNTYPE_DATE,
  JZN_EXTNTYPE_TIMESTAMP,
  JZN_EXTNTYPE_TIMESTAMPTZ,
  JZN_EXTNTYPE_YMINTERVAL,
  JZN_EXTNTYPE_DSINTERVAL
} jznExtnType;

/* Timestamp type
** DESCRIPTION:
**   This serves as a helper structure for the types JZNVAL_ORA_DATE,
**   JZNVAL_ORA_TIMESTAMP and JZNVAL_ORA_TIMESTAMPTZ.
*/
typedef struct JsonDateTime
{
  sb2    year_JsonDateTime;
  ub1    month_JsonDateTime;
  ub1    day_JsonDateTime;
  ub1    hour_JsonDateTime;
  ub1    minute_JsonDateTime;
  ub1    second_JsonDateTime;
  ub1    flags_JsonDateTime;
  ub4    fsecond_JsonDateTime;
  sb1    tzHourOffset_JsonDateTime;
  sb1    tzMinuteOffset_JsonDateTime;
  ub2    unused_JsonDateTime;
} JsonDateTime;

/*
** Two bits of the flags are reserved for encoding subtype information.
** By convention, the less-precise types will have 0 values for the
** more precise fields in the structure that they don't use. TSTZ
** (all fields present) is the default.
*/
#define JZN_DATETIME_FLAG_SUBTYPE   0x03  /* Bit mask                  */
#define JZN_DATETIME_FLAG_DATE_ONLY 0x01  /* ANSI date without time   */
#define JZN_DATETIME_FLAG_DATE_TIME 0x02  /* Oracle date with time    */
#define JZN_DATETIME_FLAG_TIMESTAMP 0x03  /* SQL timestamp            */
#define JZN_DATETIME_FLAG_TSTZ      0x00  /* Timestamp with time zone */

/* Interval type - DAY TO SECOND
** DESCRIPTION:
**   This serves as a helper structure for type JZNVAL_ORA_DAYSECOND_DUR
*/
typedef struct JsonDayInterval
{
  sb4  days_JsonDayInterval;
  sb4  hours_JsonDayInterval;
  sb4  minutes_JsonDayInterval;
  sb4  seconds_JsonDayInterval;
  sb4  fseconds_JsonDayInterval;
} JsonDayInterval;

/* Interval type - YEAR TO MONTH
** DESCRIPTION:
**   This serves as a helper structure for type JZNVAL_ORA_YEARMONTH_DUR
*/
typedef struct JsonYearInterval
{
  sb4  years_JsonYearInterval;
  sb4  months_JsonYearInterval;
} JsonYearInterval; 

/*
** Union type for representing all supported scalar events, fields, items.
** This type is used in event records as well as in DOM interfaces.
*/
typedef union jzn_sval_union
{
    struct name_portion  /* Name information (if a field name event) */
    {
      oratext *name;
      ub4      namelen;
      ub1      dummy1;
      ub1      dummy2;
      ub1      dummy3;
      ub1      flags;
    } nam;
    struct string_value  /* JSON native values (as strings) */
    {
      oratext *value;
      ub4      valuelen;
      ub1      dummy1;
      ub1      dummy2;
      ub1      dummy3;
      ub1      flags;
    } val;
    struct container_portion  /* Containers (arrays/objects) */
    {
      size_t   position;   /* Byte offset of start/end */
      ub4      old_pos;    /* Legacy byte offset */
    } cont;
    struct binary_value  /* Value information (if binary) */
    {
      ub1     *value;
      ub4      valuelen;
#define JZN_BSON_BIN_SUBTYPES
      // ### Drop this oddball construct from BSON ?
      ub1      subtype;  /* Binary subtype */
      ub1      dummy2;
      ub1      dummy3;
      ub1      flags;
    } bval;
    union integer_value /* Value information (if signed long/int) */
    {
      sb8      longval;  /* If a 64-bit value */
      sb4      intval;   /* If a 32-bit value */
    } ival;
    union unsigned_value /* Value information (if unsigned long/int) */
    {
      ub8      longval;  /* If a 64-bit value */
      ub4      intval;   /* If a 32-bit value */
    } uval;
    struct float_value   /* Value information (for float) */
    {
      float    ftval;    /* native machine float value */
    } fval;
    struct double_value  /* Value information (for double) */
    {
      double   fpval;    /* native machine double value */
    } dval;
    struct time_value    /* Value information (if UTC or TSTAMP) */
    {
      JsonDateTime dtval;
    } tval;
    struct inline_value  /* Used for in-line Oracle Numbers/Dates/Timestamps */
    {
      /*
      ** To save memory for in-memory DOMs, many Oracle types can be stored
      ** in-line in the DOM node. All the temporal types can be stored
      ** in-line. Not all Oracle Numbers will fit, unfortunately, but all
      ** numbers that represent integers and long integers should fit.
      ** Most other numbers should too (up to 28 digits of precision).
      ** ### To-do: should we restore the in-line canonical representations
      ** ### for double/float?
      */
      ub1 val[JZN_INLINE_MAX_LEN];
      /* The length is last so that the data is aligned */
      ub1 len;
      /*
      ** This also places <len> in an empty space shared by all
      ** the out-of-line types so we can flag it if necessary.
      */
#define JZN_INLINE_FLAG      0x80  /* value is in-lined       */
#define JZN_INLINE_LEN_MASK  0x0F  /* length in bottom 4 bits */
    } inlineval;
    struct ptr_value     /* Value information (if UUID or OID) */
    {
      ub1      oidval[JZN_BSON_UUID_LEN];
    } pval;
    struct dec_value     /* Decimal 128 */
    {
      ub1      mem128[JZN_BSON_DEC128_LEN];  /* memcmp-friendly decimal128 */
    } xval;
    struct obuf_value    /* Oracle native data types */
    {
/* For all Oracle built-in types:
 *   JZNVAL_ORA_NUMBER JZNVAL_ORA_DATE JZNVAL_ORA_TIMESTAMP = 18
 *   JZNVAL_ORA_TIMESTAMPTZ JZNVAL_ORA_YEARMONTH_DUR JZNVAL_ORA_DAYSECOND_DUR
 * implemented by Oracle libcore library.
 * The maximum size is 22 bytes.
 * Since the underlying bin value can be longer than 8 bytes, we set the
 * obinptr_obinval as pointer into the underlying OSON byte array.
 *
 * max(LNXM_NUM =22,  LDITZSTAMPARRAYLEN = 13, LDIYEARMONTHARRAYLEN =5, 
 *   LDIDAYSECONDARRAYLEN =11)
 * So we define JZN_MAX_ORABIN_TYP_SZ as 22.
 *    #define JZN_MAX_ORABIN_TYP_SZ 22
 *    ub1  oibintypbuf[JZN_MAX_ORABIN_TYP_SZ];
 */
      ub1     *obinptr_obinval;
      ub4      oblen_obinval;
      ub1      dummy1;
      ub1      dummy2;
      ub1      dummy3;
      ub1      flags;
    } obinval;
    struct dom_node
    {
      void *doc;
      void *node;
    } domnode;
    /*
    ** This is a carrier type for a scalar whose out-of-line value is
    ** already stored in DOM memory. The actual type is in <dtype>.
    */
    struct opaque_value
    {
      void    *data;
      ub4      datalen;
      ub1      subtype;
      ub1      dtype;
      ub1      dummy3;
      ub1      flags;
    } opaque;
    /*
    ** ### Other types not currently supported. The odd Mongo 2-part
    ** ### "timestamp" type is currently returned as a string.
    */
} jzn_sval_union;

/* Auxiliary Union of helper structures */
typedef union JsonOCIVal
{
  JsonDateTime      dt_JsonOCIVal;
  JsonDayInterval   dayInv_JsonOCIVal;
  JsonYearInterval  yrInv_JsonOCIVal;
  ub1               num_JsonOCIVal[JZN_ORA_NUM_MAX_LEN];
} JsonOCIVal;

/*
** ### This shouldn't be exposed, but it's used in the exposed OSON/IMC
** ### internal structures below. Shouldn't those be encapsulated?
*/
typedef struct jznDomHashTable jznDomHashTable;

/*
** Specifies a step location along a JSON path;
** the step must be "simple", a field name or array position.
*/
typedef struct JsonStepLocation
{
  oratext    *fieldName;
  union
  {
    ub4       nameLength;
    ub4       arrayPosition;
  } u;
  ub2         info;
  ub2         flags;

/* Leave the low 8 bits for bind variable flags */
#define JZN_STEP_NONE               0x0000
#define JZN_STEP_NESTED             0x0100    /* BOV nested link  */
#define JZN_STEP_IN_PROGRESS        0x0200    /* Step in progress */
#define JZN_STEP_OBJECT             0x0400    /* Object step      */
#define JZN_STEP_ARRAY              0x0800    /* Array step       */

} JsonStepLocation;

typedef struct jznDomSortPath
{
  JsonStepLocation *steps;
  ub4               nsteps;
  boolean           descending;
} jznDomSortPath;

typedef struct jznuHashCtx
{
  ub8    v[4];
  size_t total;
  size_t nbytes;
  ub1    exbytes[8];
} jznuHashCtx;

/*
** Data structure to carry SQL column scalar properties
*/
#define JZNPROP_NO_SCALE (sb1)MAXSB1MINVAL

typedef struct jznScalarProperties
{
  ub2 maxlen_jznScalarProperties;
  ub1 datatype_jznScalarProperties;
  ub1 precision_jznScalarProperties;
  sb1 scale_jznScalarProperties;
  /*
  ** Timestamp types:
  **   precision is the number of fractional seconds, scale is 0
  ** Number types:
  **   unchecked: precision = 0, scale == JZNPROP_NO_SCALE  NUMBER
  **   bounded:   precision > 0, scale != JZNPROP_NO_SCALE  NUMBER(prec,scl)
  **   unbounded: precision = 0, scale != JZNPROP_NO_SCALE  NUMBER(*,scl)
  **   binary:    precision > 0, scale == JZNPROP_NO_SCALE  FLOAT(prec)
  */
  ub1 fbits_jznScalarProperties;
  ub1 csform_jznScalarProperties;
  ub1 xinfo_jznScalarProperties;
} jznScalarProperties;

/* DTYVEC vector type meta-data */
typedef struct jznVectorProperties
{
  /*
  ** qctyvec struct defined in opndef3.h
  ** (stort_qctyvec, qctyvec, ub1)  -  vector element storage type
  ** (flg2_qctyvec,qctyvec, ub1)    -  auxspare2 flags for vector datatype
  ** (dim_qctyvec, qctyvec, ub4)    -  vector dimension
  **/
  ub4 vec_dim_jznVectorProperties;   /* vector dimension */
  ub1 vec_stort_jznVectorProperties; /* vector element storage type */
  ub1 vec_flg2_jznVectorProperties;  /* vector storage flag (flg2) */

/*
** vector flg2_qctyvec defined in opndef3.h for reference here
*/
#define JZNBOV_VECFLG_FLEX_DIM 0x1  /* QCTYVEC_FLEX_DIM */
                                    /* vector dimesion flexbile */

/*
** vector element storage type value defined in qvcg.h for reference here
*/
#define JZNBOV_VEC_FLEX  0 /* QVCG_FLEX  - storage type is flexible */
#define JZNBOV_VEC_FLT16 1 /* QVGC_FLT16 - float16 */
#define JZNBOV_VEC_FLT32 2 /* QVGC_FLT32 - float32 */
#define JZNBOV_VEC_FLT64 3 /* QVGC_FLT64 - float64 */
#define JZNBOV_VEC_INT8  4 /* QVGC_INT8  - int8 */

} jznVectorProperties;

#endif                                              /* JZNTYPES_ORACLE */
