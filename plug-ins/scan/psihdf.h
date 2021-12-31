/* Gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 * 
 * Copyright (C) 1999,2000,2001,2002,2003 Percy Zahl
 *
 * Authors: Percy Zahl <zahl@users.sf.net>
 * additional features: Andreas Klust <klust@users.sf.net>
 * WWW Home: http://gxsm.sf.net
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */

/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */

// assumed by AK
#define MAX_SOURCE_NAME  32
#define MAX_MODE_NAME    8
#define MAX_UNIT_STRING  8

// defined by AK
#define PSI_HEADER_OFFSET 0x03A0


// renamed tagHEADER to psiHEADER and HEADER to PsiHEADER
// to avoid conflict with tagHEADER defined
// for class DatFile in ../gnu/batch.h

typedef   struct    psiHEADER
{                   
  //{{ Do not modify the codes in this block.
  short          nHardware;                      //   XL, etc.
  short          nSource;                        //   Data source. TOPO, ERROR, LFM, etc.
  char           szSourceName[MAX_SOURCE_NAME];
  char           szImageMode[MAX_MODE_NAME];     //   AFM, LFM, NCM and MFM etc.    
  short          bFastScanDir;                   //   non-zero for forward, 0 for backward.
  short          bSlowScanDir;                   //   non-zero when scan up.
  short          bXYSwap;                        //   Swap fast-slow scanning dirctions.
  short          nCols, nRows;                   //   Image size. 
  //DWORD           dwFilter;           
  //   Bitwise OR of filter functions applied.
  // This flag is not yet well defined.
  short          dummy, dummy2;                  //   dummy by AK
  short          fLowPass;                       //   LowPass filter strength.
  short          b3rdOrder;                      //   3rd order fit applied.
  short          bAutoFlatten;                   //   Automatic flatten after imaging.
  float          fXScanSize, fYScanSize;         //   Scan size in um.
  float          fXOffset, fYOffset;             //   x,y offset in microns.
  float          fFastAngle, fSlowAngle;         //   Angle of Fast and slow direction about
                                                 //   positive x-axis.   
  float          fScanRate;                      //   Scan speed in rows per second. 
  float          fSetPoint;                      //   Error signal set point.
  char           szSetPointUnit[MAX_UNIT_STRING];
  float          fTipBiasVoltage;
  float          fSampleBiasVoltage; 
  float          fDataGain;                      //   um per data step.
  char           szUnit[MAX_UNIT_STRING];
  short          nDataMin;                       //   the z-range last used.
  short          nDataMax;
  //   ----03-02-95----
  float          fXSlope;                        // Slope correction amount in X
  float          fYSlope;                        // Slope correction amount in Y
  //  ----04-04-96----
  //}}
}    PsiHEADER;

  
  
  
//===========================================================================
//
//  PSIHDF.H
//
//      The contents of this file are extracted from original NCSA HDF ver 3.3
//        HDF.H and HDFI.H files. Only the appropriate information are extracted.
//        And some custom informations have been added.
//
//===========================================================================
//  Members:
//
//===========================================================================
//  08/31/94    jLee created
//===========================================================================

#ifndef   PSIHDF_H
#define   PSIHDF_H
  
/*--------------------------------------------------------------------------*/
/*                              MT/NT constants                             */
/*  four MT nibbles represent double, float, int, uchar (from most          */
/*  significant to least significant).                                      */
/*  The values for each nibble are:                                         */
/*      1 - Big Endian                                                      */
/*      2 - VAX                                                             */
/*      3 - Cray                                                            */
/*      4 - Little Endian                                                   */
/*      5 - Convex                                                          */
/*      6 - Fijitsu VP                                                      */
/*--------------------------------------------------------------------------*/
#define     DFMT_SUN            0x1111
#define     DFMT_ALLIANT        0x1111
#define     DFMT_IRIS4          0x1111
#define     DFMT_APOLLO         0x1111
#define     DFMT_IBM6000        0x1111
#define     DFMT_HP9000         0x1111
#define     DFMT_CONVEXNATIVE   0x5511
#define     DFMT_CONVEX         0x1111
#define     DFMT_UNICOS         0x3331
#define     DFMT_CTSS           0x3331
#define     DFMT_VAX            0x2221
#define     DFMT_MIPSEL         0x4441
#define     DFMT_PC             0x4441
#define     DFMT_MAC            0x1111
#define     DFMT_SUN386         0x4441
#define     DFMT_NEXT           0x1111
#define     DFMT_MOTOROLA       0x1111
#define     DFMT_ALPHA          0x4441
#define     DFMT_VP             0x6611

//typedef int                        bool;
typedef char                       char8;
typedef unsigned char              uchar8;
typedef char                       int8;
typedef unsigned char              uint8;
typedef short int                  int16;
typedef unsigned short int         uint16;
typedef long int                   int32;
typedef unsigned long int          uint32;
typedef int                        intn;
typedef unsigned int               uintn;
typedef float                      float32;
typedef double                     float64;
typedef long                       intf;

  
#define DFNT_HDF      0x00000000    // standard HDF format
#define DFNT_NATIVE   0x00001000    // native format
#define DFNT_CUSTOM   0x00002000    // custom format
#define DFNT_LITEND   0x00004000    // Little Endian format
  
//   type info codes
#define DFNT_NONE        0    //   indicates that number type not set
#define DFNT_QUERY       0    //   use this code to find the current type
#define DFNT_VERSION     1    //   current version of NT info
  
#define DFNT_FLOAT32     5
#define DFNT_FLOAT       5    //  For backward compat; don't use
#define DFNT_FLOAT64     6
#define DFNT_DOUBLE      6    //  For backward compat; don't use
#define DFNT_FLOAT128    7    //  No current plans for support
  
#define DFNT_INT8       20
#define DFNT_UINT8      21

#define DFNT_INT16      22
#define DFNT_UINT16     23
#define DFNT_INT32      24
#define DFNT_UINT32     25
#define DFNT_INT64      26
#define DFNT_UINT64     27
#define DFNT_INT128     28    //  No current plans for support
#define DFNT_UINT128    30    //  No current plans for support
  
#define DFNT_UCHAR8     3 //  3 chosen for backward compatibility
#define DFNT_UCHAR      3     //   uchar=uchar8 for backward combatibility
#define DFNT_CHAR8      4     //   4 chosen for backward compatibility
#define DFNT_CHAR       4     //   uchar=uchar8 for backward combatibility
#define DFNT_CHAR16     42  //  No current plans for support
#define DFNT_UCHAR16    43  //  No current plans for support
  
//   class info codes for int
#define        DFNTI_MBO       1   //   Motorola byte order 2's compl
#define DFNTI_VBO       2   //  Vax byte order 2's compl
#define DFNTI_IBO       4   //  Intel byte order 2's compl

//   class info codes for float
#define DFNTF_NONE      0   //     indicates subclass is not set
#define DFNTF_HDFDEFAULT 1    //  hdf default float format is ieee
#define DFNTF_IEEE      1   //     IEEE format
#define DFNTF_VAX       2   //     Vax format
#define DFNTF_CRAY      3   //     Cray format
#define DFNTF_PC        4   //     PC floats - flipped IEEE
#define DFNTF_CONVEX    5   // CONVEX native format
#define DFNTF_VP        6   //     Fujitsu VP native format
  
//   class info codes for char
#define DFNTC_BYTE      0 //  bitwise/numeric field
#define DFNTC_ASCII     1   //     ASCII
#define DFNTC_EBCDIC    5   //  EBCDIC
  
//   array order
#define DFO_FORTRAN     1 //  column major order
#define DFO_C           2   //     row major order

//--------------------------------------------------------------------
//   Sizes of number types
//--------------------------------------------------------------------

//   first the standard sizes of number types
  
#define SIZE_FLOAT32    4
#define SIZE_FLOAT64    8
#define SIZE_FLOAT128  16 //  No current plans for support

#define SIZE_INT8       1
#define SIZE_UINT8      1
#define SIZE_INT16      2
#define SIZE_UINT16     2
#define SIZE_INT32      4
#define SIZE_UINT32     4
#define SIZE_INT64      8
#define SIZE_UINT64     8
#define SIZE_INT128    16   //     No current plans for support
#define SIZE_UINT128   16   //     No current plans for support

#define SIZE_CHAR8      1
#define SIZE_CHAR       1   //     For backward compat char8 == char
#define SIZE_UCHAR8     1
#define SIZE_UCHAR      1   //     For backward compat uchar8 == uchar
#define SIZE_CHAR16     2   //     No current plans for support
#define SIZE_UCHAR16    2   //     No current plans for support

//   tags and refs
#define DFREF_WILDCARD 0
#define DFTAG_WILDCARD 0
#define DFTAG_NULL 1
#define DFTAG_LINKED 20       //   check uniqueness
#define DFTAG_VERSION 30

//   utility set
#define DFTAG_FID   ((uint16)100) //    File identifier
#define DFTAG_FD    ((uint16)101)  //   File description
#define DFTAG_TID   ((uint16)102)  //   Tag identifier
#define DFTAG_TD    ((uint16)103)  //   Tag descriptor
#define DFTAG_DIL   ((uint16)104)  //   data identifier label
#define DFTAG_DIA   ((uint16)105)  //   data identifier annotation
#define DFTAG_NT    ((uint16)106)  //   number type
#define DFTAG_MT    ((uint16)107)  //   machine type
  
//   raster-8 set
#define DFTAG_ID8   ((uint16)200)  //   8-bit Image dimension
#define DFTAG_IP8   ((uint16)201)  //   8-bit Image palette
#define DFTAG_RI8   ((uint16)202)  //   Raster-8 image
#define DFTAG_CI8   ((uint16)203)  //   RLE compressed 8-bit image
#define DFTAG_II8   ((uint16)204)  //   IMCOMP compressed 8-bit image
  
//   Raster Image set
#define DFTAG_ID    ((uint16)300)  //   Image DimRec
#define DFTAG_LUT   ((uint16)301)  //   Image Palette
#define DFTAG_RI    ((uint16)302)  //   Raster Image
#define DFTAG_CI    ((uint16)303)  //   Compressed Image
  
#define DFTAG_RIG   ((uint16)306)  //   Raster Image Group
#define DFTAG_LD    ((uint16)307)  //   Palette DimRec
#define DFTAG_MD    ((uint16)308)  //   Matte DimRec
#define DFTAG_MA    ((uint16)309)  //   Matte Data
#define DFTAG_CCN   ((uint16)310)  //   color correction
#define DFTAG_CFM   ((uint16)311)  //   color format
#define DFTAG_AR    ((uint16)312)  //   aspect ratio
  
#define DFTAG_DRAW  ((uint16)400)  //   Draw these images in sequence
#define DFTAG_RUN   ((uint16)401)  //   run this as a program/script

#define DFTAG_XYP   ((uint16)500)  //   x-y position
#define DFTAG_MTO   ((uint16)501)  //   machine-type override

//   Tektronix
#define DFTAG_T14   ((uint16)602)  //   TEK 4014 data
#define DFTAG_T105  ((uint16)603)  //   TEK 4105 data

//   Scientific Data set
#define DFTAG_SDG   ((uint16)700)  //   Scientific Data Group
#define DFTAG_SDD   ((uint16)701)  //   Scientific Data DimRec
#define DFTAG_SD    ((uint16)702)  //   Scientific Data
#define DFTAG_SDS   ((uint16)703)  //   Scales
#define DFTAG_SDL   ((uint16)704)  //   Labels
#define DFTAG_SDU   ((uint16)705)  //   Units
#define DFTAG_SDF   ((uint16)706)  //   Formats
#define DFTAG_SDM   ((uint16)707)  //   Max/Min
#define DFTAG_SDC   ((uint16)708)  //   Coord sys
#define DFTAG_SDT   ((uint16)709)  //   Transpose
#define DFTAG_SDLNK ((uint16)710)  //   Links related to the dataset
#define DFTAG_NDG   ((uint16)720)  //   Numeric Data Group
#define DFTAG_CAL   ((uint16)731)  //   Calibration information
#define DFTAG_FV    ((uint16)732)  //   Fill Value information
#define DFTAG_BREQ  ((uint16)799)  //   Beginning of required tags
#define DFTAG_EREQ  ((uint16)780)  //   Current end of the range
  
//   VSets
#define DFTAG_VG     ((uint16)1965) //  Vgroup
#define DFTAG_VH     ((uint16)1962) //  Vdata Header
#define DFTAG_VS     ((uint16)1963) //  Vdata Storage
  
//   compression schemes
#define DFTAG_RLE   ((uint16)11)    //  run length encoding
#define DFTAG_IMC   ((uint16)12)    //  IMCOMP compression alias
#define DFTAG_IMCOMP ((uint16)12)   //  IMCOMP compression
#define DFTAG_JPEG  ((uint16)13)    //  JPEG compression (24-bit data)
#define DFTAG_GREYJPEG  ((uint16)14)//  JPEG compression (8-bit data)
  
//   Interlace schemes
#define DFIL_PIXEL   0             //   Pixel Interlacing
#define DFIL_LINE    1             //   Scan Line Interlacing
#define DFIL_PLANE   2             //   Scan Plane Interlacing
  
//   SPECIAL CODES
#define SPECIAL_LINKED 1
#define SPECIAL_EXT 2
  
//   PARAMETERS
  
#define DF_MAXFNLEN 256
  
//---------------------------------------------------------------------
//
//   PSI stuffs
//
//----------------------------------------------------------------------

// changed by AK (reverted byte order)
#define        HDF_MAGIC_NUM ((uint32) 0x0113030E )
  
//   Here are our own custom tags
#define        PSITAG_VER     ((uint16)0x8001)    //  Version information
//   | PSITAG_VER | ref_no |[offset:length]|
//                                 +--->|version|
//   ref_no : 16bit reference number
//   version : 32bit integer number, which is composed of
//        major  : MSB 8bit major version number
//        minor  : next 8bit minor version number
//        rev    : LSB 16bit revision number
//   example ver = 4.01.17
//        version = 0x04010011
//        
  
//{{ ObsoleteTags_Begin

#define        PSITAG_ROT     ((uint16)0x8002)    // (x,y) offset, scan directions,
                                                  //   ,rotating angle of scanning area.
//   | PSITAG_POS | ref_no |[offset:length]|
//                                 +--->|Rotation|
//   ref_no : 16bit reference number
//   Rotation : |(float32)XOrigin|(float32)YOrigin|
//               |(byte) FastDir|(byte) SlowDir|
//                |(float32)fFastAxisAngle|(float32)fSlowAxisAngle|
//                  ,FastDir=1 for Forward Scan.
//                  ,SlowDir=1 for Scan Up.
  
#define        PSITAG_HDW     ((uint16)0x8003)    //  Hardware description
//   | PSITAG_HDW | ref_no |[offset:length]|
//                                 +--->|hardware|
//   ref_no : 16bit reference number
//   hardware : null-teminated text string.
  
#define        PSITAG_SRC     ((uint16)0x8004)    // Data source name
//   | PSITAG_SRC | ref_no |[offset:length]|
//                                 +--->|source|
//   ref_no : 16bit reference number
//   source : null-teminated text string.
  
#define        PSITAG_SPD     ((uint16)0x8005)    //  Scan speed
//   | PSITAG_SPD | ref_no |[offset:length]|
//                                 +--->|Speed|
//   ref_no : 16bit reference number
//   Speed : |(float32)fScanRate| Speed Unit |
//             ,SpeedUnit = null terminated string.
  
#define        PSITAG_SET     ((uint16)0x8006)    //  Z servo set point.
//   | PSITAG_SET | ref_no |[offset:length]|
//                                 +--->|SetPoint|
//   ref_no : 16bit reference number
//   SetPoint :     |(float32)fScanRate| Set Point |
//                  ,SpeedUnit = null terminated string.

//}} ObsolteTag_End
  
#define        PSITAG_HD ((uint16)0x8009)    //  Header block
//   | PSITAG_HD | ref_no |[offset:length]|
//                                 +--->|Header|
//   ref_no : 16bit reference number
//   Header : 
//        Binary header block for this particular version. This header
//        block is a direct dump of the C structure. Hence, the structure
//        is version-dependent. However, this structure is only growing
//        in size to keep old information. Always check PSITAG_VER to get
//        the correct version of writer. Do not assume your sizeof( 
//        Header)   gives you correct size. Always check the length of the
//        tag data.
  
#define        PSITAG_SPC     ((uint16)0x800A)    //  1-D Spectroscopy data
//   | PSITAG_SPEC | ref_no |[offset:length]|
//                                 +--->|Line Spectroscopy|
//   ref_no : 16bit reference number, when multiple curves are saved, 
//              this number will be used as index.
//   Line Spectroscopy :
//        General 1-D spectroscopy data ( e.g, I-V , F-D etc ) block for
//        this particular version. This block is a direct dump of the C 
//        structure and binary array. Hence, the content is version
//        -dependent. However, this structure is only growing in size to
//        keep old information. Always check PSITAG_VER to get the correct
//        version of writer. Do not assume your sizeof(...) will give you
            //        correct size. Always check the length of the tag data.
  
#define        PSITAG_LA ((uint16)0x800B)    //  Line analysis data
//   | PSITAG_LA | ref_no |[offset:length]|
//                                 +--->|Line analysis table|
//   ref_no : 16bit reference number.
//   Line analysis table :
//        The block is a direct dump of the C structure and binary array.
//        Hence, the content may be version dependent.
//        The length of the tag data is fixed size of 16384 bytes for 
//        v1.1.
                 
#define        PSITAG_RA ((uint16)0x800C)    //  Region analysis data
//   | PSITAG_RA | ref_no |[offset:length]|
//                                 +--->|Line analysis table|
//   ref_no : 16bit reference number.
//   Region analysis table :
//        The block is a direct dump of the C structure and binary array.
//        Hence, the content may be version dependent.
//        The length of the tag data is fixed size of 32767 bytes for 
//        v1.1.
                 
typedef   struct    {
  uint16    wTag;
  uint16    wRef;
  uint32    dwOffset;
  uint32    dwLength;
}    DATADESC;

//   This is the number for all HDF header info before actual binary
//   data block begines.
#define        FILE_HEADER_SIZE 16384
//#define FILE_TEXT_HEADER 8192 // Readable text header info starts here.        

#define        HDF_NUM_TAGS   64        //   Let's start with 64, which is reasonably
                                        //   bigger than we need.
#define        HDF_LEN_TID    128       //   Custome Tag ID string length.
#define        HDF_LEN_UNITS  64        //   Unit string length.
#define        HDF_LEN_HARDWARE 64      //   Hardware string length.
#define        HDF_LEN_SOURCE      64   //   Data source
#define        HDF_LEN_SPEED  64        //   Scan speed and unit string.
#define        HDF_LEN_SET       64     //   Z servo set point and unit string.
#define        HDF_LEN_TITLE     256    //   File title
#define        HDF_LEN_REPLICA     4096 //   Text description of each records.
#define        HDF_LEN_COMMENTS 2048    //   Comments.
  
#define   NT_REF_Z            ((uint16)256)  //   DFTAG_NT ref_no for ZData.
#define   NT_REF_X            ((uint16)257)  //   DFTAG_NT ref_no for X-scale.
#define   NT_REF_Y            ((uint16)258)  //   DFTAG_NT ref_no for Y-scale.
  
#define   NDG_REF_NUM              2         // NDG group RefNum. 
       
//===========================================================================
//   
//   Physical map of sample image file
//
//===========================================================================

/*
#ifdef    NOT_PROGRAM_CODE_NEVER_BE_COMPILED
  
              
0    |0x0E|0x03|0x13|0x01|    // Magic number
  
//   |(uint16)NumDDs          |(uint32) Offset to Next 
     |   
4    | HDF_NUM_TAGS  | 0                          
     |
  
//     |TAG      |RefNum               |(uint32)Offset  |(uint32)Length          |
  10   |DFTAG_MT           |DFMT_PC    |0               |0                  
       |
  22   |DFTAG_VERSION|1    |<HDFVersion>  |12                      |
  34   |DFTAG_TID          |PSITAG_VER         |<VersionTID>       |strlen()      
                 |
       |DFTAG_TID          |PSITAG_HD          |<HeaderTID>   |strlen()           |
       
       |PSITAG_VER              |1          |<PsiVer>    |4                  |
       |DFTAG_NT                |NT_REF_Z   |<ZNumType>  |4                  |
       |DFTAG_NT |NT_REF_X   |<XNumType>       |4                  |
       |DFTAG_NT                |NT_REF_Y   |<YNumType>  |4                  |
       |PSITAG_HD     |2                  |<Header>                |sizeof( Header )
       |
  
       |DFTAG_SDD               |2          |<Dimension> |22                 |    
       |DFTAG_SD                |2          |<Data>           |nNumCol*nNumRows*2 |
       |DFTAG_SDL     |2                  |<Labels>           |strlen()    
       |
       |DFTAG_SDU               |2          |<Units>          |strlen()         |
       |DFTAG_SDM               |2          |<MaxMin>    
       |8                       |
       |DFTAG_SDC               |2          |<Coord>          |strlen()           |
       |DFTAG_CAL                    |2          |<Calibration>    |36                 |
       |DFTAG_NDG               |2          |<Group>     |4 * 8              |
       |DFTAG_FID               |1          |<Title>          |strlen()           |
       |DFTAG_FD |1          |<Comments>            |strlen()           |
       |DFTAG_NULL         |0          |0                
       |0                  |
       |DFTAG_NULL         |0          |0           
       |0                  |
       |DFTAG_NULL    |0             |0                  |0        
                 |
  
//   -------------------------------------------------------------
  
  <HDFVersion>
  |major_ver|minor_ver|release| NCSA HDF Version 3.2 compatible"
  
  <VersionTID>
   PSI TAG for ProScan Version"
  
  <HeaderTID>
   PSI TAG for binary header block"
  
  <PsiVer>
  |0x04|0x01|0x0001|  //   version 04.01.0001
  
  <ZNumType>
  |DFNT_VERSION|DFNT_INT16|0x10|DFNTI_IBO|
  
  <XNumType>
  |DFNT_VERSION|DFNT_FLOAT32|0x20|DFNTF_PC|
  
  <YNumType>
  |DFNT_VERSION|DFNT_FLOAT32|0x20|DFNTF_PC|
  
  <Header>
  Direct C-structure memory dump.
  
  <Dimension>
  |0x0002|
  |(uint32)nNumCols|(uint32)nNumRows|
  |DFTAG_NT|NT_REF_Z|DFTAG_NT|NT_REF_X|DFTAG_NT|NT_REF_Y|
  
  <Labels>
   topo\0x-axis\0y-axis\0"
  
  <Units>
   um\0um\0um"
  
  <MaxMin>
  |(int16)nMax|(int16)nMin|0x0000|0x0000|
  
  <Coord>
   Cartesian coordinate system"
  
  <Calibration>
  |(float64)dfCal|(float64)dfCalError|(float64)dfOffset|(float64)dfOffsetError|
  |DFNT_FLOAT32|
  
  <Group>
  |PSITAG_HD |0x0002|DFTAG_SDD |0x0002|DFTAG_SD  |0x0002|DFTAG_SDL |0x0002|
  |DFTAG_SDU |0x0002|DFTAG_SDM |0x0002|DFTAG_SDC |0x0002|DFTAG_CAL |0x0002|
  
  <Title>
   Sample HDF file"
  
  <Comments>
   User comments"
  
  <Data>
  ....

#endif  //  NOT_PROGRAM_CODE_NEVER_BE_COMPILED

*/
 
#endif  // PSIHDF_H
