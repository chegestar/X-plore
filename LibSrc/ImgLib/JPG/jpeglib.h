/*
 * jpeglib.h
 *
 * Copyright (C) 1991-1996, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file defines the application interface for the JPEG library.
 * Most applications using the library need only include this file,
 * and perhaps jerror.h if they want to know the exact error codes.
 */

#ifndef JPEGLIB_H
#define JPEGLIB_H

struct jpeg_decompress_struct;
/*
 * Define BITS_IN_JSAMPLE as either
 *   8   for 8-bit sample values (the usual setting)
 *   12  for 12-bit sample values
 * Only 8 and 12 are legal data precisions for lossy JPEG according to the
 * JPEG standard, and the IJG code does not support anything else!
 * We do not support run-time selection of data precision, sorry.
 */
#define BITS_IN_JSAMPLE  8 /* use 8 or 12 */


/*
 * Maximum number of components (color channels) allowed in JPEG image.
 * To meet the letter of the JPEG spec, set this to 255.  However, darn
 * few applications need more than 4 channels (maybe 5 for CMYK + alpha
 * mask).  We recommend 10 as a reasonable compromise; use 4 if you are
 * really short on memory.  (Each allowed component costs a hundred or so
 * bytes of storage, whether actually used in an image or not.)
 */
#define MAX_COMPONENTS  4 /* maximum number of image components */

/* Representation of a single sample (pixel element value).
 * We frequently allocate large arrays of these, so it's important to keep
 * them small.  But if you have memory to burn and access to char or short
 * arrays is very slow on your hardware, you might want to change these.
 */
/* JSAMPLE should be the smallest type that will hold the values 0..255.
 * You can use a signed char by having GETJSAMPLE mask it with 0xFF.
 */
typedef byte JSAMPLE;
#define GETJSAMPLE(value)  ((int) (value))

#define MAXJSAMPLE   255
#define CENTERJSAMPLE   128

/* Representation of a DCT frequency coefficient.
 * This should be a signed value of at least 16 bits; "short" is usually OK.
 * Again, we allocate large arrays of these, but you can change to int
 * if you have memory to burn and "short" is really slow.
 */

typedef short JCOEF;


/* Compressed datastreams are represented as arrays of JOCTET.
 * These must be EXACTLY 8 bits wide, at least once they are written to
 * external storage.  Note that when using the stdio data source/destination
 * managers, this is also the data type passed to fread/fwrite.
 */

typedef byte JOCTET;
#define GETJOCTET(value)  (value)


/* These typedefs are used for various table entries and so forth.
 * They must be at least as wide as specified; but making them too big
 * won't cost a huge amount of memory, so we don't provide special
 * extraction code like we did for JSAMPLE.  (In other words, these
 * typedefs live at a different point on the speed/space tradeoff curve.)
 */

/* Datatype used for image dimensions.  The JPEG standard only supports
 * images up to 64K*64K due to 16-bit fields in SOF markers.  Therefore
 * "dword" is sufficient on all machines.  However, if you need to
 * handle larger images and you don't mind deviating from the spec, you
 * can change this datatype.
 */
typedef dword JDIMENSION;

#define JPEG_MAX_DIMENSION  65500L  /* a tad under 64K to prevent overflows */

/* These macros are used in all function definitions and extern declarations.
 * You could modify them if you need to change function linkage conventions;
 * in particular, you'll need to do that to make the library a Windows DLL.
 * Another application is to make all functions global for use with debuggers
 * or code profilers that require it.
 */

/* a reference to a GLOBAL function: */
#define EXTERN(type)    static type

#define JMETHOD(type,methodname,arglist)  type (*methodname) arglist


//#define DCT_ISLOW_SUPPORTED   /* slow but accurate integer algorithm */
#define DCT_IFAST_SUPPORTED   /* faster, less accurate integer method */


#define D_MULTISCAN_FILES_SUPPORTED /* Multiple-scan JPEG files? */
#define D_PROGRESSIVE_SUPPORTED      /* Progressive JPEG? (Requires MULTISCAN)*/
#define IDCT_SCALING_SUPPORTED       /* Output rescaling via IDCT? */



#define RGB_RED   2  /* Offset of Red in an RGB scanline element */
#define RGB_GREEN 1  /* Offset of Green */
#define RGB_BLUE  0  /* Offset of Blue */
                              //JSAMPLEs per RGB scanline element
#define RGB_PIXELSIZE 4       //since we use dword-aligned RGB struct, keep it 4-byte


/* Various constants determining the sizes of things.
 * All of these are specified by the JPEG standard, so don't change them
 * if you want to be compatible.
 */

#define DCTSIZE          8 /* The basic DCT block is 8x8 samples */
#define DCTSIZE2      64   /* DCTSIZE squared; # of elements in a block */
#define NUM_QUANT_TBLS      4 /* Quantization tables are numbered 0..3 */
#define NUM_HUFF_TBLS       4 /* Huffman tables are numbered 0..3 */
#define NUM_ARITH_TBLS      16   /* Arith-coding tables are numbered 0..15 */
#define MAX_COMPS_IN_SCAN   4 /* JPEG limit on # of components in one scan */
#define MAX_SAMP_FACTOR     4 /* JPEG limit on sampling factors */
/* Unfortunately, some bozo at Adobe saw no reason to be bound by the standard;
 * the PostScript DCT filter can emit files with many more than 10 blocks/MCU.
 * If you happen to run across such a file, you can up D_MAX_BLOCKS_IN_MCU
 * to handle it.  We even let you do this from the jconfig.h file.  However,
 * we strongly discourage changing C_MAX_BLOCKS_IN_MCU; just because Adobe
 * sometimes emits noncompliant files doesn't mean you should too.
 */
#ifndef D_MAX_BLOCKS_IN_MCU
#define D_MAX_BLOCKS_IN_MCU   10 /* decompressor's limit on blocks per MCU */
#endif


/* Data structures for images (arrays of samples and of DCT coefficients).
 * On 80x86 machines, the image arrays are too big for near pointers,
 * but the pointer arrays can fit in near memory.
 */

typedef JSAMPLE *JSAMPROW;   /* ptr to one image row of pixel samples. */
typedef JSAMPROW *JSAMPARRAY; /* ptr to some rows (a 2-D sample array) */
typedef JSAMPARRAY *JSAMPIMAGE;  /* a 3-D sample array: top index is color */

typedef JCOEF JBLOCK[DCTSIZE2];  /* one block of coefficients */
typedef JBLOCK *JBLOCKROW;   /* pointer to one row of coefficient blocks */
typedef JBLOCKROW *JBLOCKARRAY;     /* a 2-D array of coefficient blocks */

typedef JCOEF *JCOEFPTR;  /* useful in a couple of places */


/* Known color spaces. */

enum J_COLOR_SPACE{
   JCS_UNKNOWN,      /* error/unspecified */
   JCS_GRAYSCALE,    /* monochrome */
   JCS_RGB,    /* red/green/blue */
   JCS_YCbCr,     /* Y/Cb/Cr (also known as YUV) */
   JCS_CMYK,      /* C/M/Y/K */
   JCS_YCCK    /* Y/Cb/Cr/K */
};

/* DCT/IDCT algorithm options. */

enum J_DCT_METHOD{
   JDCT_ISLOW,    /* slow but accurate integer algorithm */
   JDCT_IFAST,    /* faster, less accurate integer method */
};

#ifndef JDCT_DEFAULT    /* may be overridden in jconfig.h */
#define JDCT_DEFAULT  JDCT_ISLOW
#endif
#ifndef JDCT_FASTEST    /* may be overridden in jconfig.h */
#define JDCT_FASTEST  JDCT_IFAST
#endif

//----------------------------

typedef struct jvirt_sarray_control * jvirt_sarray_ptr;
typedef struct jvirt_barray_control * jvirt_barray_ptr;

/* Decompression startup: read start of JPEG datastream to see what's there */
/* Return value is one of: */
#define JPEG_ERROR -1
#define JPEG_SUSPENDED     0 /* Suspended due to lack of input data */
#define JPEG_HEADER_OK     1 /* Found valid image datastream */
#define JPEG_HEADER_TABLES_ONLY  2 /* Found valid table-specs-only datastream */

/* Additional entry points for buffered-image mode. */
EXTERN(int) jpeg_consume_input (jpeg_decompress_struct* cinfo);
/* Return value is one of: */
/* #define JPEG_SUSPENDED  0    Suspended due to lack of input data */
#define JPEG_REACHED_SOS   1 /* Reached start of new scan */
#define JPEG_REACHED_EOI   2 /* Reached end of image */
#define JPEG_ROW_COMPLETED 3 /* Completed one iMCU row */
#define JPEG_SCAN_COMPLETED   4 /* Completed last iMCU row of a scan */

EXTERN(void) jpeg_abort (jpeg_decompress_struct* cinfo);

enum E_STATE{
   DSTATE_NULL,
   DSTATE_START   =200,   /* after create_decompress */
   DSTATE_INHEADER =  201,   /* reading header markers, no SOS yet */
   DSTATE_READY   =202  , /* found SOS, ready for start_decompress */
   DSTATE_PRELOAD   =203,   /* reading multiscan file in start_decompress*/
   DSTATE_PRESCAN   =204 ,  /* performing dummy pass for 2-pass quant */
   DSTATE_SCANNING   =205 ,  /* start_decompress done, read_scanlines OK */
};

/* We assume that right shift corresponds to signed division by 2 with
 * rounding towards minus infinity.  This is correct for typical "arithmetic
 * shift" instructions that shift in copies of the sign bit.  But some
 * C compilers implement >> with an unsigned shift.  For these machines you
 * must define RIGHT_SHIFT_IS_UNSIGNED.
 * RIGHT_SHIFT provides a proper signed right shift of an int quantity.
 * It is only applied with constant shift counts.  SHIFT_TEMPS must be
 * included in the variables of any routine using RIGHT_SHIFT.
 */

#define RIGHT_SHIFT(x,shft)   ((x) >> (shft))

/* Decompression module initialization routines */
EXTERN(bool) jinit_phuff_decoder (jpeg_decompress_struct* cinfo);
EXTERN(bool) jinit_upsampler (jpeg_decompress_struct* cinfo);

/* Utility routines in jutils.c */
EXTERN(long) jdiv_round_up (long a, long b);
EXTERN(long) jround_up (long a, long b);
EXTERN(void) jcopy_sample_rows (JSAMPARRAY input_array, int source_row, JSAMPARRAY output_array, int dest_row, int num_rows, JDIMENSION num_cols);

#endif /* JPEGLIB_H */
