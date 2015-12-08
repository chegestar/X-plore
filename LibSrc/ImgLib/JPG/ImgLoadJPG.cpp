#include <ImgLib.h>
namespace jpglib{
#include "jpeglib.h"
//#ifdef _WIN32_WCE
//#include <winbase.h>          //due to RaiseException
//#endif

//----------------------------

#if 0
#include <Profiler.h>
enum{
   PROF_ALL,
   PROF_SCANLINES,
   PROF_IDCT,
   PROF_0,
   PROF_1,
};
extern const char prof_img_texts[] =
   "All\0"
   "Scanlines\0"
   "IDCT\0"
   "0\0"
   "1\0"
;
extern C_profiler *prof_img;
#define PROF_S(n) prof_img->MarkS(n)
#define PROF_E(n) prof_img->MarkE(n)

#else

#define PROF_S(n)
#define PROF_E(n)
#endif

//----------------------------
//#define SHORTxSHORT_32

#ifdef SHORTxSHORT_32      /* may work if 'int' is 32 bits */
#define MULTIPLY16C16(var,_const)  ((short(var)) * (short(_const)))
#endif
#ifdef SHORTxLCONST_32      /* known to work with Microsoft C 6.0 */
#define MULTIPLY16C16(var,const)  (((short) (var)) * ((int) (const)))
#endif

#ifndef MULTIPLY16C16      /* default definition */
#define MULTIPLY16C16(var,const)  ((var) * (const))
#endif

/* Descale and correctly round an int value that's scaled by N bits.
 * We assume RIGHT_SHIFT rounds towards minus infinity, so adding
 * the fudge factor is correct for either sign of X.
 */

#define DESCALE(x,n)  RIGHT_SHIFT((x) + (1 << ((n)-1)), n)

/* Same except both inputs are variables. */

#ifdef SHORTxSHORT_32      /* may work if 'int' is 32 bits */
#define MULTIPLY16V16(var1,var2)  (((short) (var1)) * ((short) (var2)))
#endif

#ifndef MULTIPLY16V16      /* default definition */
#define MULTIPLY16V16(var1,var2)  ((var1) * (var2))
#endif

   /*
 * Each IDCT routine is responsible for range-limiting its results and
 * converting them to unsigned form (0..MAXJSAMPLE).  The raw outputs could
 * be quite far out of range if the input data is corrupt, so a bulletproof
 * range-limiting step is required.  We use a mask-and-table-lookup method
 * to do the combined operations quickly.  See the comments with
 * prepare_range_limit_table (in jdmaster.c) for more info.
 */

#define IDCT_range_limit(cinfo)  ((cinfo)->sample_range_limit + CENTERJSAMPLE)

#define RANGE_MASK  (MAXJSAMPLE * 4 + 3) /* 2 bits wider than legal samples */

   /*
 * In ANSI C, and indeed any rational implementation, size_t is also the
 * type returned by sizeof().  However, it seems there are some irrational
 * implementations out there, in which sizeof() returns an int even though
 * size_t is defined as long or unsigned long.  To ensure consistent results
 * we always use this SIZEOF() macro in place of using sizeof() directly.
 */

#define SIZEOF(object)  ((size_t) sizeof(object))

//#define JFREAD(h, buf, sizeofbuf) ((size_t)dtaRead(h, buf, sizeofbuf))
#define JFREAD(h, buf, sizeofbuf) (((S_file_struct*)h)->Read(buf, sizeofbuf))


/*
 * A forward DCT routine is given a pointer to a work area of type DCTELEM[];
 * the DCT is to be performed in-place in that buffer.  Type DCTELEM is int
 * for 8-bit samples, int for 12-bit samples.  (NOTE: Floating-point DCT
 * implementations use an array of type FAST_FLOAT, instead.)
 * The DCT inputs are expected to be signed (range +-CENTERJSAMPLE).
 * The DCT outputs are returned scaled up by a factor of 8; they therefore
 * have a range of +-8K for 8-bit data, +-128K for 12-bit data.  This
 * convention improves accuracy in integer implementations and saves some
 * work in floating-point ones.
 * Quantization of the output coefficients is done by jcdctmgr.c.
 */

typedef int DCTELEM;      /* 16 or 32 bits is fine */

typedef JMETHOD(void, forward_DCT_method_ptr, (DCTELEM * data));

/*
 * An inverse DCT routine is given a pointer to the input JBLOCK and a pointer
 * to an output sample array.  The routine must dequantize the input data as
 * well as perform the IDCT; for dequantization, it uses the multiplier table
 * pointed to by compptr->dct_table.  The output data is to be placed into the
 * sample array starting at a specified column.  (Any row offset needed will
 * be applied to the array pointer before it is passed to the IDCT code.)
 * Note that the number of samples emitted by the IDCT routine is
 * DCT_scaled_size * DCT_scaled_size.
 */

/* typedef inverse_DCT_method_ptr is declared in jpegint.h */

/*
 * Each IDCT routine has its own ideas about the best dct_table element type.
 */

typedef int ISLOW_MULT_TYPE; /* short or int, whichever is faster */
typedef int IFAST_MULT_TYPE; /* 16 bits is OK, use short if faster */
#define IFAST_SCALE_BITS  2   /* fractional bits in scale factors */


//----------------------------

#define MAX_ALLOC_CHUNK  1000000000L
//----------------------------

/*
 * jpeg_natural_order[i] is the natural-order position of the i'th element
 * of zigzag order.
 *
 * When reading corrupted data, the Huffman decoders could attempt
 * to reference an entry beyond the end of this array (if the decoded
 * zero run length reaches past the end of the block).  To prevent
 * wild stores without adding an inner-loop test, we put some extra
 * "63"s after the real entries.  This will cause the extra coefficient
 * to be stored in location 63 of the block, not somewhere random.
 * The worst case would be a run-length of 15, which means we need 16
 * fake entries.
 */

static const int jpeg_natural_order[DCTSIZE2+16] = {
  0,  1,  8, 16,  9,  2,  3, 10,
 17, 24, 32, 25, 18, 11,  4,  5,
 12, 19, 26, 33, 40, 48, 41, 34,
 27, 20, 13,  6,  7, 14, 21, 28,
 35, 42, 49, 56, 57, 50, 43, 36,
 29, 22, 15, 23, 30, 37, 44, 51,
 58, 59, 52, 45, 38, 31, 39, 46,
 53, 60, 61, 54, 47, 55, 62, 63,
 63, 63, 63, 63, 63, 63, 63, 63, /* extra entries for safety in decoder */
 63, 63, 63, 63, 63, 63, 63, 63
};

//----------------------------

enum JPEG_MARKER{       /* JPEG marker codes */
  M_SOF0  = 0xc0,
  M_SOF1  = 0xc1,
  M_SOF2  = 0xc2,
  M_SOF3  = 0xc3,
  
  M_SOF5  = 0xc5,
  M_SOF6  = 0xc6,
  M_SOF7  = 0xc7,
  
  M_JPG   = 0xc8,
  M_SOF9  = 0xc9,
  M_SOF10 = 0xca,
  M_SOF11 = 0xcb,
  
  M_SOF13 = 0xcd,
  M_SOF14 = 0xce,
  M_SOF15 = 0xcf,
  
  M_DHT   = 0xc4,
  
  M_DAC   = 0xcc,
  
  M_RST0  = 0xd0,
  M_RST1  = 0xd1,
  M_RST2  = 0xd2,
  M_RST3  = 0xd3,
  M_RST4  = 0xd4,
  M_RST5  = 0xd5,
  M_RST6  = 0xd6,
  M_RST7  = 0xd7,
  
  M_SOI   = 0xd8,
  M_EOI   = 0xd9,
  M_SOS   = 0xda,
  M_DQT   = 0xdb,
  M_DNL   = 0xdc,
  M_DRI   = 0xdd,
  M_DHP   = 0xde,
  M_EXP   = 0xdf,
  
  M_APP0  = 0xe0,
  M_APP1  = 0xe1,
  M_APP2  = 0xe2,
  M_APP3  = 0xe3,
  M_APP4  = 0xe4,
  M_APP5  = 0xe5,
  M_APP6  = 0xe6,
  M_APP7  = 0xe7,
  M_APP8  = 0xe8,
  M_APP9  = 0xe9,
  M_APP10 = 0xea,
  M_APP11 = 0xeb,
  M_APP12 = 0xec,
  M_APP13 = 0xed,
  M_APP14 = 0xee,
  M_APP15 = 0xef,
  
  M_JPG0  = 0xf0,
  M_JPG13 = 0xfd,
  M_COM   = 0xfe,
  
  M_TEM   = 0x01,
  
  M_ERROR = 0x100
};

//----------------------------

struct S_file_struct{
   C_file *ck;

   S_file_struct(C_file *ck1):
      ck(ck1)
   {
   }

   long Seek(long pos){
      if(!ck->Seek(pos))
         return -1;
      return pos;
   }
   long Read(void *buf, long sz){
                              //do not let loader read beyond size (jpg loader does try to)
      sz = Min(sz, long(ck->GetFileSize() - ck->Tell()));
      if(!ck->Read((char*)buf, sz))
         return 0;
      return sz;
   }
};

//----------------------------
#define INPUT_BUF_SIZE  32768  /* choose an efficiently fread'able size */

struct my_source_mgr{
   const JOCTET * next_input_byte; /* => next byte to read from buffer */
   size_t bytes_in_buffer;  /* # of bytes remaining in buffer */

   void init_source(){
      start_of_file = true;
   }
   bool fill_input_buffer();
   bool skip_input_data(long num_bytes);
   bool resync_to_restart(jpeg_decompress_struct* cinfo, int desired);

   int infile;     /* source stream */
   JOCTET buffer[INPUT_BUF_SIZE];      /* start of buffer */
   bool start_of_file;   /* have we gotten any data yet? */
};

//----------------------------

/*
 * Fill the input buffer --- called whenever buffer is emptied.
 *
 * In typical applications, this should read fresh data into the buffer
 * (ignoring the current state of next_input_byte & bytes_in_buffer),
 * reset the pointer & count to the start of the buffer, and return true
 * indicating that the buffer has been reloaded.  It is not necessary to
 * fill the buffer entirely, only to obtain at least one more byte.
 *
 * There is no such thing as an EOF return.  If the end of the file has been
 * reached, the routine has a choice of E RREXIT() or inserting fake data into
 * the buffer.  In most cases, generating a warning message and inserting a
 * fake EOI marker is the best course of action --- this will allow the
 * decompressor to output however much of the image is there.  However,
 * the resulting error message is misleading if the real problem is an empty
 * input file, so we handle that case specially.
 *
 * In applications that need to be able to suspend compression due to input
 * not being available yet, a false return indicates that no more data can be
 * obtained right now, but more may be forthcoming later.  In this situation,
 * the decompressor will return to its caller (with an indication of the
 * number of scanlines it has read, if any).  The application should resume
 * decompression after it has loaded more data into the input buffer.  Note
 * that there are substantial restrictions on the use of suspension --- see
 * the documentation.
 *
 * When suspending, the decompressor will back up to a convenient restart point
 * (typically the start of the current MCU). next_input_byte & bytes_in_buffer
 * indicate where the restart point will be if the current call returns false.
 * Data beyond this point must be rescanned after resumption, so move it to
 * the front of the buffer rather than discarding it.
 */

bool my_source_mgr::fill_input_buffer(){
  size_t nbytes;
  nbytes = JFREAD(infile, buffer, INPUT_BUF_SIZE);

  if (nbytes <= 0) {
    if (start_of_file)   /* Treat empty input file as fatal error */
      return false;//E RREXIT(cinfo, JERR_INPUT_EMPTY);
    /* Insert a fake EOI marker */
    buffer[0] = (JOCTET) 0xFF;
    buffer[1] = (JOCTET) 0xD9; //JPEG_EOI
    nbytes = 2;
  }

  next_input_byte = buffer;
  bytes_in_buffer = nbytes;
  start_of_file = false;

  return true;
}

//----------------------------

struct mem_pool_hdr{
   mem_pool_hdr *next;   /* next in list of pools */
   size_t bytes_used;     /* how many bytes already used within pool */
   size_t bytes_left;     /* bytes still available in this pool */
};

//----------------------------

struct my_memory_mgr{
   /* Method pointers */
   void *AllocSmall(size_t sizeofobject);
   JMETHOD(JSAMPARRAY, AllocSarray, (jpeg_decompress_struct* cinfo, JDIMENSION samplesperrow, JDIMENSION numrows));
   JMETHOD(JBLOCKARRAY, alloc_barray, (jpeg_decompress_struct* cinfo, JDIMENSION blocksperrow, JDIMENSION numrows));
   JMETHOD(jvirt_barray_ptr, request_virt_barray, (jpeg_decompress_struct* cinfo, bool pre_zero, JDIMENSION blocksperrow, JDIMENSION numrows, JDIMENSION maxaccess));
   JMETHOD(bool, realize_virt_arrays, (jpeg_decompress_struct* cinfo));
   JMETHOD(JSAMPARRAY, access_virt_sarray, (jpeg_decompress_struct* cinfo, jvirt_sarray_ptr ptr, JDIMENSION start_row, JDIMENSION num_rows, bool writable));
   JMETHOD(JBLOCKARRAY, access_virt_barray, (jpeg_decompress_struct* cinfo, jvirt_barray_ptr ptr, JDIMENSION start_row, JDIMENSION num_rows, bool writable));
   void free_pool();
   void self_destruct(){
      free_pool();
   }

  /* Each pool identifier (lifetime class) names a linked list of pools. */
   mem_pool_hdr *small_list;

  /* Since we only have one lifetime class of virtual arrays, only one
   * linked list is necessary (for each datatype).  Note that the virtual
   * array control blocks being linked together are actually stored somewhere
   * in the small-pool list.
   */
   jvirt_sarray_ptr virt_sarray_list;
   jvirt_barray_ptr virt_barray_list;

  /* alloc_sarray and alloc_barray set this value for use by virtual
   * array routines.
   */
   JDIMENSION last_rowsperchunk;  /* from most recent alloc_sarray/barray */
};

//----------------------------

void my_memory_mgr::free_pool(){
   mem_pool_hdr *shdr_ptr;
   
   virt_sarray_list = NULL;
   virt_barray_list = NULL;
   
   /* Release small objects */
   shdr_ptr = small_list;
   small_list = NULL;
   
   while (shdr_ptr != NULL) {
      mem_pool_hdr *next_shdr_ptr = shdr_ptr->next;
      delete[] (byte*)shdr_ptr;
      shdr_ptr = next_shdr_ptr;
   }
}

//----------------------------
/* DCT coefficient quantization tables. */
struct JQUANT_TBL{
  /* This array gives the coefficient quantizers in natural array order
   * (not the zigzag order in which they are stored in a JPEG DQT marker).
   * CAUTION: IJG versions prior to v6a kept this array in zigzag order.
   */
   word quantval[DCTSIZE2];  /* quantization step for each coefficient */
  /* This field is used only during compression.  It's initialized false when
   * the table is created, and set true when it's been output to the file.
   * You could suppress output of a table by setting this to true.
   * (See jpeg_suppress_tables for an example.)
   */
};

//----------------------------


/* Basic info about one component (color channel). */

struct jpeg_component_info{
  /* These values are fixed over the whole image. */
  /* For compression, they must be supplied by parameter setup; */
  /* for decompression, they are read from the SOF marker. */
  int component_id;     /* identifier for this component (0..255) */
  int component_index;     /* its index in SOF or cinfo->comp_info[] */
  int h_samp_factor;    /* horizontal sampling factor (1..4) */
  int v_samp_factor;    /* vertical sampling factor (1..4) */
  int quant_tbl_no;     /* quantization table selector (0..3) */
  /* These values may vary between scans. */
  /* For compression, they must be supplied by parameter setup; */
  /* for decompression, they are read from the SOS marker. */
  /* The decompressor output side may not use these variables. */
  int dc_tbl_no;     /* DC entropy table selector (0..3) */
  int ac_tbl_no;     /* AC entropy table selector (0..3) */
  
  /* Remaining fields should be treated as private by applications. */
  
  /* These values are computed during compression or decompression startup: */
  /* Component's size in DCT blocks.
   * Any dummy blocks added to complete an MCU are not counted; therefore
   * these values do not depend on whether a scan is interleaved or not.
   */
  JDIMENSION width_in_blocks;
  JDIMENSION height_in_blocks;
  /* Size of a DCT block in samples.  Always DCTSIZE for compression.
   * For decompression this is the size of the output from one DCT block,
   * reflecting any scaling we choose to apply during the IDCT step.
   * Values of 1,2,4,8 are likely to be supported.  Note that different
   * components may receive different IDCT scalings.
   */
  int DCT_scaled_size;
  /* The downsampled dimensions are the component's actual, unpadded number
   * of samples at the main buffer (preprocessing/compression interface), thus
   * downsampled_width = ceil(image_width * Hi/Hmax)
   * and similarly for height.  For decompression, IDCT scaling is included, so
   * downsampled_width = ceil(image_width * Hi/Hmax * DCT_scaled_size/DCTSIZE)
   */
  JDIMENSION downsampled_width;   /* actual width in samples */
  JDIMENSION downsampled_height; /* actual height in samples */
  /* This flag is used only for decompression.  In cases where some of the
   * components will be ignored (eg grayscale output from YCbCr image),
   * we can skip most computations for the unused components.
   */
  bool component_needed;   /* do we need the value of this component? */

  /* These values are computed before starting a scan of the component. */
  /* The decompressor output side may not use these variables. */
  int MCU_width;     /* number of blocks per MCU, horizontally */
  int MCU_height;    /* number of blocks per MCU, vertically */
  int MCU_blocks;    /* MCU_width * MCU_height */
  int MCU_sample_width;    /* MCU width in samples, MCU_width*DCT_scaled_size */
  int last_col_width;      /* # of non-dummy blocks across in last MCU */
  int last_row_height;     /* # of non-dummy blocks down in last MCU */

  /* Saved quantization table for component; NULL if none yet saved.
   * See jdinput.c comments about the need for this information.
   * This field is currently used only for decompression.
   */
  JQUANT_TBL *quant_table;

//----------------------------
/* Allocated multiplier tables: big enough for any supported variant */

   union{
     ISLOW_MULT_TYPE islow_array[DCTSIZE2];
#ifdef DCT_IFAST_SUPPORTED
     IFAST_MULT_TYPE ifast_array[DCTSIZE2];
#endif
   } dct_table;
   jpeg_component_info(){
      MemSet(&dct_table, 0, SIZEOF(dct_table));
   }
};

//----------------------------
typedef JMETHOD(void, inverse_DCT_method_ptr, (jpeg_decompress_struct* cinfo, jpeg_component_info * compptr, JCOEFPTR coef_block, JSAMPARRAY output_buf, JDIMENSION output_col));

static bool skip_variable (jpeg_decompress_struct* cinfo);
static bool get_app0(jpeg_decompress_struct* cinfo);
static bool get_app14(jpeg_decompress_struct* cinfo);
//----------------------------

typedef int bit_buf_type;   /* type of bit-extraction buffer */

struct bitread_perm_state{      /* Bitreading state saved across MCUs */
   bit_buf_type get_buffer;   /* current bit-extraction buffer */
   int bits_left;      /* # of unused bits in it */
   bool printed_eod;      /* flag to suppress multiple warning msgs */
};

//----------------------------

/*
 * Expanded entropy decoder object for Huffman decoding.
 *
 * The savable_state subrecord contains fields that change within an MCU,
 * but must not be updated permanently until we complete the MCU.
 */

struct savable_state{
  int last_dc_val[MAX_COMPS_IN_SCAN]; /* last DC coef for each component */
};

//----------------------------
/* Huffman coding tables. */

struct JHUFF_TBL{
  /* These two fields directly represent the contents of a JPEG DHT marker */
   byte bits[17];    /* bits[k] = # of symbols with codes of */
            /* length k bits; bits[0] is unused */
   byte huffval[256];      /* The symbols, in order of incr code length */
  /* This field is used only during compression.  It's initialized false when
   * the table is created, and set true when it's been output to the file.
   * You could suppress output of a table by setting this to true.
   * (See jpeg_suppress_tables for an example.)
   */
};


#define HUFF_LOOKAHEAD   8   /* # of bits of lookahead */

struct d_derived_tbl{
  /* Basic tables: (element [0] of each array is unused) */
   int mincode[17];      /* smallest code of length k */
   int maxcode[18];      /* largest code of length k (-1 if none) */
  /* (maxcode[17] is a sentinel to ensure jpeg_huff_decode terminates) */
   int valptr[17];      /* huffval[] index of 1st symbol of length k */

  /* Link to public Huffman table (needed only in jpeg_huff_decode) */
   JHUFF_TBL *_pub;

  /* Lookahead tables: indexed by the next HUFF_LOOKAHEAD bits of
   * the input data stream.  If the next Huffman code is no more
   * than HUFF_LOOKAHEAD bits long, we can obtain its length and
   * the corresponding symbol directly from these tables.
   */
   int look_nbits[1<<HUFF_LOOKAHEAD]; /* # bits, or 0 if too long */
   byte look_sym[1<<HUFF_LOOKAHEAD]; /* symbol, or unused */
};

//----------------------------

/* Entropy decoding */
struct huff_entropy_decoder{
   JMETHOD(bool, start_pass, (jpeg_decompress_struct* cinfo));
   JMETHOD(bool, decode_mcu, (jpeg_decompress_struct* cinfo,   JBLOCKROW *MCU_data));
   bitread_perm_state bitstate;   /* Bit buffer at start of MCU */
   savable_state saved;     /* Other state at start of MCU */

  /* These fields are NOT loaded into local working state. */
   dword restarts_to_go;   /* MCUs left in this restart interval */

  /* Pointers to derived tables (these workspaces have image lifespan) */
   d_derived_tbl dc_derived_tbls[NUM_HUFF_TBLS];
   d_derived_tbl ac_derived_tbls[NUM_HUFF_TBLS];
};

//----------------------------
static void start_iMCU_row (jpeg_decompress_struct* cinfo);
static int consume_markers (jpeg_decompress_struct* cinfo);

struct jpeg_decompress_struct{
   my_memory_mgr mem;  /* Memory manager module */
   E_STATE global_state;      /* for checking call sequence validity */

  /* Source of compressed data */
   my_source_mgr src;

  /* Basic description of image --- filled in by jpeg_read_header(). */
  /* Application may inspect these values to decide how to process image. */

   JDIMENSION image_width;  /* nominal image width (from SOF marker) */
   JDIMENSION image_height; /* nominal image height */
   int num_components;      /* # of color components in JPEG image */
   J_COLOR_SPACE jpeg_color_space; /* colorspace of JPEG image */

  /* Decompression processing parameters --- these fields must be set before
   * calling jpeg_start_decompress().  Note that jpeg_read_header() initializes
   * them to default values.
   */

   J_COLOR_SPACE out_color_space; /* colorspace for output */

   dword scale_num, scale_denom; /* fraction by which to scale image */

   bool buffered_image;  /* true=multiple output passes */

  JDIMENSION output_width; /* scaled image width */
  JDIMENSION output_height;   /* scaled image height */
  int out_color_components;   /* # of color components in out_color_space */

  /* Row index of next scanline to be read from jpeg_read_scanlines().
   * Application may use this to control its processing loop, e.g.,
   * "while (output_scanline < output_height)".
   */
  JDIMENSION output_scanline; /* 0 .. output_height-1  */

  /* Current input scan number and number of iMCU rows completed in scan.
   * These indicate the progress of the decompressor input side.
   */
  int input_scan_number;   /* Number of SOS markers seen so far */
  JDIMENSION input_iMCU_row;  /* Number of iMCU rows completed */

  /* The "output scan number" is the notional scan being displayed by the
   * output side.  The decompressor will not allow output scan/row number
   * to get ahead of input scan/row, but it can fall arbitrarily far behind.
   */
  int output_scan_number;  /* Nominal scan number being displayed */
  JDIMENSION output_iMCU_row; /* Number of iMCU rows read */

  /* Current progression status.  coef_bits[c][i] indicates the precision
   * with which component c's DCT coefficient i (in zigzag order) is known.
   * It is -1 when no data has yet been received, otherwise it is the point
   * transform (shift) value for the most recent scan of the coefficient
   * (thus, 0 at completion of the progression).
   * This pointer is NULL when reading a non-progressive file.
   */
  int (*coef_bits)[DCTSIZE2]; /* -1 or current Al value for each coef */

  /* Internal JPEG parameters --- the application usually need not look at
   * these fields.  Note that the decompressor output side may not use
   * any parameters that can change between scans.
   */

  /* Quantization and Huffman tables are carried forward across input
   * datastreams when processing abbreviated JPEG datastreams.
   */

  JQUANT_TBL *quant_tbl_ptrs[NUM_QUANT_TBLS];
  /* ptrs to coefficient quantization tables, or NULL if not defined */

  JHUFF_TBL * dc_huff_tbl_ptrs[NUM_HUFF_TBLS];
  JHUFF_TBL * ac_huff_tbl_ptrs[NUM_HUFF_TBLS];
  /* ptrs to Huffman coding tables, or NULL if not defined */

  /* These parameters are never carried across datastreams, since they
   * are given in SOF/SOS markers or defined to be reset by SOI.
   */

  int data_precision;      /* bits of precision in image data */

  jpeg_component_info *comp_info;
  /* comp_info[i] describes component that appears i'th in SOF */

  bool progressive_mode;   /* true if SOFn specifies progressive mode */
  bool arith_code;      /* true=arithmetic coding, false=Huffman */

  byte arith_dc_L[NUM_ARITH_TBLS]; /* L values for DC arith-coding tables */
  byte arith_dc_U[NUM_ARITH_TBLS]; /* U values for DC arith-coding tables */
  byte arith_ac_K[NUM_ARITH_TBLS]; /* Kx values for AC arith-coding tables */

  dword restart_interval; /* MCUs per restart interval, or 0 for no restart */

  /* These fields record data obtained from optional markers recognized by
   * the JPEG library.
   */
  bool saw_JFIF_marker; /* true iff a JFIF APP0 marker was found */
  /* Data copied from JFIF marker: */
  bool saw_Adobe_marker;   /* true iff an Adobe APP14 marker was found */
  byte Adobe_transform;   /* Color transform code from Adobe marker */

  bool CCIR601_sampling;   /* true=first samples are cosited */

  /*
   * These fields are computed during decompression startup
   */
  int max_h_samp_factor;   /* largest h_samp_factor */
  int max_v_samp_factor;   /* largest v_samp_factor */

  int min_DCT_scaled_size; /* smallest DCT_scaled_size of any component */

  JDIMENSION total_iMCU_rows; /* # of iMCU rows in image */
  /* The coefficient controller's input and output progress is measured in
   * units of "iMCU" (interleaved MCU) rows.  These are the same as MCU rows
   * in fully interleaved JPEG scans, but are used whether the scan is
   * interleaved or not.  We define an iMCU row as v_samp_factor DCT block
   * rows of each component.  Therefore, the IDCT output contains
   * v_samp_factor*DCT_scaled_size sample rows of a component per iMCU row.
   */

  JSAMPLE sample_range_base[5 * (MAXJSAMPLE+1) + CENTERJSAMPLE]; /* table for fast range-limiting */
  JSAMPLE *sample_range_limit;

  /*
   * These fields are valid during any one scan.
   * They describe the components and MCUs actually appearing in the scan.
   * Note that the decompressor output side must not use these fields.
   */
  int comps_in_scan;    /* # of JPEG components in this scan */
  jpeg_component_info *cur_comp_info[MAX_COMPS_IN_SCAN];
  /* *cur_comp_info[i] describes component that appears i'th in SOS */

  JDIMENSION MCUs_per_row; /* # of MCUs across the image */
  //JDIMENSION MCU_rows_in_scan;   /* # of MCU rows in the image */

  int blocks_in_MCU;    /* # of DCT blocks per MCU */
  int MCU_membership[D_MAX_BLOCKS_IN_MCU];
  /* MCU_membership[i] is index in cur_comp_info of component owning */
  /* i'th block in an MCU */

  int Ss, Se, Ah, Al;      /* progressive JPEG parameters for scan */

  /* This field is shared between entropy decoder and marker parser.
   * It is either zero or the code of a JPEG marker that has been
   * read from the data source, but has not yet been processed.
   */
  int unread_marker;

//----------------------------
   bool prepare_for_output_pass(){
      if(!idct.start_pass(this))
         return false;
      coef.start_output_pass(this);
      upsample.start_pass_upsample(this);
      post.start_pass(this);
      main.start_pass_main();
      return true;
   }
//----------------------------
   struct{
      bool init(jpeg_decompress_struct* cinfo){

         /* Allocate the workspace.
         * ngroups is the number of row groups we need.
         */
         int ngroups = cinfo->min_DCT_scaled_size;
         for(int ci = 0; ci < cinfo->num_components; ci++){
            jpeg_component_info *compptr = &cinfo->comp_info[ci];
            int rgroup = (compptr->v_samp_factor * compptr->DCT_scaled_size) / cinfo->min_DCT_scaled_size; /* height of a row group of component */
            buffer[ci] = (*cinfo->mem.AllocSarray) (cinfo, compptr->width_in_blocks * compptr->DCT_scaled_size, (JDIMENSION) (rgroup * ngroups));
            if(!buffer[ci])
               return false;
         }
         return true;
      }

      void start_pass_main(){
              /* Simple case with no context needed */
         buffer_full = false;   /* Mark buffer empty */
         rowgroup_ctr = 0;
      }
      void process_data(jpeg_decompress_struct* cinfo, JSAMPARRAY output_buf, dword *out_row_ctr, dword out_rows_avail){

         dword rowgroups_avail;

         /* Read input data if we haven't filled the main buffer yet */
         if (! buffer_full) {
                                    //prof: 83% here
            if (!cinfo->coef.decompress_data(cinfo, buffer))
               return;        /* suspension forced, can do nothing more */
            buffer_full = true; /* OK, we have an iMCU row to work with */
         }
                                    //prof: 17% here
         /* There are always min_DCT_scaled_size row groups in an iMCU row. */
         rowgroups_avail = cinfo->min_DCT_scaled_size;
         /* Note: at the bottom of the image, we may pass extra garbage row groups
         * to the postprocessor.  The postprocessor has to check for bottom
         * of image anyway (at row resolution), so no point in us doing it too.
         */

         /* Feed the postprocessor */
         cinfo->upsample.sep_upsample(cinfo, buffer, &rowgroup_ctr, rowgroups_avail, output_buf, out_row_ctr, out_rows_avail);

         /* Has postprocessor consumed all the data yet? If so, mark buffer empty */
         if (rowgroup_ctr >= rowgroups_avail) {
            buffer_full = false;
            rowgroup_ctr = 0;
         }
      }

      /* Pointer to allocated workspace (M or M+2 row groups). */
      JSAMPARRAY buffer[MAX_COMPONENTS];

      bool buffer_full;     /* Have we gotten an iMCU row from decoder? */
      JDIMENSION rowgroup_ctr; /* counts row groups output to postprocessor */

      /* Remaining fields are only used in the context case. */

      /* These are the master pointers to the funny-order pointer lists. */
      JSAMPIMAGE xbuffer;   /* pointers to weird pointer lists */
      JDIMENSION rowgroups_avail; /* row groups available to postprocessor */
   } main;
//----------------------------
   struct my_coef_controller{
      void start_input_pass(jpeg_decompress_struct* cinfo){
         cinfo->input_iMCU_row = 0;
         start_iMCU_row(cinfo);
      }
      static int consume_data(jpeg_decompress_struct* cinfo);
      void start_output_pass (jpeg_decompress_struct* cinfo){
         cinfo->output_iMCU_row = 0;
      }
      JMETHOD(int, decompress_data, (jpeg_decompress_struct* cinfo, JSAMPIMAGE output_buf));
      /* These variables keep track of the current location of the input side. */
      /* cinfo->input_iMCU_row is also used for this. */
      JDIMENSION MCU_ctr;      /* counts MCUs processed in current row */
      int MCU_vert_offset;     /* counts MCU rows within iMCU row */
      int MCU_rows_per_iMCU_row;  /* number of such rows needed */

      /* The output side's location is represented by cinfo->output_iMCU_row. */

      /* In single-pass modes, it's sufficient to buffer just one MCU.
      * We allocate a workspace of D_MAX_BLOCKS_IN_MCU coefficient blocks,
      * and let the entropy decoder write into that workspace each time.
      * (On 80x86, the workspace is even though it's not really very big;
      * this is to keep the module interfaces unchanged when a large coefficient
      * buffer is necessary.)
      * In multi-pass modes, this array points to the current MCU's blocks
      * within the virtual arrays; it is used only by the input side.
      */
      JBLOCKROW MCU_buffer[D_MAX_BLOCKS_IN_MCU];

#ifdef D_MULTISCAN_FILES_SUPPORTED
      /* In multi-pass modes, we need a virtual block array for each component. */
      jvirt_barray_ptr whole_image[MAX_COMPONENTS];
#endif
   };
   my_coef_controller coef;
//----------------------------
   struct{
      void start_pass(jpeg_decompress_struct* cinfo){
              /* For single-pass processing without color quantization,
              * I have no work to do; just call the upsampler directly.
              */
         starting_row = next_row = 0;
      }
      /* Color quantization source buffer: this holds output data from
      * the upsample/color conversion step to be passed to the quantizer.
      * For two-pass color quantization, we need a full-image buffer;
      * for one-pass operation, a strip buffer is sufficient.
      */
      jvirt_sarray_ptr whole_image;  /* virtual array, or NULL if one-pass */
      JSAMPARRAY buffer;    /* strip buffer, or current strip of virtual */
      JDIMENSION strip_height; /* buffer size in rows */
      /* for two-pass mode only: */
      JDIMENSION starting_row; /* row # of first row in current strip */
      JDIMENSION next_row;     /* index of next row to fill/empty in strip */
   } post;
//----------------------------
   struct my_input_controller{
      JMETHOD(int, _consume_input, (jpeg_decompress_struct* cinfo));
      void reset_input_controller (jpeg_decompress_struct* cinfo){

         _consume_input = consume_markers;
         has_multiple_scans = false; /* "unknown" would be better */
         eoi_reached = false;
         inheaders = true;
         /* Reset other modules */
         cinfo->marker.reset_marker_reader(cinfo);
         /* Reset progression state -- would be cleaner if entropy decoder did this */
         cinfo->coef_bits = NULL;
      }
      JMETHOD(bool, start_input_pass, (jpeg_decompress_struct* cinfo));
      JMETHOD(void, finish_input_pass, (jpeg_decompress_struct* cinfo));

      /* State variables made visible to other modules */
      bool has_multiple_scans;   /* True if file has multiple scans */
      bool eoi_reached;      /* True when EOI has been consumed */

      bool inheaders;    /* true until first SOS is reached */
   };
   my_input_controller inputctl;
//----------------------------
   /* Marker reading & parsing */
   struct jpeg_marker_reader {
   /*
    * Reset marker processing state to begin a fresh datastream.
    */
      void reset_marker_reader(jpeg_decompress_struct* cinfo){
         delete[] cinfo->comp_info;
         cinfo->comp_info = NULL;    /* until allocated by get_sof */
         cinfo->input_scan_number = 0;     /* no SOS seen yet */
         cinfo->unread_marker = 0;      /* no pending marker */
         cinfo->marker.saw_SOI = false;   /* set internal state too */
         cinfo->marker.saw_SOF = false;
         cinfo->marker.discarded_bytes = 0;
      }
     /* Read markers until SOS or EOI.
      * Returns same codes as are defined for jpeg_consume_input:
      * JPEG_SUSPENDED, JPEG_REACHED_SOS, or JPEG_REACHED_EOI.
      */
     int read_markers(jpeg_decompress_struct* cinfo);

     typedef JMETHOD(bool, jpeg_marker_parser_method, (jpeg_decompress_struct* cinfo));
     /* Read a restart marker --- exported for use by entropy decoder only */
     bool read_restart_marker(jpeg_decompress_struct* cinfo);
     /* Application-overridable marker processing methods */
     jpeg_marker_parser_method process_COM;
     jpeg_marker_parser_method process_APPn[16];

     /* State of marker reader --- nominally internal, but applications
      * supplying COM or APPn handlers might like to know the state.
      */
     bool saw_SOI;      /* found SOI? */
     bool saw_SOF;      /* found SOF? */
     int next_restart_num;      /* next restart number expected (0-7) */
     dword discarded_bytes;   /* # of bytes skipped looking for a marker */
   };
   jpeg_marker_reader marker;
//----------------------------
   huff_entropy_decoder *entropy;
//----------------------------
   struct my_idct_controller{
      void init(jpeg_decompress_struct *cinfo){
         for(int ci = 0; ci < cinfo->num_components; ci++) {
      /* Mark multiplier table not yet set up for any method */
            cur_method[ci] = -1;
         }
      }

      bool start_pass(jpeg_decompress_struct* cinfo);
      /* It is useful to allow each component to have a separate IDCT method. */
      inverse_DCT_method_ptr inverse_DCT[MAX_COMPONENTS];

      /* This array contains the IDCT method code that each multiplier table
      * is currently set up for, or -1 if it's not yet set up.
      * The actual multiplier tables are pointed to by dct_table in the
      * per-component comp_info structures.
      */
      int cur_method[MAX_COMPONENTS];
   };
   my_idct_controller idct;

//----------------------------
   struct my_upsampler{
      void start_pass_upsample(jpeg_decompress_struct* cinfo){
         /* Mark the conversion buffer empty */
         next_row_out = cinfo->max_v_samp_factor;
         /* Initialize total-height counter for detecting bottom of image */
         rows_to_go = cinfo->output_height;
      }
      void sep_upsample(jpeg_decompress_struct* cinfo, JSAMPIMAGE input_buf, JDIMENSION *in_row_group_ctr, JDIMENSION in_row_groups_avail, JSAMPARRAY output_buf, JDIMENSION *out_row_ctr, JDIMENSION out_rows_avail);
     /* Color conversion buffer.  When using separate upsampling and color
      * conversion steps, this buffer holds one upsampled row group until it
      * has been color converted and output.
      * Note: we do not allocate any storage for component(s) which are full-size,
      * ie do not need rescaling.  The corresponding entry of color_buf[] is
      * simply set to point to the input data array, thereby avoiding copying.
      */
     JSAMPARRAY color_buf[MAX_COMPONENTS];

     typedef JMETHOD(void, upsample1_ptr, (jpeg_decompress_struct* cinfo, jpeg_component_info * compptr, JSAMPARRAY input_data, JSAMPARRAY * output_data_ptr));
     /* Per-component upsampling method pointers */
     upsample1_ptr methods[MAX_COMPONENTS];

     int next_row_out;     /* counts rows emitted from color_buf */
     JDIMENSION rows_to_go;   /* counts rows remaining in image */

     /* Height of an input row group for each component. */
     int rowgroup_height[MAX_COMPONENTS];

     /* These arrays save pixel expansion factors so that int_expand need not
      * recompute them each time.  They are unused for other upsampling methods.
      */
     byte h_expand[MAX_COMPONENTS];
     byte v_expand[MAX_COMPONENTS];
   } upsample;
//----------------------------
   struct my_color_deconverter{
      JMETHOD(void, color_convert, (jpeg_decompress_struct* cinfo,   JSAMPIMAGE input_buf, JDIMENSION input_row,   JSAMPARRAY output_buf, int num_rows));

     /* Private state for YCC->RGB conversion */
      int * Cr_r_tab;    /* => table for Cr to R conversion */
      int * Cb_b_tab;    /* => table for Cb to B conversion */
      int * Cr_g_tab;     /* => table for Cr to G conversion */
      int * Cb_g_tab;     /* => table for Cb to G conversion */
      ~my_color_deconverter(){
         delete[] Cr_r_tab;
      }
   };
   my_color_deconverter cconvert;

//----------------------------

   void jinit_memory_mgr();

//----------------------------

   void jinit_input_controller();
   void prepare_range_limit_table();

//----------------------------
   jpeg_decompress_struct(){
      MemSet(this, 0, SIZEOF(*this));
      jinit_memory_mgr();

      /* Initialize method pointers */
      marker.process_COM = skip_variable;
      for (int i = 0; i < 16; i++)
         marker.process_APPn[i] = skip_variable;
      marker.process_APPn[0] = get_app0;
      marker.process_APPn[14] = get_app14;
      /* Reset marker processing state */
      marker.reset_marker_reader(this);
      jinit_input_controller();
      prepare_range_limit_table();

      global_state = DSTATE_START;
   }
   ~jpeg_decompress_struct(){
      for(int i=NUM_QUANT_TBLS; i--; ){
         delete quant_tbl_ptrs[i];
      }
      delete[] comp_info;
      for(int i=NUM_HUFF_TBLS; i--; ){
         delete dc_huff_tbl_ptrs[i];
         delete ac_huff_tbl_ptrs[i];
      }
      delete entropy;
   }
   bool master_selection();
};

//----------------------------

/*
 * Several decompression processes need to range-limit values to the range
 * 0..MAXJSAMPLE; the input value may fall somewhat outside this range
 * due to noise introduced by quantization, roundoff error, etc.  These
 * processes are inner loops and need to be as fast as possible.  On most
 * machines, particularly CPUs with pipelines or instruction prefetch,
 * a (subscript-check-less) C table lookup
 *    x = sample_range_limit[x];
 * is faster than explicit tests
 *    if (x < 0)  x = 0;
 *    else if (x > MAXJSAMPLE)  x = MAXJSAMPLE;
 * These processes all use a common table prepared by the routine below.
 *
 * For most steps we can mathematically guarantee that the initial value
 * of x is within MAXJSAMPLE+1 of the legal range, so a table running from
 * -(MAXJSAMPLE+1) to 2*MAXJSAMPLE+1 is sufficient.  But for the initial
 * limiting step (just after the IDCT), a wildly out-of-range value is 
 * possible if the input data is corrupt.  To avoid any chance of indexing
 * off the end of memory and getting a bad-pointer trap, we perform the
 * post-IDCT limiting thus:
 *    x = range_limit[x & MASK];
 * where MASK is 2 bits wider than legal sample data, ie 10 bits for 8-bit
 * samples.  Under normal circumstances this is more than enough range and
 * a correct output will be generated; with bogus input data the mask will
 * cause wraparound, and we will safely generate a bogus-but-in-range output.
 * For the post-IDCT step, we want to convert the data from signed to unsigned
 * representation by adding CENTERJSAMPLE at the same time that we limit it.
 * So the post-IDCT limiting table ends up looking like this:
 *   CENTERJSAMPLE,CENTERJSAMPLE+1,...,MAXJSAMPLE,
 *   MAXJSAMPLE (repeat 2*(MAXJSAMPLE+1)-CENTERJSAMPLE times),
 *   0          (repeat 2*(MAXJSAMPLE+1)-CENTERJSAMPLE times),
 *   0,1,...,CENTERJSAMPLE-1
 * Negative inputs select values from the upper half of the table after
 * masking.
 *
 * We can save some space by overlapping the start of the post-IDCT table
 * with the simpler range limiting table.  The post-IDCT table begins at
 * sample_range_limit + CENTERJSAMPLE.
 *
 * Note that the table is allocated in near data space on PCs; it's small
 * enough and used often enough to justify this.
 */

/* Allocate and fill in the sample_range_limit table */
void jpeg_decompress_struct::prepare_range_limit_table (){
   int i;
   JSAMPLE *table = sample_range_base;
   table += (MAXJSAMPLE+1); /* allow negative subscripts of simple table */
   sample_range_limit = table;
   /* First segment of "simple" table: limit[x] = 0 for x < 0 */
   MemSet(table - (MAXJSAMPLE+1), 0, (MAXJSAMPLE+1) * SIZEOF(JSAMPLE));
   /* Main part of "simple" table: limit[x] = x */
   for (i = 0; i <= MAXJSAMPLE; i++)
      table[i] = (JSAMPLE) i;
   table += CENTERJSAMPLE;  /* Point to where post-IDCT table starts */
   /* End of simple table, rest of first half of post-IDCT table */
   for (i = CENTERJSAMPLE; i < 2*(MAXJSAMPLE+1); i++)
      table[i] = MAXJSAMPLE;
   /* Second half of post-IDCT table */
   MemSet(table + (2 * (MAXJSAMPLE+1)), 0, (2 * (MAXJSAMPLE+1) - CENTERJSAMPLE) * SIZEOF(JSAMPLE));
   MemCpy(table + (4 * (MAXJSAMPLE+1) - CENTERJSAMPLE), sample_range_limit, CENTERJSAMPLE * SIZEOF(JSAMPLE));
}

//----------------------------

/*
 * Macros for fetching data from the data source module.
 *
 * At all times, cinfo->src->next_input_byte and ->bytes_in_buffer reflect
 * the current restart point; we update them only when we have reached a
 * suitable place to restart if a suspension occurs.
 */

/* Declare and initialize local copies of input pointer/count */
#define INPUT_VARS(cinfo)  \
   my_source_mgr *datasrc = &(cinfo)->src;  \
   const JOCTET * next_input_byte = datasrc->next_input_byte;  \
   size_t bytes_in_buffer = datasrc->bytes_in_buffer

/* Unload the local copies --- do this only at a restart boundary */
#define INPUT_SYNC(cinfo)  \
   ( datasrc->next_input_byte = next_input_byte,  \
     datasrc->bytes_in_buffer = bytes_in_buffer )

/* Reload the local copies --- seldom used except in MAKE_BYTE_AVAIL */
#define INPUT_RELOAD(cinfo)  \
   ( next_input_byte = datasrc->next_input_byte,  \
     bytes_in_buffer = datasrc->bytes_in_buffer )

/* Internal macro for INPUT_BYTE and INPUT_2BYTES: make a byte available.
 * Note we do *not* do INPUT_SYNC before calling fill_input_buffer,
 * but we must reload the local copies after a successful fill.
 */
#define MAKE_BYTE_AVAIL(cinfo,action)  \
   if (bytes_in_buffer == 0) {  \
     if (!datasrc->fill_input_buffer())  \
       { action; }  \
     INPUT_RELOAD(cinfo);  \
   }  \
   bytes_in_buffer--

/* Read a byte into variable V.
 * If must suspend, take the specified action (typically "return false").
 */
#define INPUT_BYTE(cinfo,V,action)  \
   do{ MAKE_BYTE_AVAIL(cinfo,action); \
   V = GETJOCTET(*next_input_byte++); }while(0)

/* As above, but read two bytes interpreted as an unsigned 16-bit integer.
 * V should be declared dword or perhaps int.
 */
#define INPUT_2BYTES(cinfo,V,action)  \
   do{ MAKE_BYTE_AVAIL(cinfo,action); \
        V = ((dword) GETJOCTET(*next_input_byte++)) << 8; \
        MAKE_BYTE_AVAIL(cinfo,action); \
        V += GETJOCTET(*next_input_byte++); }while(0)

//----------------------------

static bool get_dqt (jpeg_decompress_struct* cinfo)
/* Process a DQT marker */
{
  int length;
  int n, i, prec;
  dword tmp;
  JQUANT_TBL *quant_ptr;
  INPUT_VARS(cinfo);

  INPUT_2BYTES(cinfo, length, return false);
  length -= 2;

  while (length > 0) {
    INPUT_BYTE(cinfo, n, return false);
    prec = n >> 4;
    n &= 0x0F;

    if (n >= NUM_QUANT_TBLS)
      return false;//E RREXIT1(cinfo, JERR_DQT_INDEX, n);
      
    if(!cinfo->quant_tbl_ptrs[n]){
         cinfo->quant_tbl_ptrs[n] = new JQUANT_TBL;
         if(!cinfo->quant_tbl_ptrs[n])
            return false;
    }
    quant_ptr = cinfo->quant_tbl_ptrs[n];

    for (i = 0; i < DCTSIZE2; i++) {
      if (prec)
   INPUT_2BYTES(cinfo, tmp, return false);
      else
   INPUT_BYTE(cinfo, tmp, return false);
      /* We convert the zigzag-order table to natural array order. */
      quant_ptr->quantval[jpeg_natural_order[i]] = (word) tmp;
    }

    length -= DCTSIZE2+1;
    if (prec) length -= DCTSIZE2;
  }

  INPUT_SYNC(cinfo);
  return true;
}

//----------------------------
static bool first_marker (jpeg_decompress_struct* cinfo)
/* Like next_marker, but used to obtain the initial SOI marker. */
/* For this marker, we do not allow preceding garbage or fill; otherwise,
 * we might well scan an entire input file before realizing it ain't JPEG.
 * If an application wants to process non-JFIF files, it must seek to the
 * SOI before calling the JPEG library.
 */
{
  int c, c2;
  INPUT_VARS(cinfo);

  INPUT_BYTE(cinfo, c, return false);
  INPUT_BYTE(cinfo, c2, return false);
  if (c != 0xFF || c2 != (int) M_SOI)
    //E RREXIT2(cinfo, JERR_NO_SOI, c, c2);
    return false;

  cinfo->unread_marker = c2;

  INPUT_SYNC(cinfo);
  return true;
}

//----------------------------
/*
 * Find the next JPEG marker, save it in cinfo->unread_marker.
 * Returns false if had to suspend before reaching a marker;
 * in that case cinfo->unread_marker is unchanged.
 *
 * Note that the result might not be a valid marker code,
 * but it will never be 0 or FF.
 */
static bool next_marker (jpeg_decompress_struct* cinfo){
  int c;
  INPUT_VARS(cinfo);

  for (;;) {
    INPUT_BYTE(cinfo, c, return false);
    /* Skip any non-FF bytes.
     * This may look a bit inefficient, but it will not occur in a valid file.
     * We sync after each discarded byte so that a suspending data source
     * can discard the byte from its buffer.
     */
    while (c != 0xFF) {
      cinfo->marker.discarded_bytes++;
      INPUT_SYNC(cinfo);
      INPUT_BYTE(cinfo, c, return false);
    }
    /* This loop swallows any duplicate FF bytes.  Extra FFs are legal as
     * pad bytes, so don't count them in discarded_bytes.  We assume there
     * will not be so many consecutive FF bytes as to overflow a suspending
     * data source's input buffer.
     */
    do {
      INPUT_BYTE(cinfo, c, return false);
    } while (c == 0xFF);
    if (c != 0)
      break;         /* found a valid marker, exit loop */
    /* Reach here if we found a stuffed-zero data sequence (FF/00).
     * Discard it and loop back to try again.
     */
    cinfo->marker.discarded_bytes += 2;
    INPUT_SYNC(cinfo);
  }

  if (cinfo->marker.discarded_bytes != 0) {
    cinfo->marker.discarded_bytes = 0;
  }

  cinfo->unread_marker = c;

  INPUT_SYNC(cinfo);
  return true;
}

//----------------------------
/*
 * Routines to process JPEG markers.
 *
 * Entry condition: JPEG marker itself has been read and its code saved
 *   in cinfo->unread_marker; input restart point is just after the marker.
 *
 * Exit: if return true, have read and processed any parameters, and have
 *   updated the restart point to point after the parameters.
 *   If return false, was forced to suspend before reaching end of
 *   marker parameters; restart point has not been moved.  Same routine
 *   will be called again after application supplies more input data.
 *
 * This approach to suspension assumes that all of a marker's parameters can
 * fit into a single input bufferload.  This should hold for "normal"
 * markers.  Some COM/APPn markers might have large parameter segments,
 * but we use skip_input_data to get past those, and thereby put the problem
 * on the source manager's shoulders.
 *
 * Note that we don't bother to avoid duplicate trace messages if a
 * suspension occurs within marker parameters.  Other side effects
 * require more care.
 */


static bool get_soi (jpeg_decompress_struct* cinfo){
/* Process an SOI marker */
  int i;
  
  if (cinfo->marker.saw_SOI)
    return false;//E RREXIT(cinfo, JERR_SOI_DUPLICATE);

  /* Reset all parameters that are defined to be reset by SOI */

  for (i = 0; i < NUM_ARITH_TBLS; i++) {
    cinfo->arith_dc_L[i] = 0;
    cinfo->arith_dc_U[i] = 1;
    cinfo->arith_ac_K[i] = 5;
  }
  cinfo->restart_interval = 0;

  /* Set initial assumptions for colorspace etc */

  cinfo->jpeg_color_space = JCS_UNKNOWN;
  cinfo->CCIR601_sampling = false; /* Assume non-CCIR sampling??? */

  cinfo->saw_JFIF_marker = false;
  cinfo->saw_Adobe_marker = false;
  cinfo->Adobe_transform = 0;

  cinfo->marker.saw_SOI = true;

  return true;
}

//----------------------------

static bool get_sof (jpeg_decompress_struct* cinfo, bool is_prog, bool is_arith){
   /* Process a SOFn marker */
   int length;
   INPUT_VARS(cinfo);

   cinfo->progressive_mode = is_prog;
   cinfo->arith_code = is_arith;

   INPUT_2BYTES(cinfo, length, return false);

   INPUT_BYTE(cinfo, cinfo->data_precision, return false);
   INPUT_2BYTES(cinfo, cinfo->image_height, return false);
   INPUT_2BYTES(cinfo, cinfo->image_width, return false);
   INPUT_BYTE(cinfo, cinfo->num_components, return false);

   length -= 8;

   if (cinfo->marker.saw_SOF)
      return false;//E//RREXIT(cinfo, JERR_SOF_DUPLICATE);

   /* We don't support files in which the image height is initially specified */
   /* as 0 and is later redefined by DNL.  As long as we have to check that,  */
   /* might as well have a general sanity check. */
   if (cinfo->image_height <= 0 || cinfo->image_width <= 0
      || cinfo->num_components <= 0)
      return false;//E RREXIT(cinfo, JERR_EMPTY_IMAGE);

   if (length != (cinfo->num_components * 3))
      return false;//E RREXIT(cinfo, JERR_BAD_LENGTH);

   if(!cinfo->comp_info){  /* do only once, even if suspend */
      cinfo->comp_info = new jpeg_component_info[cinfo->num_components];
      if(!cinfo->comp_info)
         return false;
   }

   for(int ci = 0; ci < cinfo->num_components; ci++){
      jpeg_component_info *compptr = &cinfo->comp_info[ci];
      compptr->component_index = ci;
      INPUT_BYTE(cinfo, compptr->component_id, return false);
      int c;
      INPUT_BYTE(cinfo, c, return false);
      compptr->h_samp_factor = (c >> 4) & 15;
      compptr->v_samp_factor = (c     ) & 15;
      INPUT_BYTE(cinfo, compptr->quant_tbl_no, return false);

   }

   cinfo->marker.saw_SOF = true;

   INPUT_SYNC(cinfo);
   return true;
}

//----------------------------

static bool get_sos (jpeg_decompress_struct* cinfo){
   /* Process a SOS marker */
   int length;
   int i, ci, n, c, cc;
   jpeg_component_info * compptr;
   INPUT_VARS(cinfo);

   if (! cinfo->marker.saw_SOF)
      return false;//E RREXIT(cinfo, JERR_SOS_NO_SOF);

   INPUT_2BYTES(cinfo, length, return false);

   INPUT_BYTE(cinfo, n, return false); /* Number of components */

   if (length != (n * 2 + 6) || n < 1 || n > MAX_COMPS_IN_SCAN)
      return false;//E RREXIT(cinfo, JERR_BAD_LENGTH);


   cinfo->comps_in_scan = n;

   /* Collect the component-spec parameters */

   for (i = 0; i < n; i++) {
      INPUT_BYTE(cinfo, cc, return false);
      INPUT_BYTE(cinfo, c, return false);

      for (ci = 0, compptr = cinfo->comp_info; ci < cinfo->num_components;
         ci++, compptr++) {
            if (cc == compptr->component_id)
               goto id_found;
      }

      return false;//E RREXIT1(cinfo, JERR_BAD_COMPONENT_ID, cc);

id_found:

      cinfo->cur_comp_info[i] = compptr;
      compptr->dc_tbl_no = (c >> 4) & 15;
      compptr->ac_tbl_no = (c     ) & 15;
   }

   /* Collect the additional scan parameters Ss, Se, Ah/Al. */
   INPUT_BYTE(cinfo, c, return false);
   cinfo->Ss = c;
   INPUT_BYTE(cinfo, c, return false);
   cinfo->Se = c;
   INPUT_BYTE(cinfo, c, return false);
   cinfo->Ah = (c >> 4) & 15;
   cinfo->Al = (c     ) & 15;

   /* Prepare to scan data & restart markers */
   cinfo->marker.next_restart_num = 0;

   /* Count another SOS marker */
   cinfo->input_scan_number++;

   INPUT_SYNC(cinfo);
   return true;
}

//----------------------------

static bool get_dac (jpeg_decompress_struct* cinfo)
/* Process a DAC marker */
{
   int length;
   int index, val;
   INPUT_VARS(cinfo);

   INPUT_2BYTES(cinfo, length, return false);
   length -= 2;

   while (length > 0) {
      INPUT_BYTE(cinfo, index, return false);
      INPUT_BYTE(cinfo, val, return false);

      length -= 2;

      if (index < 0 || index >= (2*NUM_ARITH_TBLS))
         return false;//E RREXIT1(cinfo, JERR_DAC_INDEX, index);

      if (index >= NUM_ARITH_TBLS) { /* define AC table */
         cinfo->arith_ac_K[index-NUM_ARITH_TBLS] = (byte) val;
      } else {         /* define DC table */
         cinfo->arith_dc_L[index] = (byte) (val & 0x0F);
         cinfo->arith_dc_U[index] = (byte) (val >> 4);
         if (cinfo->arith_dc_L[index] > cinfo->arith_dc_U[index])
            return false;//E RREXIT1(cinfo, JERR_DAC_VALUE, val);
      }
   }

   INPUT_SYNC(cinfo);
   return true;
}

//----------------------------

static bool get_dht (jpeg_decompress_struct* cinfo)
/* Process a DHT marker */
{
   int length;
   byte bits[17];
   byte huffval[256];
   int i, index, count;
   JHUFF_TBL **htblptr;
   INPUT_VARS(cinfo);

   INPUT_2BYTES(cinfo, length, return false);
   length -= 2;

   while (length > 0) {
      INPUT_BYTE(cinfo, index, return false);

      bits[0] = 0;
      count = 0;
      for (i = 1; i <= 16; i++) {
         INPUT_BYTE(cinfo, bits[i], return false);
         count += bits[i];
      }

      length -= 1 + 16;

      if (count > 256 || ((int) count) > length)
         return false;//E RREXIT(cinfo, JERR_DHT_COUNTS);

      for (i = 0; i < count; i++)
         INPUT_BYTE(cinfo, huffval[i], return false);

      length -= count;

      if (index & 0x10) {    /* AC table definition */
         index -= 0x10;
         htblptr = &cinfo->ac_huff_tbl_ptrs[index];
      } else {         /* DC table definition */
         htblptr = &cinfo->dc_huff_tbl_ptrs[index];
      }

      if (index < 0 || index >= NUM_HUFF_TBLS)
         return false;//E RREXIT1(cinfo, JERR_DHT_INDEX, index);

      if(!*htblptr){
         *htblptr = new JHUFF_TBL;
         if(!*htblptr)
            return false;
      }

      MemCpy((*htblptr)->bits, bits, SIZEOF((*htblptr)->bits));
      MemCpy((*htblptr)->huffval, huffval, SIZEOF((*htblptr)->huffval));
   }

   INPUT_SYNC(cinfo);
   return true;
}




static bool
get_dri (jpeg_decompress_struct* cinfo)
/* Process a DRI marker */
{
  int length;
  dword tmp;
  INPUT_VARS(cinfo);

  INPUT_2BYTES(cinfo, length, return false);
  
  if (length != 4)
    return false;//E RREXIT(cinfo, JERR_BAD_LENGTH);

  INPUT_2BYTES(cinfo, tmp, return false);

  cinfo->restart_interval = tmp;

  INPUT_SYNC(cinfo);
  return true;
}

//----------------------------

/* Skip over an unknown or uninteresting variable-length marker */
static bool skip_variable (jpeg_decompress_struct* cinfo){
  int length;
  INPUT_VARS(cinfo);

  INPUT_2BYTES(cinfo, length, return false);
  
  INPUT_SYNC(cinfo);    /* do before skip_input_data */
  if(!cinfo->src.skip_input_data((long) length - 2L))
     return false;

  return true;
}

/*
 * Read a restart marker, which is expected to appear next in the datastream;
 * if the marker is not there, take appropriate recovery action.
 * Returns false if suspension is required.
 *
 * This is called by the entropy decoder after it has read an appropriate
 * number of MCUs.  cinfo->unread_marker may be nonzero if the entropy decoder
 * has already read a marker from the data source.  Under normal conditions
 * cinfo->unread_marker will be reset to 0 before returning; if not reset,
 * it holds a marker which the decoder will be unable to read past.
 */

bool jpeg_decompress_struct::jpeg_marker_reader::read_restart_marker (jpeg_decompress_struct* cinfo){
  /* Obtain a marker unless we already did. */
  /* Note that next_marker will complain if it skips any data. */
  if (cinfo->unread_marker == 0) {
    if (! next_marker(cinfo))
      return false;
  }

  if (cinfo->unread_marker ==
      ((int) M_RST0 + cinfo->marker.next_restart_num)) {
    /* Normal case --- swallow the marker and let entropy decoder continue */
    cinfo->unread_marker = 0;
  } else {
    /* Uh-oh, the restart markers have been messed up. */
    /* Let the data source manager determine how to resync. */
    if (!cinfo->src.resync_to_restart(cinfo, cinfo->marker.next_restart_num))
      return false;
  }

  /* Update next-restart state */
  cinfo->marker.next_restart_num = (cinfo->marker.next_restart_num + 1) & 7;

  return true;
}

//----------------------------

/*
 * Read markers until SOS or EOI.
 *
 * Returns same codes as are defined for jpeg_consume_input:
 * JPEG_SUSPENDED, JPEG_REACHED_SOS, or JPEG_REACHED_EOI.
 */

int jpeg_decompress_struct::jpeg_marker_reader::read_markers (jpeg_decompress_struct* cinfo){
   /* Outer loop repeats once for each marker. */
   for (;;) {
      /* Collect the marker proper, unless we already did. */
      /* NB: first_marker() enforces the requirement that SOI appear first. */
      if (cinfo->unread_marker == 0) {
         if (! cinfo->marker.saw_SOI) {
            if (! first_marker(cinfo))
               return JPEG_SUSPENDED;
         } else {
            if (! next_marker(cinfo))
               return JPEG_SUSPENDED;
         }
      }
      /* At this point cinfo->unread_marker contains the marker code and the
      * input point is just past the marker proper, but before any parameters.
      * A suspension will cause us to return with this state still true.
      */
      switch (cinfo->unread_marker) {
    case M_SOI:
       if (! get_soi(cinfo))
          return JPEG_ERROR;
       break;

    case M_SOF0:     /* Baseline */
    case M_SOF1:     /* Extended sequential, Huffman */
       if (! get_sof(cinfo, false, false))
          return JPEG_SUSPENDED;
       break;

    case M_SOF2:     /* Progressive, Huffman */
       if (! get_sof(cinfo, true, false))
          return JPEG_SUSPENDED;
       break;

    case M_SOF9:     /* Extended sequential, arithmetic */
       if (! get_sof(cinfo, false, true))
          return JPEG_SUSPENDED;
       break;

    case M_SOF10:    /* Progressive, arithmetic */
       if (! get_sof(cinfo, true, true))
          return JPEG_SUSPENDED;
       break;

       /* Currently unsupported SOFn types */
    case M_SOF3:     /* Lossless, Huffman */
    case M_SOF5:     /* Differential sequential, Huffman */
    case M_SOF6:     /* Differential progressive, Huffman */
    case M_SOF7:     /* Differential lossless, Huffman */
    case M_JPG:         /* Reserved for JPEG extensions */
    case M_SOF11:    /* Lossless, arithmetic */
    case M_SOF13:    /* Differential sequential, arithmetic */
    case M_SOF14:    /* Differential progressive, arithmetic */
    case M_SOF15:    /* Differential lossless, arithmetic */
       //E RREXIT1(cinfo, JERR_SOF_UNSUPPORTED, cinfo->unread_marker);
       return JPEG_ERROR;
       break;

    case M_SOS:
       if (! get_sos(cinfo))
          return JPEG_SUSPENDED;
       cinfo->unread_marker = 0;  /* processed the marker */
       return JPEG_REACHED_SOS;

    case M_EOI:
       cinfo->unread_marker = 0;  /* processed the marker */
       return JPEG_REACHED_EOI;

    case M_DAC:
       if (! get_dac(cinfo))
          return JPEG_SUSPENDED;
       break;

    case M_DHT:
       if (! get_dht(cinfo))
          return JPEG_SUSPENDED;
       break;

    case M_DQT:
       if (! get_dqt(cinfo))
          return JPEG_SUSPENDED;
       break;

    case M_DRI:
       if (! get_dri(cinfo))
          return JPEG_SUSPENDED;
       break;

    case M_APP0:
    case M_APP1:
    case M_APP2:
    case M_APP3:
    case M_APP4:
    case M_APP5:
    case M_APP6:
    case M_APP7:
    case M_APP8:
    case M_APP9:
    case M_APP10:
    case M_APP11:
    case M_APP12:
    case M_APP13:
    case M_APP14:
    case M_APP15:
       if (! (*cinfo->marker.process_APPn[cinfo->unread_marker - (int) M_APP0]) (cinfo))
          return JPEG_SUSPENDED;
       break;

    case M_COM:
       if (! (*cinfo->marker.process_COM) (cinfo))
          return JPEG_SUSPENDED;
       break;

    case M_RST0:     /* these are all parameterless */
    case M_RST1:
    case M_RST2:
    case M_RST3:
    case M_RST4:
    case M_RST5:
    case M_RST6:
    case M_RST7:
    case M_TEM:
       break;

    case M_DNL:         /* Ignore DNL ... perhaps the wrong thing */
       if (! skip_variable(cinfo))
          return JPEG_SUSPENDED;
       break;

    default:         /* must be DHP, EXP, JPGn, or RESn */
       /* For now, we treat the reserved markers as fatal errors since they are
       * likely to be used to signal incompatible JPEG Part 3 extensions.
       * Once the JPEG 3 version-number marker is well defined, this code
       * ought to change!
       */
       return JPEG_ERROR;//E RREXIT1(cinfo, JERR_UNKNOWN_MARKER, cinfo->unread_marker);
       break;
      }
      /* Successfully processed marker, so reset state variable */
      cinfo->unread_marker = 0;
   } /* end loop */
}

//----------------------------

static bool get_app0 (jpeg_decompress_struct* cinfo){
#define JFIF_LEN 14
   int length;
   byte b[JFIF_LEN];
   int buffp;
   INPUT_VARS(cinfo);

   INPUT_2BYTES(cinfo, length, return false);
   length -= 2;

   /* See if a JFIF APP0 marker is present */

   if (length >= JFIF_LEN) {
      for (buffp = 0; buffp < JFIF_LEN; buffp++)
         INPUT_BYTE(cinfo, b[buffp], return false);
      length -= JFIF_LEN;

      if (b[0]==0x4A && b[1]==0x46 && b[2]==0x49 && b[3]==0x46 && b[4]==0) {
         /* Found JFIF APP0 marker: check version */
         /* Major version must be 1, anything else signals an incompatible change.
         * We used to treat this as an error, but now it's a nonfatal warning,
         * because some bozo at Hijaak couldn't read the spec.
         * Minor version should be 0..2, but process anyway if newer.
         */
         /* Save info */
         cinfo->saw_JFIF_marker = true;
         //byte density_unit = b[7];
         //cinfo->X_density = (b[8] << 8) + b[9];
         //cinfo->Y_density = (b[10] << 8) + b[11];
      } else {
         /* Start of APP0 does not match "JFIF" */
      }
   } else {
      /* Too short to be JFIF marker */
   }

   INPUT_SYNC(cinfo);
   if (length > 0)    /* skip any remaining data -- could be lots */
      if(!cinfo->src.skip_input_data((long) length))
         return false;

   return true;
}


static bool get_app14 (jpeg_decompress_struct* cinfo)
/* Process an APP14 marker */
{
#define ADOBE_LEN 12
  int length;
  byte b[ADOBE_LEN];
  int buffp;
  dword version, flags0, flags1, transform;
  INPUT_VARS(cinfo);

  INPUT_2BYTES(cinfo, length, return false);
  length -= 2;

  /* See if an Adobe APP14 marker is present */

  if (length >= ADOBE_LEN) {
    for (buffp = 0; buffp < ADOBE_LEN; buffp++)
      INPUT_BYTE(cinfo, b[buffp], return false);
    length -= ADOBE_LEN;

    if (b[0]==0x41 && b[1]==0x64 && b[2]==0x6F && b[3]==0x62 && b[4]==0x65) {
      /* Found Adobe APP14 marker */
      version = (b[5] << 8) + b[6];
      flags0 = (b[7] << 8) + b[8];
      flags1 = (b[9] << 8) + b[10];
      transform = b[11];
      cinfo->saw_Adobe_marker = true;
      cinfo->Adobe_transform = (byte) transform;
    } else {
      /* Start of APP14 does not match "Adobe" */
    }
  } else {
    /* Too short to be Adobe marker */
  }

  INPUT_SYNC(cinfo);
  if (length > 0)    /* skip any remaining data -- could be lots */
    if(!cinfo->src.skip_input_data((long) length))
       return false;

  return true;
}

//----------------------------

static void jpeg_destroy_decompress (jpeg_decompress_struct* cinfo){
   cinfo->mem.self_destruct();
  cinfo->global_state = DSTATE_NULL; /* mark it destroyed */
}

//----------------------------

/*
 * Decompression startup: read start of JPEG datastream to see what's there.
 * Need only initialize JPEG object and supply a data source before calling.
 *
 * This routine will read as far as the first SOS marker (ie, actual start of
 * compressed data), and will save all tables and parameters in the JPEG
 * object.  It will also initialize the decompression parameters to default
 * values, and finally return JPEG_HEADER_OK.  On return, the application may
 * adjust the decompression parameters and then call jpeg_start_decompress.
 * (Or, if the application only wanted to determine the image parameters,
 * the data need not be decompressed.  In that case, call jpeg_abort or
 * jpeg_destroy to release any temporary space.)
 * If an abbreviated (tables only) datastream is presented, the routine will
 * return JPEG_HEADER_TABLES_ONLY upon reaching EOI.  The application may then
 * re-use the JPEG object to read the abbreviated image datastream(s).
 * It is unnecessary (but OK) to call jpeg_abort in this case.
 * The JPEG_SUSPENDED return code only occurs if the data source module
 * requests suspension of the decompressor.  In this case the application
 * should load more source data and then re-call jpeg_read_header to resume
 * processing.
 * If a non-suspending data source is used and require_image is true, then the
 * return code need not be inspected since only JPEG_HEADER_OK is possible.
 *
 * This routine is now just a front end to jpeg_consume_input, with some
 * extra error checking.
 */

static int jpeg_read_header (jpeg_decompress_struct* cinfo){
  int retcode;

/*
  if (cinfo->global_state != DSTATE_START &&
      cinfo->global_state != DSTATE_INHEADER)
    E RREXIT1(cinfo, JERR_BAD_STATE, cinfo->global_state);
    */

  retcode = jpeg_consume_input(cinfo);

  switch (retcode) {
  case JPEG_REACHED_SOS:
    retcode = JPEG_HEADER_OK;
    break;
  case JPEG_REACHED_EOI:
  /*
    if (require_image)
      E RREXIT(cinfo, JERR_NO_IMAGE);
      */
    /* Reset to start state; it would be safer to require the application to
     * call jpeg_abort, but we can't change it now for compatibility reasons.
     * A side effect is to free any temporary memory (there shouldn't be any).
     */
    jpeg_abort(cinfo); /* sets state = DSTATE_START */
    retcode = JPEG_HEADER_TABLES_ONLY;
    break;
  case JPEG_SUSPENDED:
    /* no work */
    break;
  }

  return retcode;
}

//----------------------------

/*
 * Set default decompression parameters.
 */

static void
default_decompress_parms (jpeg_decompress_struct* cinfo)
{
   /* Guess the input colorspace, and set output colorspace accordingly. */
   /* (Wish JPEG committee had provided a real way to specify this...) */
   /* Note application may override our guesses. */
   switch (cinfo->num_components) {
   case 1:
      cinfo->jpeg_color_space = JCS_GRAYSCALE;
      cinfo->out_color_space = JCS_GRAYSCALE;
      break;
      
   case 3:
      if (cinfo->saw_JFIF_marker) {
         cinfo->jpeg_color_space = JCS_YCbCr; /* JFIF implies YCbCr */
      } else
      if (cinfo->saw_Adobe_marker) {
         switch (cinfo->Adobe_transform) {
         case 0:
            cinfo->jpeg_color_space = JCS_RGB;
            break;
         case 1:
            cinfo->jpeg_color_space = JCS_YCbCr;
            break;
         default:
            cinfo->jpeg_color_space = JCS_YCbCr; /* assume it's YCbCr */
            break;
         }
      } else {
         /* Saw no special markers, try to guess from the component IDs */
         int cid0 = cinfo->comp_info[0].component_id;
         int cid1 = cinfo->comp_info[1].component_id;
         int cid2 = cinfo->comp_info[2].component_id;
         
         if (cid0 == 1 && cid1 == 2 && cid2 == 3)
            cinfo->jpeg_color_space = JCS_YCbCr; /* assume JFIF w/out marker */
         else if (cid0 == 82 && cid1 == 71 && cid2 == 66)
            cinfo->jpeg_color_space = JCS_RGB; /* ASCII 'R', 'G', 'B' */
         else {
            cinfo->jpeg_color_space = JCS_YCbCr; /* assume it's YCbCr */
         }
      }
      /* Always guess RGB is proper output colorspace. */
      cinfo->out_color_space = JCS_RGB;
      break;
      
   case 4:
      if (cinfo->saw_Adobe_marker) {
         switch (cinfo->Adobe_transform) {
         case 0:
            cinfo->jpeg_color_space = JCS_CMYK;
            break;
         case 2:
            cinfo->jpeg_color_space = JCS_YCCK;
            break;
         default:
            cinfo->jpeg_color_space = JCS_YCCK; /* assume it's YCCK */
            break;
         }
      } else {
         /* No special markers, assume straight CMYK. */
         cinfo->jpeg_color_space = JCS_CMYK;
      }
      cinfo->out_color_space = JCS_CMYK;
      break;
      
   default:
      cinfo->jpeg_color_space = JCS_UNKNOWN;
      cinfo->out_color_space = JCS_UNKNOWN;
      break;
   }
   
   /* Set defaults for other decompression parameters. */
   cinfo->scale_num = 1;    /* 1:1 scaling */
   cinfo->scale_denom = 1;
   cinfo->buffered_image = false;
}

//----------------------------

/*
 * Consume data in advance of what the decompressor requires.
 * This can be called at any time once the decompressor object has
 * been created and a data source has been set up.
 *
 * This routine is essentially a state machine that handles a couple
 * of critical state-transition actions, namely initial setup and
 * transition from header scanning to ready-for-start_decompress.
 * All the actual input is done via the input controller's consume_input
 * method.
 */

static int jpeg_consume_input (jpeg_decompress_struct* cinfo){
   int retcode = JPEG_SUSPENDED;

   /* NB: every possible DSTATE value should be listed in this switch */
   switch (cinfo->global_state) {
   case DSTATE_START:
      /* Start-of-datastream actions: reset appropriate modules */
      cinfo->inputctl.reset_input_controller(cinfo);
      /* Initialize application's data source module */
      cinfo->src.init_source();
      cinfo->global_state = DSTATE_INHEADER;
      /*FALLTHROUGH*/
   case DSTATE_INHEADER:
      retcode = cinfo->inputctl._consume_input(cinfo);
      if (retcode == JPEG_REACHED_SOS) { /* Found SOS, prepare to decompress */
         /* Set up default parameters based on header data */
         default_decompress_parms(cinfo);
         /* Set global state: ready for start_decompress */
         cinfo->global_state = DSTATE_READY;
      }
      break;
   case DSTATE_READY:
      /* Can't advance past first SOS until start_decompress is called */
      retcode = JPEG_REACHED_SOS;
      break;
   case DSTATE_PRELOAD:
   case DSTATE_PRESCAN:
   case DSTATE_SCANNING:
      retcode = cinfo->inputctl._consume_input(cinfo);
      break;
   default:
      return JPEG_ERROR; //E RREXIT1(cinfo, JERR_BAD_STATE, cinfo->global_state);
   }
   return retcode;
}

//----------------------------

/*
 * Abort processing of a JPEG compression or decompression operation,
 * but don't destroy the object itself.
 *
 * For this, we merely clean up all the nonpermanent memory pools.
 * Note that temp files (virtual arrays) are not allowed to belong to
 * the permanent pool, so we will be able to close all temp files here.
 * Closing a data source or destination, if necessary, is the application's
 * responsibility.
 */

static void jpeg_abort (jpeg_decompress_struct* cinfo){
  cinfo->mem.self_destruct();
  /* Reset overall state for possible reuse of object */
  cinfo->global_state = DSTATE_START;
}





//----------------------------

/* Forward declarations */
static bool output_pass_setup (jpeg_decompress_struct* cinfo);


/*
 * Decompression initialization.
 * jpeg_read_header must be completed before calling this.
 *
 * If a multipass operating mode was selected, this will do all but the
 * last pass, and thus may take a great deal of time.
 *
 */

static bool jpegStartDecompress(jpeg_decompress_struct* cinfo){

   if (cinfo->global_state == DSTATE_READY) {
      cinfo->global_state = DSTATE_PRELOAD;
   }
   if (cinfo->global_state == DSTATE_PRELOAD) {
      /* If file has multiple scans, absorb them all into the coef buffer */
      if (cinfo->inputctl.has_multiple_scans) {
#ifdef D_MULTISCAN_FILES_SUPPORTED
         while(true){
            /* Absorb some more input */
            int retcode = cinfo->inputctl._consume_input(cinfo);
            if (retcode == JPEG_SUSPENDED)
               return true;
            if (retcode == JPEG_ERROR)
               return false;
            if (retcode == JPEG_REACHED_EOI)
               break;
         }
#else
         E RREXIT(cinfo, JERR_NOT_COMPILED);
#endif
      }
      cinfo->output_scan_number = cinfo->input_scan_number;
   } else
   if (cinfo->global_state != DSTATE_PRESCAN)
      return false;//E RREXIT1(cinfo, JERR_BAD_STATE, cinfo->global_state);
   /* Perform any dummy output passes, and set up for the final pass */
   return output_pass_setup(cinfo);
}

//----------------------------

/*
 * Set up for an output pass, and perform any dummy pass(es) needed.
 * Common subroutine for jpeg_start_decompress and jpeg_start_output.
 * Entry: global_state = DSTATE_PRESCAN only if previously suspended.
 */

static bool output_pass_setup (jpeg_decompress_struct* cinfo){

  if (cinfo->global_state != DSTATE_PRESCAN) {
    /* First call: do pass setup */
    if(!cinfo->prepare_for_output_pass())
       return false;
    cinfo->output_scanline = 0;
    cinfo->global_state = DSTATE_PRESCAN;
  }
  /* Ready for application to drive output pass through
   * jpeg_read_scanlines
   */
  cinfo->global_state = DSTATE_SCANNING;
  return true;
}

//----------------------------
/*
 * Read some scanlines of data from the JPEG decompressor.
 *
 * The return value will be the number of lines actually read.
 * This may be less than the number requested in several cases,
 * including bottom of image, data source suspension, and operating
 * modes that emit multiple scanlines at a time.
 *
 * Note: we warn about excess calls to jpeg_read_scanlines() since
 * this likely signals an application programmer error.  However,
 * an oversize buffer (max_lines > scanlines remaining) is not an error.
 */
static JDIMENSION jpeg_read_scanlines (jpeg_decompress_struct* cinfo, JSAMPARRAY scanlines, JDIMENSION max_lines){

   if(cinfo->global_state != DSTATE_SCANNING)
      return 0;//E RREXIT1(cinfo, JERR_BAD_STATE, cinfo->global_state);
   if(cinfo->output_scanline >= cinfo->output_height){
      return 0;
   }
                              //Process some data
   dword row_ctr = 0;
   cinfo->main.process_data(cinfo, scanlines, &row_ctr, max_lines);
   cinfo->output_scanline += row_ctr;
   return row_ctr;
}

//----------------------------
/*
 * Skip data --- used to skip over a potentially large amount of
 * uninteresting data (such as an APPn marker).
 *
 * Writers of suspendable-input applications must note that skip_input_data
 * is not granted the right to give a suspension return.  If the skip extends
 * beyond the data currently in the buffer, the buffer can be marked empty so
 * that the next read will cause a fill_input_buffer call that can suspend.
 * Arranging for additional bytes to be discarded before reloading the input
 * buffer is the application writer's problem.
 */

bool my_source_mgr::skip_input_data(long num_bytes){
   /* Just a dumb implementation for now.  Could use fseek() except
   * it doesn't work on pipes.  Not clear that being smart is worth
   * any trouble anyway --- large skips are infrequent.
   */
   if (num_bytes > 0) {
      while (num_bytes > (long) bytes_in_buffer) {
         num_bytes -= (long)bytes_in_buffer;
         if(!fill_input_buffer())
            return false;
         /* note we assume that fill_input_buffer will never return false,
         * so suspension need not be handled.
         */
      }
      next_input_byte += (size_t) num_bytes;
      bytes_in_buffer -= (size_t) num_bytes;
   }
   return true;
}


/*
 * An additional method that can be provided by data source modules is the
 * resync_to_restart method for error recovery in the presence of RST markers.
 * For the moment, this source module just uses the default resync method
 * provided by the JPEG library.  That method assumes that no backtracking
 * is possible.
 */


/*
 * Prepare for input from a stdio stream.
 * The caller must have already opened the stream, and is responsible
 * for closing it after finishing decompression.
 */

static bool jpeg_stdio_src (jpeg_decompress_struct* cinfo, int infile){

   my_source_mgr * src = &cinfo->src;
  //src->term_source = term_source;
  src->infile = infile;
  src->bytes_in_buffer = 0; /* forces fill_input_buffer on first read */
  src->next_input_byte = NULL; /* until buffer loaded */
  return true;
}

//----------------------------

static void start_iMCU_row (jpeg_decompress_struct* cinfo)
/* Reset within-iMCU-row counters for a new row (input side) */
{
   jpeg_decompress_struct::my_coef_controller &coef = cinfo->coef;

   /* In an interleaved scan, an MCU row is the same as an iMCU row.
   * In a noninterleaved scan, an iMCU row has v_samp_factor MCU rows.
   * But at the bottom of the image, process only what's left.
   */
   if (cinfo->comps_in_scan > 1) {
      coef.MCU_rows_per_iMCU_row = 1;
   } else {
      if (cinfo->input_iMCU_row < (cinfo->total_iMCU_rows-1))
         coef.MCU_rows_per_iMCU_row = cinfo->cur_comp_info[0]->v_samp_factor;
      else
         coef.MCU_rows_per_iMCU_row = cinfo->cur_comp_info[0]->last_row_height;
   }

   coef.MCU_ctr = 0;
   coef.MCU_vert_offset = 0;
}

//----------------------------
/*
 * Decompress and return some data in the single-pass case.
 * Always attempts to emit one fully interleaved MCU row ("iMCU" row).
 * Input and output must run in lockstep since we have only a one-MCU buffer.
 * Return value is JPEG_ROW_COMPLETED, JPEG_SCAN_COMPLETED, or JPEG_SUSPENDED.
 *
 * NB: output_buf contains a plane for each component in image.
 * For single pass, this is the same as the components in the scan.
 */
static int decompress_onepass (jpeg_decompress_struct* cinfo, JSAMPIMAGE output_buf){

   jpeg_decompress_struct::my_coef_controller &coef = cinfo->coef;
   JDIMENSION MCU_col_num;  /* index of current MCU within row */
   JDIMENSION last_MCU_col = cinfo->MCUs_per_row - 1;
   JDIMENSION last_iMCU_row = cinfo->total_iMCU_rows - 1;
   int blkn, ci, xindex, yindex, yoffset, useful_width;
   JSAMPARRAY output_ptr;
   JDIMENSION start_col, output_col;
   jpeg_component_info *compptr;

   /* Loop to process as much as one whole iMCU row */
   for(yoffset = coef.MCU_vert_offset; yoffset < coef.MCU_rows_per_iMCU_row; yoffset++){
      for(MCU_col_num = coef.MCU_ctr; MCU_col_num <= last_MCU_col; MCU_col_num++){
         /* Try to fetch an MCU.  Entropy decoder expects buffer to be zeroed. */
         MemSet(coef.MCU_buffer[0], 0, (cinfo->blocks_in_MCU * SIZEOF(JBLOCK)));
         PROF_S(PROF_0);
                              //prof: 55% of loop
         if (! (*cinfo->entropy->decode_mcu) (cinfo, coef.MCU_buffer)) {
            /* Suspension forced; update state counters and exit */
            coef.MCU_vert_offset = yoffset;
            coef.MCU_ctr = MCU_col_num;
            return JPEG_SUSPENDED;
         }
         PROF_E(PROF_0);
         /* Determine where data should go in output_buf and do the IDCT thing.
         * We skip dummy blocks at the right and bottom edges (but blkn gets
         * incremented past them!).  Note the inner loop relies on having
         * allocated the MCU_buffer[] blocks sequentially.
         */
         PROF_S(PROF_1);
                              //prof: 45% of loop
         blkn = 0;         /* index of current DCT block within MCU */
         for (ci = 0; ci < cinfo->comps_in_scan; ci++) {
            compptr = cinfo->cur_comp_info[ci];
            /* Don't bother to IDCT an uninteresting component. */
            if (! compptr->component_needed) {
               blkn += compptr->MCU_blocks;
               continue;
            }
            inverse_DCT_method_ptr inverse_DCT = cinfo->idct.inverse_DCT[compptr->component_index];
            useful_width = (MCU_col_num < last_MCU_col) ? compptr->MCU_width : compptr->last_col_width;
            output_ptr = output_buf[ci] + yoffset * compptr->DCT_scaled_size;
            start_col = MCU_col_num * compptr->MCU_sample_width;
            for (yindex = 0; yindex < compptr->MCU_height; yindex++) {
               if (cinfo->input_iMCU_row < last_iMCU_row || yoffset+yindex < compptr->last_row_height) {
                     output_col = start_col;
                     for (xindex = 0; xindex < useful_width; xindex++) {
                        PROF_S(PROF_IDCT);
                        (*inverse_DCT)(cinfo, compptr, (JCOEFPTR) coef.MCU_buffer[blkn+xindex], output_ptr, output_col);
                        PROF_E(PROF_IDCT);
                        output_col += compptr->DCT_scaled_size;
                     }
               }
               blkn += compptr->MCU_width;
               output_ptr += compptr->DCT_scaled_size;
            }
         }
         PROF_E(PROF_1);
      }
      /* Completed an MCU row, but perhaps not an iMCU row */
      coef.MCU_ctr = 0;
   }
   /* Completed the iMCU row, advance counters for next one */
   cinfo->output_iMCU_row++;
   if (++(cinfo->input_iMCU_row) < cinfo->total_iMCU_rows) {
      start_iMCU_row(cinfo);
      return JPEG_ROW_COMPLETED;
   }
   /* Completed the scan */
   (*cinfo->inputctl.finish_input_pass) (cinfo);
   return JPEG_SCAN_COMPLETED;
}

//----------------------------
/*
 * Dummy consume-input routine for single-pass operation.
 */

static int
dummy_consume_data (jpeg_decompress_struct* cinfo)
{
  return JPEG_SUSPENDED;   /* Always indicate nothing was done */
}


#ifdef D_MULTISCAN_FILES_SUPPORTED

/*
 * Consume input data and store it in the full-image coefficient buffer.
 * We read as much as one fully interleaved MCU row ("iMCU" row) per call,
 * ie, v_samp_factor block rows for each component in the scan.
 * Return value is JPEG_ROW_COMPLETED, JPEG_SCAN_COMPLETED, or JPEG_SUSPENDED.
 */

int jpeg_decompress_struct::my_coef_controller::consume_data (jpeg_decompress_struct* cinfo){
   jpeg_decompress_struct::my_coef_controller &coef = cinfo->coef;
   JDIMENSION MCU_col_num;  /* index of current MCU within row */
   int blkn, ci, xindex, yindex, yoffset;
   JDIMENSION start_col;
   JBLOCKARRAY buffer[MAX_COMPS_IN_SCAN];
   JBLOCKROW buffer_ptr;
   jpeg_component_info *compptr;

   /* Align the virtual buffers for the components used in this scan. */
   for (ci = 0; ci < cinfo->comps_in_scan; ci++) {
      compptr = cinfo->cur_comp_info[ci];
      buffer[ci] = (*cinfo->mem.access_virt_barray) (cinfo, coef.whole_image[compptr->component_index], cinfo->input_iMCU_row * compptr->v_samp_factor, (JDIMENSION) compptr->v_samp_factor, true);
      if(!buffer[ci])
         return JPEG_ERROR;
      /* Note: entropy decoder expects buffer to be zeroed,
      * but this is handled automatically by the memory manager
      * because we requested a pre-zeroed array.
      */
   }

   /* Loop to process one whole iMCU row */
   for (yoffset = coef.MCU_vert_offset; yoffset < coef.MCU_rows_per_iMCU_row;
      yoffset++) {
         for (MCU_col_num = coef.MCU_ctr; MCU_col_num < cinfo->MCUs_per_row;
            MCU_col_num++) {
               /* Construct list of pointers to DCT blocks belonging to this MCU */
               blkn = 0;         /* index of current DCT block within MCU */
               for (ci = 0; ci < cinfo->comps_in_scan; ci++) {
                  compptr = cinfo->cur_comp_info[ci];
                  start_col = MCU_col_num * compptr->MCU_width;
                  for (yindex = 0; yindex < compptr->MCU_height; yindex++) {
                     buffer_ptr = buffer[ci][yindex+yoffset] + start_col;
                     for (xindex = 0; xindex < compptr->MCU_width; xindex++) {
                        coef.MCU_buffer[blkn++] = buffer_ptr++;
                     }
                  }
               }
               /* Try to fetch the MCU. */
               if (! (*cinfo->entropy->decode_mcu) (cinfo, coef.MCU_buffer)) {
                  /* Suspension forced; update state counters and exit */
                  coef.MCU_vert_offset = yoffset;
                  coef.MCU_ctr = MCU_col_num;
                  return JPEG_SUSPENDED;
               }
         }
         /* Completed an MCU row, but perhaps not an iMCU row */
         coef.MCU_ctr = 0;
   }
   /* Completed the iMCU row, advance counters for next one */
   if (++(cinfo->input_iMCU_row) < cinfo->total_iMCU_rows) {
      start_iMCU_row(cinfo);
      return JPEG_ROW_COMPLETED;
   }
   /* Completed the scan */
   (*cinfo->inputctl.finish_input_pass) (cinfo);
   return JPEG_SCAN_COMPLETED;
}


/*
 * Decompress and return some data in the multi-pass case.
 * Always attempts to emit one fully interleaved MCU row ("iMCU" row).
 * Return value is JPEG_ROW_COMPLETED, JPEG_SCAN_COMPLETED, or JPEG_SUSPENDED.
 *
 * NB: output_buf contains a plane for each component in image.
 */

static int decompress_data (jpeg_decompress_struct* cinfo, JSAMPIMAGE output_buf){
   jpeg_decompress_struct::my_coef_controller &coef = cinfo->coef;
   JDIMENSION last_iMCU_row = cinfo->total_iMCU_rows - 1;
   JDIMENSION block_num;
   int ci, block_row, block_rows;
   JBLOCKARRAY buffer;
   JBLOCKROW buffer_ptr;
   JSAMPARRAY output_ptr;
   JDIMENSION output_col;
   jpeg_component_info *compptr;

   /* Force some input to be done if we are getting ahead of the input. */
   while (cinfo->input_scan_number < cinfo->output_scan_number ||
      (cinfo->input_scan_number == cinfo->output_scan_number &&
      cinfo->input_iMCU_row <= cinfo->output_iMCU_row)) {
         if (cinfo->inputctl._consume_input(cinfo) == JPEG_SUSPENDED)
            return JPEG_SUSPENDED;
   }

   /* OK, output from the virtual arrays. */
   for (ci = 0, compptr = cinfo->comp_info; ci < cinfo->num_components; ci++, compptr++) {
         /* Don't bother to IDCT an uninteresting component. */
         if (! compptr->component_needed)
            continue;
         /* Align the virtual buffer for this component. */
         buffer = (*cinfo->mem.access_virt_barray) (cinfo, coef.whole_image[ci], cinfo->output_iMCU_row * compptr->v_samp_factor, (JDIMENSION) compptr->v_samp_factor, false);
         if(!buffer)
            return JPEG_ERROR;
         /* Count non-dummy DCT block rows in this iMCU row. */
         if (cinfo->output_iMCU_row < last_iMCU_row)
            block_rows = compptr->v_samp_factor;
         else {
            /* NB: can't use last_row_height here; it is input-side-dependent! */
            block_rows = (int) (compptr->height_in_blocks % compptr->v_samp_factor);
            if (block_rows == 0) block_rows = compptr->v_samp_factor;
         }
         inverse_DCT_method_ptr inverse_DCT = cinfo->idct.inverse_DCT[ci];
         output_ptr = output_buf[ci];
         /* Loop over all DCT blocks to be processed. */
         for (block_row = 0; block_row < block_rows; block_row++) {
            buffer_ptr = buffer[block_row];
            output_col = 0;
            for (block_num = 0; block_num < compptr->width_in_blocks; block_num++) {
               (*inverse_DCT) (cinfo, compptr, (JCOEFPTR) buffer_ptr, output_ptr, output_col);
               buffer_ptr++;
               output_col += compptr->DCT_scaled_size;
            }
            output_ptr += compptr->DCT_scaled_size;
         }
   }

   if (++(cinfo->output_iMCU_row) < cinfo->total_iMCU_rows)
      return JPEG_ROW_COMPLETED;
   return JPEG_SCAN_COMPLETED;
}

#endif //D_MULTISCAN_FILES_SUPPORTED

//----------------------------
/*
 * Initialize coefficient buffer controller.
 */
static bool jinit_d_coef_controller (jpeg_decompress_struct* cinfo, bool need_full_buffer){
   jpeg_decompress_struct::my_coef_controller &coef = cinfo->coef;

   /* Create the coefficient buffer. */
   if (need_full_buffer) {
#ifdef D_MULTISCAN_FILES_SUPPORTED
      /* Allocate a full-image virtual array for each component, */
      /* padded to a multiple of samp_factor DCT blocks in each direction. */
      /* Note we ask for a pre-zeroed array. */
      jpeg_component_info *compptr;
      int ci;

      for(ci = 0, compptr = cinfo->comp_info; ci < cinfo->num_components; ci++, compptr++){
         int access_rows = compptr->v_samp_factor;
         coef.whole_image[ci] = (*cinfo->mem.request_virt_barray) (cinfo, true, (JDIMENSION) jround_up(compptr->width_in_blocks, compptr->h_samp_factor), (JDIMENSION) jround_up((long) compptr->height_in_blocks, (long) compptr->v_samp_factor), (JDIMENSION) access_rows);
         if(!coef.whole_image[ci])
            return false;
      }
      coef.decompress_data = decompress_data;
#else
      E RREXIT(cinfo, JERR_NOT_COMPILED);
#endif
   } else {
      /* We only need a single-MCU buffer. */
      int i;
      JBLOCKROW buffer = (JBLOCKROW)cinfo->mem.AllocSmall(D_MAX_BLOCKS_IN_MCU * SIZEOF(JBLOCK));
      if(!buffer)
         return false;
      for (i = 0; i < D_MAX_BLOCKS_IN_MCU; i++) {
         coef.MCU_buffer[i] = buffer + i;
      }
      coef.decompress_data = decompress_onepass;
   }
   return true;
}

//----------------------------

/**************** YCbCr -> RGB conversion: most common case **************/

/*
 * YCbCr is defined per CCIR 601-1, except that Cb and Cr are
 * normalized to the range 0..MAXJSAMPLE rather than -0.5 .. 0.5.
 * The conversion equations to be implemented are therefore
 * R = Y                + 1.40200 * Cr
 * G = Y - 0.34414 * Cb - 0.71414 * Cr
 * B = Y + 1.77200 * Cb
 * where Cb and Cr represent the incoming values less CENTERJSAMPLE.
 * (These numbers are derived from TIFF 6.0 section 21, dated 3-June-92.)
 *
 * To avoid floating-point arithmetic, we represent the fractional constants
 * as integers scaled up by 2^16 (about 4 digits precision); we have to divide
 * the products by 2^16, with appropriate rounding, to get the correct answer.
 * Notice that Y, being an integral input, does not contribute any fraction
 * so it need not participate in the rounding.
 *
 * For even more speed, we avoid doing any multiplications in the inner loop
 * by precalculating the constants times Cb and Cr for all possible values.
 * For 8-bit JSAMPLEs this is very reasonable (only 256 entries per table);
 * for 12-bit samples it is still acceptable.  It's not very reasonable for
 * 16-bit samples, but if you want lossless storage you shouldn't be changing
 * colorspace anyway.
 * The Cr=>R and Cb=>B values can be rounded to integers in advance; the
 * values for the G calculation are left scaled up, since we must add them
 * together before rounding.
 */

#define SCALEBITS 16 /* speediest right-shift on some machines */
#define ONE_HALF  ((int) 1 << (SCALEBITS-1))
#define FIX1(x)    ((int) ((x) * (1L<<SCALEBITS) + 0.5))


/*
 * Initialize tables for YCC->RGB colorspace conversion.
 */

static bool build_ycc_rgb_table (jpeg_decompress_struct* cinfo){
   jpeg_decompress_struct::my_color_deconverter &cconvert = cinfo->cconvert;

   cconvert.Cr_r_tab = new int[(MAXJSAMPLE+1)*4];
   if(!cconvert.Cr_r_tab)
      return false;
   cconvert.Cb_b_tab = cconvert.Cr_r_tab+256;
   cconvert.Cr_g_tab = cconvert.Cb_b_tab+256;
   cconvert.Cb_g_tab = cconvert.Cr_g_tab+256;
   for(int i = 0, x = -CENTERJSAMPLE; i <= MAXJSAMPLE; i++, x++) {
      /* i is the actual input pixel value, in the range 0..MAXJSAMPLE */
      /* The Cb or Cr value we are thinking of is x = i - CENTERJSAMPLE */
      /* Cr=>R value is nearest int to 1.40200 * x */
      cconvert.Cr_r_tab[i] = (int)RIGHT_SHIFT(FIX1(1.40200) * x + ONE_HALF, SCALEBITS);
      /* Cb=>B value is nearest int to 1.77200 * x */
      cconvert.Cb_b_tab[i] = (int)RIGHT_SHIFT(FIX1(1.77200) * x + ONE_HALF, SCALEBITS);
      /* Cr=>G value is scaled-up -0.71414 * x */
      cconvert.Cr_g_tab[i] = (- FIX1(0.71414)) * x;
      /* Cb=>G value is scaled-up -0.34414 * x */
      /* We also add in ONE_HALF so that need not do it in inner loop */
      cconvert.Cb_g_tab[i] = (- FIX1(0.34414)) * x + ONE_HALF;
   }
   return true;
}

//----------------------------
/*
 * Convert some rows of samples to the output colorspace.
 *
 * Note that we change from noninterleaved, one-plane-per-component format
 * to interleaved-pixel format.  The output buffer is therefore three times
 * as wide as the input buffer.
 * A starting row offset is provided only for the input buffer.  The caller
 * can easily adjust the passed output_buf value to accommodate any row
 * offset required on that side.
 */
static void ycc_rgb_convert (jpeg_decompress_struct* cinfo, JSAMPIMAGE input_buf, JDIMENSION input_row, JSAMPARRAY output_buf, int num_rows){

   jpeg_decompress_struct::my_color_deconverter &cconvert = cinfo->cconvert;
   int y, cb, cr;
   JSAMPROW outptr;
   JSAMPROW inptr0, inptr1, inptr2;
   JDIMENSION col;
   JDIMENSION num_cols = cinfo->output_width;
  /* copy these pointers into registers if possible */
   JSAMPLE * range_limit = cinfo->sample_range_limit;
   int * Crrtab = cconvert.Cr_r_tab;
   int * Cbbtab = cconvert.Cb_b_tab;
   int * Crgtab = cconvert.Cr_g_tab;
   int * Cbgtab = cconvert.Cb_g_tab;

   while(--num_rows >= 0){
      inptr0 = input_buf[0][input_row];
      inptr1 = input_buf[1][input_row];
      inptr2 = input_buf[2][input_row];
      input_row++;
      outptr = *output_buf++;
      for (col = 0; col < num_cols; col++) {
         y  = GETJSAMPLE(inptr0[col]);
         cb = GETJSAMPLE(inptr1[col]);
         cr = GETJSAMPLE(inptr2[col]);
      /* Range-limiting is essential due to noise introduced by DCT losses. */
         outptr[RGB_RED] =   range_limit[y + Crrtab[cr]];
         outptr[RGB_GREEN] = range_limit[y +
            ((int) RIGHT_SHIFT(Cbgtab[cb] + Crgtab[cr],
            SCALEBITS))];
         outptr[RGB_BLUE] =  range_limit[y + Cbbtab[cb]];
         outptr += RGB_PIXELSIZE;
      }
   }
}

//----------------------------
/**************** Cases other than YCbCr -> RGB **************/


/*
 * Color conversion for no colorspace change: just copy the data,
 * converting from separate-planes to interleaved representation.
 */

static void null_convert(jpeg_decompress_struct* cinfo, JSAMPIMAGE input_buf, JDIMENSION input_row, JSAMPARRAY output_buf, int num_rows){
   JSAMPROW inptr, outptr;
   JDIMENSION count;
   int num_components = cinfo->num_components;
   JDIMENSION num_cols = cinfo->output_width;
   int ci;

   while (--num_rows >= 0) {
      for (ci = 0; ci < num_components; ci++) {
         inptr = input_buf[ci][input_row];
         outptr = output_buf[0] + ci;
         for (count = num_cols; count > 0; count--) {
            *outptr = *inptr++;  /* needn't bother with GETJSAMPLE() here */
            outptr += num_components;
         }
      }
      input_row++;
      output_buf++;
   }
}

//----------------------------

static void null_convert1(jpeg_decompress_struct* cinfo, JSAMPIMAGE input_buf, JDIMENSION input_row, JSAMPARRAY output_buf, int num_rows){
   JSAMPROW inptr, outptr;
   JDIMENSION count;
   int num_components = cinfo->num_components;
   JDIMENSION num_cols = cinfo->output_width;
   int ci;

   while (--num_rows >= 0) {
      for (ci = 0; ci < num_components; ci++) {
         inptr = input_buf[ci][input_row];
         outptr = output_buf[0] + ci;
         for (count = num_cols; count > 0; count--) {
            *outptr = *inptr++;  /* needn't bother with GETJSAMPLE() here */
            outptr += RGB_PIXELSIZE;
         }
      }
      input_row++;
      output_buf++;
   }
}


/*
 * Color conversion for grayscale: just copy the data.
 * This also works for YCbCr -> grayscale conversion, in which
 * we just copy the Y (luminance) component and ignore chrominance.
 */

static void grayscale_convert (jpeg_decompress_struct* cinfo, JSAMPIMAGE input_buf, JDIMENSION input_row, JSAMPARRAY output_buf, int num_rows){
  jcopy_sample_rows(input_buf[0], (int) input_row, output_buf, 0, num_rows, cinfo->output_width);
}


/*
 * Adobe-style YCCK->CMYK conversion.
 * We convert YCbCr to R=1-C, G=1-M, and B=1-Y using the same
 * conversion as above, while passing K (black) unchanged.
 * We assume build_ycc_rgb_table has been called.
 */

static void ycck_cmyk_convert (jpeg_decompress_struct* cinfo, JSAMPIMAGE input_buf, JDIMENSION input_row, JSAMPARRAY output_buf, int num_rows){
   jpeg_decompress_struct::my_color_deconverter &cconvert = cinfo->cconvert;
   int y, cb, cr;
   JSAMPROW outptr;
   JSAMPROW inptr0, inptr1, inptr2, inptr3;
   JDIMENSION col;
   JDIMENSION num_cols = cinfo->output_width;
   /* copy these pointers into registers if possible */
   JSAMPLE * range_limit = cinfo->sample_range_limit;
   int * Crrtab = cconvert.Cr_r_tab;
   int * Cbbtab = cconvert.Cb_b_tab;
   int * Crgtab = cconvert.Cr_g_tab;
   int * Cbgtab = cconvert.Cb_g_tab;

   while (--num_rows >= 0) {
      inptr0 = input_buf[0][input_row];
      inptr1 = input_buf[1][input_row];
      inptr2 = input_buf[2][input_row];
      inptr3 = input_buf[3][input_row];
      input_row++;
      outptr = *output_buf++;
      for (col = 0; col < num_cols; col++) {
         y  = GETJSAMPLE(inptr0[col]);
         cb = GETJSAMPLE(inptr1[col]);
         cr = GETJSAMPLE(inptr2[col]);
         /* Range-limiting is essential due to noise introduced by DCT losses. */
         outptr[0] = range_limit[MAXJSAMPLE - (y + Crrtab[cr])];  /* red */
         outptr[1] = range_limit[MAXJSAMPLE - (y +       /* green */
            ((int) RIGHT_SHIFT(Cbgtab[cb] + Crgtab[cr],
            SCALEBITS)))];
         outptr[2] = range_limit[MAXJSAMPLE - (y + Cbbtab[cb])];  /* blue */
         /* K passes through unchanged */
         outptr[3] = inptr3[col];   /* don't need GETJSAMPLE here */
         outptr += 4;
      }
   }
}

//----------------------------

/*
 * Module initialization routine for output colorspace conversion.
 */
static bool jinit_color_deconverter (jpeg_decompress_struct* cinfo){

   int ci;
   jpeg_decompress_struct::my_color_deconverter &cconvert = cinfo->cconvert;

   /* Make sure num_components agrees with jpeg_color_space */
   switch (cinfo->jpeg_color_space) {
   case JCS_GRAYSCALE:
      if (cinfo->num_components != 1)
         return false;//E RREXIT(cinfo, JERR_BAD_J_COLORSPACE);
      break;

   case JCS_RGB:
   case JCS_YCbCr:
      if (cinfo->num_components != 3)
         return false;//E RREXIT(cinfo, JERR_BAD_J_COLORSPACE);
      break;

   case JCS_CMYK:
   case JCS_YCCK:
      if (cinfo->num_components != 4)
         return false;//E RREXIT(cinfo, JERR_BAD_J_COLORSPACE);
      break;

   default:        /* JCS_UNKNOWN can be anything */
      if (cinfo->num_components < 1)
         return false;//E RREXIT(cinfo, JERR_BAD_J_COLORSPACE);
      break;
   }

   /* Set out_color_components and conversion method based on requested space.
   * Also clear the component_needed flags for any unused components,
   * so that earlier pipeline stages can avoid useless computation.
   */

   switch (cinfo->out_color_space) {
   case JCS_GRAYSCALE:
      cinfo->out_color_components = 1;
      if (cinfo->jpeg_color_space == JCS_GRAYSCALE ||
         cinfo->jpeg_color_space == JCS_YCbCr) {
            cconvert.color_convert = grayscale_convert;
            /* For color->grayscale conversion, only the Y (0) component is needed */
            for (ci = 1; ci < cinfo->num_components; ci++)
               cinfo->comp_info[ci].component_needed = false;
      } else
         return false;//E RREXIT(cinfo, JERR_CONVERSION_NOTIMPL);
      break;

   case JCS_RGB:
      cinfo->out_color_components = RGB_PIXELSIZE;
      if (cinfo->jpeg_color_space == JCS_YCbCr) {
         cconvert.color_convert = ycc_rgb_convert;
         build_ycc_rgb_table(cinfo);
      } else if (cinfo->jpeg_color_space == JCS_RGB && RGB_PIXELSIZE == 4)
         cconvert.color_convert = null_convert1;
      else
         return false;//E RREXIT(cinfo, JERR_CONVERSION_NOTIMPL);
      break;

   case JCS_CMYK:
      cinfo->out_color_components = 4;
      if (cinfo->jpeg_color_space == JCS_YCCK) {
         cconvert.color_convert = ycck_cmyk_convert;
         build_ycc_rgb_table(cinfo);
      } else if (cinfo->jpeg_color_space == JCS_CMYK) {
         cconvert.color_convert = null_convert;
      } else
         return false;//E RREXIT(cinfo, JERR_CONVERSION_NOTIMPL);
      break;

   default:
      /* Permit null conversion to same output space */
      if (cinfo->out_color_space == cinfo->jpeg_color_space) {
         cinfo->out_color_components = cinfo->num_components;
         cconvert.color_convert = null_convert;
      } else        /* unsupported non-null conversion */
         return false;//E RREXIT(cinfo, JERR_CONVERSION_NOTIMPL);
      break;
   }
   return true;
}

//----------------------------
/* The current scaled-IDCT routines require ISLOW-style multiplier tables,
 * so be sure to compile that code if either ISLOW or SCALING is requested.
 */
#ifdef DCT_ISLOW_SUPPORTED
#define PROVIDE_ISLOW_TABLES
#else
#ifdef IDCT_SCALING_SUPPORTED
#define PROVIDE_ISLOW_TABLES
#endif
#endif

static void jpeg_idct_ifast(jpeg_decompress_struct* cinfo, jpeg_component_info * compptr, JCOEFPTR coef_block, JSAMPARRAY output_buf, JDIMENSION output_col);
static void jpeg_idct_4x4 (jpeg_decompress_struct* cinfo, jpeg_component_info * compptr, JCOEFPTR coef_block, JSAMPARRAY output_buf, JDIMENSION output_col);
static void jpeg_idct_2x2 (jpeg_decompress_struct* cinfo, jpeg_component_info * compptr, JCOEFPTR coef_block, JSAMPARRAY output_buf, JDIMENSION output_col);
static void jpeg_idct_1x1 (jpeg_decompress_struct* cinfo, jpeg_component_info * compptr, JCOEFPTR coef_block, JSAMPARRAY output_buf, JDIMENSION output_col);


/*
 * Prepare for an output pass.
 * Here we select the proper IDCT routine for each component and build
 * a matching multiplier table.
 */

bool jpeg_decompress_struct::my_idct_controller::start_pass(jpeg_decompress_struct* cinfo){

   int ci, i;
   jpeg_component_info *compptr;
   int method = 0;
   inverse_DCT_method_ptr method_ptr = NULL;
   JQUANT_TBL * qtbl;

   for(ci = 0, compptr = cinfo->comp_info; ci < cinfo->num_components; ci++, compptr++){
      //Select the proper IDCT routine for this component's scaling
      switch(compptr->DCT_scaled_size){
#ifdef IDCT_SCALING_SUPPORTED
      case 1:
         method_ptr = jpeg_idct_1x1;
         method = JDCT_ISLOW; /* jidctred uses islow-style table */
         break;
      case 2:
         method_ptr = jpeg_idct_2x2;
         method = JDCT_ISLOW; /* jidctred uses islow-style table */
         break;
      case 4:
         method_ptr = jpeg_idct_4x4;
         method = JDCT_ISLOW; /* jidctred uses islow-style table */
         break;
#endif
      case DCTSIZE:
         method_ptr = jpeg_idct_ifast;
         method = JDCT_IFAST;
         /*
         switch (cinfo->dct_method) {
         case JDCT_ISLOW:
#ifdef DCT_ISLOW_SUPPORTED
            method_ptr = jpeg_idct_islow;
            method = JDCT_ISLOW;
#else
            method_ptr = jpeg_idct_ifast;
            method = JDCT_IFAST;
#endif
            break;
#ifdef DCT_IFAST_SUPPORTED
         case JDCT_IFAST:
            method_ptr = jpeg_idct_ifast;
            method = JDCT_IFAST;
            break;
#endif
         default:
            return false;//E RREXIT(cinfo, JERR_NOT_COMPILED);
            //break;
         }
         */
         break;
      default:
         return false;//E RREXIT1(cinfo, JERR_BAD_DCTSIZE, compptr->DCT_scaled_size);
      }
      inverse_DCT[ci] = method_ptr;
      /* Create multiplier table from quant table.
      * However, we can skip this if the component is uninteresting
      * or if we already built the table.  Also, if no quant table
      * has yet been saved for the component, we leave the
      * multiplier table all-zero; we'll be reading zeroes from the
      * coefficient controller's buffer anyway.
      */
      if(! compptr->component_needed || cur_method[ci] == method)
         continue;
      qtbl = compptr->quant_table;
      if (qtbl == NULL)      /* happens if no data yet for component */
         continue;
      cur_method[ci] = method;
      switch (method){
#ifdef PROVIDE_ISLOW_TABLES
      case JDCT_ISLOW:
         {
            /* For LL&M IDCT method, multipliers are equal to raw quantization
            * coefficients, but are stored as ints to ensure access efficiency.
            */
            ISLOW_MULT_TYPE * ismtbl = compptr->dct_table.islow_array;
            for (i = 0; i < DCTSIZE2; i++) {
               ismtbl[i] = (ISLOW_MULT_TYPE) qtbl->quantval[i];
            }
         }
         break;
#endif
#ifdef DCT_IFAST_SUPPORTED
      case JDCT_IFAST:
         {
            /* For AA&N IDCT method, multipliers are equal to quantization
            * coefficients scaled by scalefactor[row]*scalefactor[col], where
            *   scalefactor[0] = 1
            *   scalefactor[k] = cos(k*PI/16) * sqrt(2)    for k=1..7
            * For integer operation, the multiplier table is to be scaled by
            * IFAST_SCALE_BITS.
            */
            IFAST_MULT_TYPE * ifmtbl = compptr->dct_table.ifast_array;
#define CONST_BITS 14
            static const short aanscales[DCTSIZE2] = {
               /* precomputed values scaled up by 14 bits */
               16384, 22725, 21407, 19266, 16384, 12873,  8867,  4520,
               22725, 31521, 29692, 26722, 22725, 17855, 12299,  6270,
               21407, 29692, 27969, 25172, 21407, 16819, 11585,  5906,
               19266, 26722, 25172, 22654, 19266, 15137, 10426,  5315,
               16384, 22725, 21407, 19266, 16384, 12873,  8867,  4520,
               12873, 17855, 16819, 15137, 12873, 10114,  6967,  3552,
               8867, 12299, 11585, 10426,  8867,  6967,  4799,  2446,
               4520,  6270,  5906,  5315,  4520,  3552,  2446,  1247
            };
               for (i = 0; i < DCTSIZE2; i++) {
                  ifmtbl[i] = (IFAST_MULT_TYPE)
                     DESCALE(MULTIPLY16V16((int) qtbl->quantval[i],
                     (int) aanscales[i]),
                     CONST_BITS-IFAST_SCALE_BITS);
               }
         }
       break;
#endif
    default:
       return false;//E RREXIT(cinfo, JERR_NOT_COMPILED);
       //break;
      }
   }
   return true;
}

//----------------------------



/* This macro is to work around compilers with missing or broken
 * structure assignment.  You'll need to fix this code if you have
 * such a compiler and you change MAX_COMPS_IN_SCAN.
 */

#ifndef NO_STRUCT_ASSIGN
#define ASSIGN_STATE(dest,src)  ((dest) = (src))
#else
#if MAX_COMPS_IN_SCAN == 4
#define ASSIGN_STATE(dest,src)  \
   ((dest).last_dc_val[0] = (src).last_dc_val[0], \
    (dest).last_dc_val[1] = (src).last_dc_val[1], \
    (dest).last_dc_val[2] = (src).last_dc_val[2], \
    (dest).last_dc_val[3] = (src).last_dc_val[3])
#endif
#endif

//----------------------------
/*
 * Compute the derived values for a Huffman table.
 * Note this is also used by jdphuff.c.
 */
static bool jpeg_make_d_derived_tbl(jpeg_decompress_struct* cinfo, JHUFF_TBL * htbl, d_derived_tbl &pdtbl){

   int p, i, l, si;
   int lookbits, ctr;
   char huffsize[257];
   dword huffcode[257];
   dword code;

   d_derived_tbl *dtbl = &pdtbl;
   dtbl->_pub = htbl;     /* fill in back link */

   /* Figure C.1: make table of Huffman code length for each symbol */
   /* Note that this is in code-length order. */
   p = 0;
   for (l = 1; l <= 16; l++) {
      for (i = 1; i <= (int) htbl->bits[l]; i++)
         huffsize[p++] = (char) l;
   }
   huffsize[p] = 0;

   /* Figure C.2: generate the codes themselves */
   /* Note that this is in code-length order. */
   code = 0;
   si = huffsize[0];
   p = 0;
   while (huffsize[p]) {
      while (((int) huffsize[p]) == si) {
         huffcode[p++] = code;
         code++;
      }
      code <<= 1;
      si++;
   }
   /* Figure F.15: generate decoding tables for bit-sequential decoding */
   p = 0;
   for (l = 1; l <= 16; l++) {
      if (htbl->bits[l]) {
         dtbl->valptr[l] = p; /* huffval[] index of 1st symbol of code length l */
         dtbl->mincode[l] = huffcode[p]; /* minimum code of length l */
         p += htbl->bits[l];
         dtbl->maxcode[l] = huffcode[p-1]; /* maximum code of length l */
      } else {
         dtbl->maxcode[l] = -1;  /* -1 if no codes of this length */
      }
   }
   dtbl->maxcode[17] = 0xFFFFFL; /* ensures jpeg_huff_decode terminates */

   /* Compute lookahead tables to speed up decoding.
   * First we set all the table entries to 0, indicating "too long";
   * then we iterate through the Huffman codes that are short enough and
   * fill in all the entries that correspond to bit sequences starting
   * with that code.
   */
   MemSet(dtbl->look_nbits, 0, SIZEOF(dtbl->look_nbits));

   p = 0;
   for (l = 1; l <= HUFF_LOOKAHEAD; l++) {
      for (i = 1; i <= (int) htbl->bits[l]; i++, p++) {
         /* l = current code's length, p = its index in huffcode[] & huffval[]. */
         /* Generate left-justified code followed by all possible bit sequences */
         lookbits = huffcode[p] << (HUFF_LOOKAHEAD-l);
         for (ctr = 1 << (HUFF_LOOKAHEAD-l); ctr > 0; ctr--) {
            dtbl->look_nbits[lookbits] = l;
            dtbl->look_sym[lookbits] = htbl->huffval[p];
            lookbits++;
         }
      }
   }
   return true;
}

//----------------------------

/*
 * Initialize for a Huffman-compressed scan.
 */

static bool start_pass_huff_decoder (jpeg_decompress_struct* cinfo){
   huff_entropy_decoder* entropy = cinfo->entropy;
   int ci, dctbl, actbl;
   jpeg_component_info * compptr;

   /* Check that the scan parameters Ss, Se, Ah/Al are OK for sequential JPEG.
   * This ought to be an error condition, but we make it a warning because
   * there are some baseline files out there with all zeroes in these bytes.
   */
   for (ci = 0; ci < cinfo->comps_in_scan; ci++) {
      compptr = cinfo->cur_comp_info[ci];
      dctbl = compptr->dc_tbl_no;
      actbl = compptr->ac_tbl_no;
      /* Make sure requested tables are present */
      if (dctbl < 0 || dctbl >= NUM_HUFF_TBLS || cinfo->dc_huff_tbl_ptrs[dctbl] == NULL)
         return false;//E RREXIT1(cinfo, JERR_NO_HUFF_TABLE, dctbl);
      if (actbl < 0 || actbl >= NUM_HUFF_TBLS || cinfo->ac_huff_tbl_ptrs[actbl] == NULL)
         return false;//E RREXIT1(cinfo, JERR_NO_HUFF_TABLE, actbl);
      /* Compute derived values for Huffman tables */
      /* We may do this more than once for a table, but it's not expensive */
      if(!jpeg_make_d_derived_tbl(cinfo, cinfo->dc_huff_tbl_ptrs[dctbl], entropy->dc_derived_tbls[dctbl]))
         return false;
      if(!jpeg_make_d_derived_tbl(cinfo, cinfo->ac_huff_tbl_ptrs[actbl], entropy->ac_derived_tbls[actbl]))
         return false;
      /* Initialize DC predictions to 0 */
      entropy->saved.last_dc_val[ci] = 0;
   }
   /* Initialize bitread state variables */
   entropy->bitstate.bits_left = 0;
   entropy->bitstate.get_buffer = 0; /* unnecessary, but keeps Purify quiet */
   entropy->bitstate.printed_eod = false;

   /* Initialize restart counter */
   entropy->restarts_to_go = cinfo->restart_interval;
   return true;
}

/*
 * Out-of-line code for bit fetching (shared with jdphuff.c).
 * See jdhuff.h for info about usage.
 * Note: current values of get_buffer and bits_left are passed as parameters,
 * but are returned in the corresponding fields of the state struct.
 *
 * On most machines MIN_GET_BITS should be 25 to allow the full 32-bit width
 * of get_buffer to be used.  (On machines with wider words, an even larger
 * buffer could be used.)  However, on some machines 32-bit shifts are
 * quite slow and take time proportional to the number of places shifted.
 * (This is true with most PC compilers, for instance.)  In this case it may
 * be a win to set MIN_GET_BITS to the minimum value of 15.  This reduces the
 * average shift distance at the cost of more calls to jpeg_fill_bit_buffer.
 */
#define BIT_BUF_SIZE  32   /* size of buffer in bits */

#ifdef SLOW_SHIFT_32
#define MIN_GET_BITS  15   /* minimum allowable value */
#else
#define MIN_GET_BITS  (BIT_BUF_SIZE-7)
#endif

//----------------------------
struct bitread_working_state{      /* Bitreading working state within an MCU */
  /* current data source state */
  const JOCTET * next_input_byte; /* => next byte to read from source */
  size_t bytes_in_buffer;   /* # of bytes remaining in source buffer */
  int unread_marker;      /* nonzero if we have hit a marker */
  /* bit input buffer --- note these values are kept in register variables,
   * not in this struct, inside the inner loops.
   */
  bit_buf_type get_buffer;   /* current bit-extraction buffer */
  int bits_left;      /* # of unused bits in it */
  /* pointers needed by jpeg_fill_bit_buffer */
  jpeg_decompress_struct* cinfo;   /* back link to decompress master record */
  bool * printed_eod_ptr;   /* => flag in permanent state */
};

//----------------------------

static bool jpeg_fill_bit_buffer (bitread_working_state * state, bit_buf_type get_buffer, int bits_left, int nbits){
   /* Load up the bit buffer to a depth of at least nbits */
   /* Copy heavily used state fields into locals (hopefully registers) */
   const JOCTET * next_input_byte = state->next_input_byte;
   size_t bytes_in_buffer = state->bytes_in_buffer;
   int c;

   /* Attempt to load at least MIN_GET_BITS bits into get_buffer. */
   /* (It is assumed that no request will be for more than that many bits.) */

   while (bits_left < MIN_GET_BITS) {
      /* Attempt to read a byte */
      if (state->unread_marker != 0)
         goto no_more_data;   /* can't advance past a marker */

      if (bytes_in_buffer == 0) {
         if (!state->cinfo->src.fill_input_buffer())
            return false;
         next_input_byte = state->cinfo->src.next_input_byte;
         bytes_in_buffer = state->cinfo->src.bytes_in_buffer;
      }
      bytes_in_buffer--;
      c = GETJOCTET(*next_input_byte++);

      /* If it's 0xFF, check and discard stuffed zero byte */
      if (c == 0xFF) {
         do {
            if (bytes_in_buffer == 0) {
               if (!state->cinfo->src.fill_input_buffer())
                  return false;
               next_input_byte = state->cinfo->src.next_input_byte;
               bytes_in_buffer = state->cinfo->src.bytes_in_buffer;
            }
            bytes_in_buffer--;
            c = GETJOCTET(*next_input_byte++);
         } while (c == 0xFF);

         if (c == 0) {
            /* Found FF/00, which represents an FF data byte */
            c = 0xFF;
         } else {
            /* Oops, it's actually a marker indicating end of compressed data. */
            /* Better put it back for use later */
            state->unread_marker = c;

no_more_data:
            /* There should be enough bits still left in the data segment; */
            /* if so, just break out of the outer while loop. */
            if (bits_left >= nbits)
               break;
            /* Uh-oh.  Report corrupted data to user and stuff zeroes into
            * the data stream, so that we can produce some kind of image.
            * Note that this code will be repeated for each byte demanded
            * for the rest of the segment.  We use a nonvolatile flag to ensure
            * that only one warning message appears.
            */
            if (! *(state->printed_eod_ptr)) {
               *(state->printed_eod_ptr) = true;
            }
            c = 0;         /* insert a zero byte into bit buffer */
         }
      }

      /* OK, load c into get_buffer */
      get_buffer = (get_buffer << 8) | c;
      bits_left += 8;
   }

   /* Unload the local registers */
   state->next_input_byte = next_input_byte;
   state->bytes_in_buffer = bytes_in_buffer;
   state->get_buffer = get_buffer;
   state->bits_left = bits_left;

   return true;
}

//----------------------------

/*
 * These macros provide the in-line portion of bit fetching.
 * Use CHECK_BIT_BUFFER to ensure there are N bits in get_buffer
 * before using GET_BITS, PEEK_BITS, or DROP_BITS.
 * The variables get_buffer and bits_left are assumed to be locals,
 * but the state struct might not be (jpeg_huff_decode needs this).
 *   CHECK_BIT_BUFFER(state,n,action);
 *      Ensure there are N bits in get_buffer; if suspend, take action.
 *      val = GET_BITS(n);
 *      Fetch next N bits.
 *      val = PEEK_BITS(n);
 *      Fetch next N bits without removing them from the buffer.
 *   DROP_BITS(n);
 *      Discard next N bits.
 * The value N should be a simple variable, not an expression, because it
 * is evaluated multiple times.
 */

#define CHECK_BIT_BUFFER(state,nbits,action) \
   { if (bits_left < (nbits)) {  \
       if (! jpeg_fill_bit_buffer(&(state),get_buffer,bits_left,nbits))  \
         { action; }  \
       get_buffer = (state).get_buffer; bits_left = (state).bits_left; } }

#define GET_BITS(nbits) \
   (((int) (get_buffer >> (bits_left -= (nbits)))) & ((1<<(nbits))-1))

#define PEEK_BITS(nbits) \
   (((int) (get_buffer >> (bits_left -  (nbits)))) & ((1<<(nbits))-1))

#define DROP_BITS(nbits) \
   (bits_left -= (nbits))

//----------------------------

/*
 * Out-of-line code for Huffman code decoding.
 * See jdhuff.h for info about usage.
 */

static int jpeg_huff_decode (bitread_working_state * state, bit_buf_type get_buffer, int bits_left, d_derived_tbl * htbl, int min_bits){
  int l = min_bits;
  int code;

  /* HUFF_DECODE has determined that the code is at least min_bits */
  /* bits long, so fetch that many bits in one swoop. */

  CHECK_BIT_BUFFER(*state, l, return -1);
  code = GET_BITS(l);

  /* Collect the rest of the Huffman code one bit at a time. */
  /* This is per Figure F.16 in the JPEG spec. */

  while (code > htbl->maxcode[l]) {
    code <<= 1;
    CHECK_BIT_BUFFER(*state, 1, return -1);
    code |= GET_BITS(1);
    l++;
  }

  /* Unload the local registers */
  state->get_buffer = get_buffer;
  state->bits_left = bits_left;

  /* With garbage input we may reach the sentinel value l = 17. */

  if (l > 16) {
    return 0;        /* fake a zero as the safest result */
  }

   return htbl->_pub->huffval[ htbl->valptr[l] + ((int) (code - htbl->mincode[l])) ];
}


/*
 * Figure F.12: extend sign bit.
 * On some machines, a shift and add will be faster than a table lookup.
 */

#ifdef AVOID_TABLES

#define HUFF_EXTEND(x,s)  ((x) < (1<<((s)-1)) ? (x) + (((-1)<<(s)) + 1) : (x))

#else

#define HUFF_EXTEND(x,s)  ((x) < extend_test[s] ? (x) + extend_offset[s] : (x))

static const int extend_test[16] =   /* entry n is 2**(n-1) */
  { 0, 0x0001, 0x0002, 0x0004, 0x0008, 0x0010, 0x0020, 0x0040, 0x0080,
    0x0100, 0x0200, 0x0400, 0x0800, 0x1000, 0x2000, 0x4000 };

static const int extend_offset[16] = /* entry n is (-1 << n) + 1 */
  { 0, ((-1)<<1) + 1, ((-1)<<2) + 1, ((-1)<<3) + 1, ((-1)<<4) + 1,
    ((-1)<<5) + 1, ((-1)<<6) + 1, ((-1)<<7) + 1, ((-1)<<8) + 1,
    ((-1)<<9) + 1, ((-1)<<10) + 1, ((-1)<<11) + 1, ((-1)<<12) + 1,
    ((-1)<<13) + 1, ((-1)<<14) + 1, ((-1)<<15) + 1 };

#endif /* AVOID_TABLES */


/*
 * Check for a restart marker & resynchronize decoder.
 * Returns false if must suspend.
 */

static bool
process_restart (jpeg_decompress_struct* cinfo)
{
  huff_entropy_decoder* entropy = cinfo->entropy;
  int ci;

  /* Throw away any unused bits remaining in bit buffer; */
  /* include any full bytes in next_marker's count of discarded bytes */
  cinfo->marker.discarded_bytes += entropy->bitstate.bits_left / 8;
  entropy->bitstate.bits_left = 0;

  /* Advance past the RSTn marker */
  if (!cinfo->marker.read_restart_marker(cinfo))
    return false;

  /* Re-initialize DC predictions to 0 */
  for (ci = 0; ci < cinfo->comps_in_scan; ci++)
    entropy->saved.last_dc_val[ci] = 0;

  /* Reset restart counter */
  entropy->restarts_to_go = cinfo->restart_interval;

  /* Next segment can get another out-of-data warning */
  entropy->bitstate.printed_eod = false;

  return true;
}

//----------------------------


//----------------------------

/*
 * Code for extracting next Huffman-coded symbol from input bit stream.
 * Again, this is time-critical and we make the main paths be macros.
 *
 * We use a lookahead table to process codes of up to HUFF_LOOKAHEAD bits
 * without looping.  Usually, more than 95% of the Huffman codes will be 8
 * or fewer bits long.  The few overlength codes are handled with a loop,
 * which need not be inline code.
 *
 * Notes about the HUFF_DECODE macro:
 * 1. Near the end of the data segment, we may fail to get enough bits
 *    for a lookahead.  In that case, we do it the hard way.
 * 2. If the lookahead table contains no entry, the next code must be
 *    more than HUFF_LOOKAHEAD bits long.
 * 3. jpeg_huff_decode returns -1 if forced to suspend.
 */

#define HUFF_DECODE(result,state,htbl,failaction,slowlabel){ \
   register int nb, look; \
   if (bits_left < HUFF_LOOKAHEAD) { \
      if (! jpeg_fill_bit_buffer(&state,get_buffer,bits_left, 0)) {failaction;} \
      get_buffer = state.get_buffer; bits_left = state.bits_left; \
      if (bits_left < HUFF_LOOKAHEAD) { \
         nb = 1; goto slowlabel; \
      } \
   } \
   look = PEEK_BITS(HUFF_LOOKAHEAD); \
   if ((nb = htbl->look_nbits[look]) != 0) { \
      DROP_BITS(nb); \
      result = htbl->look_sym[look]; \
   } else { \
      nb = HUFF_LOOKAHEAD+1; \
slowlabel: \
      if ((result=jpeg_huff_decode(&state,get_buffer,bits_left,htbl,nb)) < 0) \
      { failaction; } \
      get_buffer = state.get_buffer; bits_left = state.bits_left; \
   } \
}

//----------------------------

/* Macros to declare and load/save bitread local variables. */
#define BITREAD_STATE_VARS  \
   register bit_buf_type get_buffer;  \
   register int bits_left;  \
   bitread_working_state br_state

#define BITREAD_LOAD_STATE(cinfop,permstate)  \
   br_state.cinfo = cinfop; \
   br_state.next_input_byte = cinfop->src.next_input_byte; \
   br_state.bytes_in_buffer = cinfop->src.bytes_in_buffer; \
   br_state.unread_marker = cinfop->unread_marker; \
   get_buffer = permstate.get_buffer; \
   bits_left = permstate.bits_left; \
   br_state.printed_eod_ptr = & permstate.printed_eod

#define BITREAD_SAVE_STATE(cinfop,permstate)  \
   cinfop->src.next_input_byte = br_state.next_input_byte; \
   cinfop->src.bytes_in_buffer = br_state.bytes_in_buffer; \
   cinfop->unread_marker = br_state.unread_marker; \
   permstate.get_buffer = get_buffer; \
   permstate.bits_left = bits_left


//----------------------------

/*
 * Decode and return one MCU's worth of Huffman-compressed coefficients.
 * The coefficients are reordered from zigzag order into natural array order,
 * but are not dequantized.
 *
 * The i'th block of the MCU is stored into the block pointed to by
 * MCU_data[i].  WE ASSUME THIS AREA HAS BEEN ZEROED BY THE CALLER.
 * (Wholesale zeroing is usually a little faster than retail...)
 *
 * Returns false if data source requested suspension.  In that case no
 * changes have been made to permanent state.  (Exception: some output
 * coefficients may already have been assigned.  This is harmless for
 * this module, since we'll just re-assign them on the next call.)
 */
static bool decode_mcu (jpeg_decompress_struct* cinfo, JBLOCKROW *MCU_data){

   huff_entropy_decoder* entropy = cinfo->entropy;
   int s, k, r;
   int blkn, ci;
   JBLOCKROW block;
   BITREAD_STATE_VARS;
   savable_state state;
   d_derived_tbl * dctbl;
   d_derived_tbl * actbl;
   jpeg_component_info * compptr;

   /* Process restart marker if needed; may have to suspend */
   if (cinfo->restart_interval) {
      if (entropy->restarts_to_go == 0)
         if (! process_restart(cinfo))
            return false;
   }
   /* Load up working state */
   BITREAD_LOAD_STATE(cinfo,entropy->bitstate);
   ASSIGN_STATE(state, entropy->saved);

   /* Outer loop handles each block in the MCU */
   for (blkn = 0; blkn < cinfo->blocks_in_MCU; blkn++) {
      block = MCU_data[blkn];
      ci = cinfo->MCU_membership[blkn];
      compptr = cinfo->cur_comp_info[ci];
      dctbl = &entropy->dc_derived_tbls[compptr->dc_tbl_no];
      actbl = &entropy->ac_derived_tbls[compptr->ac_tbl_no];

      /* Decode a single block's worth of coefficients */

      /* Section F.2.2.1: decode the DC coefficient difference */
      HUFF_DECODE(s, br_state, dctbl, return false, label1);
      if (s) {
         CHECK_BIT_BUFFER(br_state, s, return false);
         r = GET_BITS(s);
         s = HUFF_EXTEND(r, s);
      }
      /* Shortcut if component's values are not interesting */
      if (! compptr->component_needed)
         goto skip_ACs;

      /* Convert DC difference to actual value, update last_dc_val */
      s += state.last_dc_val[ci];
      state.last_dc_val[ci] = s;
      /* Output the DC coefficient (assumes jpeg_natural_order[0] = 0) */
      (*block)[0] = (JCOEF) s;

      /* Do we need to decode the AC coefficients for this component? */
      if (compptr->DCT_scaled_size > 1) {
         /* Section F.2.2.2: decode the AC coefficients */
         /* Since zeroes are skipped, output area must be cleared beforehand */
         for (k = 1; k < DCTSIZE2; k++) {
            HUFF_DECODE(s, br_state, actbl, return false, label2);
            r = s >> 4;
            s &= 15;
            if (s) {
               k += r;
               CHECK_BIT_BUFFER(br_state, s, return false);
               r = GET_BITS(s);
               s = HUFF_EXTEND(r, s);
               /* Output coefficient in natural (dezigzagged) order.
               * Note: the extra entries in jpeg_natural_order[] will save us
               * if k >= DCTSIZE2, which could happen if the data is corrupted.
               */
               (*block)[jpeg_natural_order[k]] = (JCOEF) s;
            } else {
               if (r != 15)
                  break;
               k += 15;
            }
         }
      } else {
skip_ACs:
         /* Section F.2.2.2: decode the AC coefficients */
         /* In this path we just discard the values */
         for (k = 1; k < DCTSIZE2; k++) {
            HUFF_DECODE(s, br_state, actbl, return false, label3);
            r = s >> 4;
            s &= 15;
            if (s) {
               k += r;
               CHECK_BIT_BUFFER(br_state, s, return false);
               DROP_BITS(s);
            } else {
               if (r != 15)
                  break;
               k += 15;
            }
         }
      }
   }
   /* Completed MCU, so update state */
   BITREAD_SAVE_STATE(cinfo,entropy->bitstate);
   ASSIGN_STATE(entropy->saved, state);

   /* Account for restart interval (no-op if not using restarts) */
   entropy->restarts_to_go--;
   return true;
}


/*
 * Module initialization routine for Huffman entropy decoding.
 */

static bool jinit_huff_decoder (jpeg_decompress_struct* cinfo){

   huff_entropy_decoder* entropy = new huff_entropy_decoder;
   if(!entropy)
      return false;
   cinfo->entropy = entropy;
   entropy->start_pass = start_pass_huff_decoder;
   entropy->decode_mcu = decode_mcu;
   return true;
}

//----------------------------

/*
 * Routines to calculate various quantities related to the size of the image.
 */

static bool initial_setup (jpeg_decompress_struct* cinfo){
   /* Called once, when first SOS marker is reached */
   int ci;
   jpeg_component_info *compptr;

   /* Make sure image isn't bigger than I can handle */
   if ((long) cinfo->image_height > (long) JPEG_MAX_DIMENSION || (long) cinfo->image_width > (long) JPEG_MAX_DIMENSION)
      return false;//E RREXIT1(cinfo, JERR_IMAGE_TOO_BIG, (dword) JPEG_MAX_DIMENSION);

   /* For now, precision must match compiled-in value... */
   if (cinfo->data_precision != BITS_IN_JSAMPLE)
      return false;//E RREXIT1(cinfo, JERR_BAD_PRECISION, cinfo->data_precision);

   /* Check that number of components won't exceed internal array sizes */
   if (cinfo->num_components > MAX_COMPONENTS)
      return false;//E RREXIT2(cinfo, JERR_COMPONENT_COUNT, cinfo->num_components, MAX_COMPONENTS);

   /* Compute maximum sampling factors; check factor validity */
   cinfo->max_h_samp_factor = 1;
   cinfo->max_v_samp_factor = 1;
   for (ci = 0, compptr = cinfo->comp_info; ci < cinfo->num_components;
      ci++, compptr++) {
         if (compptr->h_samp_factor<=0 || compptr->h_samp_factor>MAX_SAMP_FACTOR || compptr->v_samp_factor<=0 || compptr->v_samp_factor>MAX_SAMP_FACTOR)
            return false;//E RREXIT(cinfo, JERR_BAD_SAMPLING);
         cinfo->max_h_samp_factor = Max(cinfo->max_h_samp_factor, compptr->h_samp_factor);
         cinfo->max_v_samp_factor = Max(cinfo->max_v_samp_factor, compptr->v_samp_factor);
   }

   /* We initialize DCT_scaled_size and min_DCT_scaled_size to DCTSIZE.
   * In the full decompressor, this will be overridden by jdmaster.c;
   * but in the transcoder, jdmaster.c is not used, so we must do it here.
   */
   cinfo->min_DCT_scaled_size = DCTSIZE;

   /* Compute dimensions of components */
   for (ci = 0, compptr = cinfo->comp_info; ci < cinfo->num_components;
      ci++, compptr++) {
         compptr->DCT_scaled_size = DCTSIZE;
         /* Size in DCT blocks */
         compptr->width_in_blocks = (JDIMENSION)jdiv_round_up((long) cinfo->image_width * (long) compptr->h_samp_factor, (long) (cinfo->max_h_samp_factor * DCTSIZE));
         compptr->height_in_blocks = (JDIMENSION)jdiv_round_up((long) cinfo->image_height * (long) compptr->v_samp_factor, (long) (cinfo->max_v_samp_factor * DCTSIZE));
         /* downsampled_width and downsampled_height will also be overridden by
         * jdmaster.c if we are doing full decompression.  The transcoder library
         * doesn't use these values, but the calling application might.
         */
         /* Size in samples */
         compptr->downsampled_width = (JDIMENSION) jdiv_round_up((long) cinfo->image_width * (long) compptr->h_samp_factor, (long) cinfo->max_h_samp_factor);
         compptr->downsampled_height = (JDIMENSION) jdiv_round_up((long) cinfo->image_height * (long) compptr->v_samp_factor, (long) cinfo->max_v_samp_factor);
         /* Mark component needed, until color conversion says otherwise */
         compptr->component_needed = true;
         /* Mark no quantization table yet saved for component */
         compptr->quant_table = NULL;
   }

   /* Compute number of fully interleaved MCU rows. */
   cinfo->total_iMCU_rows = (JDIMENSION) jdiv_round_up((long) cinfo->image_height, (long) (cinfo->max_v_samp_factor*DCTSIZE));

   /* Decide whether file contains multiple scans */
   cinfo->inputctl.has_multiple_scans = (cinfo->comps_in_scan < cinfo->num_components || cinfo->progressive_mode);
   return true;
}

//----------------------------

static bool per_scan_setup (jpeg_decompress_struct* cinfo){
/* Do computations that are needed before processing a JPEG scan */
/* cinfo->comps_in_scan and cinfo->cur_comp_info[] were set from SOS marker */
   int ci, mcublks, tmp;
   jpeg_component_info *compptr;

   if (cinfo->comps_in_scan == 1) {

      /* Noninterleaved (single-component) scan */
      compptr = cinfo->cur_comp_info[0];

      /* Overall image size in MCUs */
      cinfo->MCUs_per_row = compptr->width_in_blocks;
      //cinfo->MCU_rows_in_scan = compptr->height_in_blocks;

      /* For noninterleaved scan, always one block per MCU */
      compptr->MCU_width = 1;
      compptr->MCU_height = 1;
      compptr->MCU_blocks = 1;
      compptr->MCU_sample_width = compptr->DCT_scaled_size;
      compptr->last_col_width = 1;
      /* For noninterleaved scans, it is convenient to define last_row_height
      * as the number of block rows present in the last iMCU row.
      */
      tmp = (int) (compptr->height_in_blocks % compptr->v_samp_factor);
      if (tmp == 0) tmp = compptr->v_samp_factor;
      compptr->last_row_height = tmp;

      /* Prepare array describing MCU composition */
      cinfo->blocks_in_MCU = 1;
      cinfo->MCU_membership[0] = 0;

   } else {

      /* Interleaved (multi-component) scan */
      if (cinfo->comps_in_scan <= 0 || cinfo->comps_in_scan > MAX_COMPS_IN_SCAN)
         return false;//E RREXIT2(cinfo, JERR_COMPONENT_COUNT, cinfo->comps_in_scan, MAX_COMPS_IN_SCAN);

      /* Overall image size in MCUs */
      cinfo->MCUs_per_row = (JDIMENSION) jdiv_round_up((long) cinfo->image_width, (long) (cinfo->max_h_samp_factor*DCTSIZE));
      //cinfo->MCU_rows_in_scan = (JDIMENSION)jdiv_round_up((long) cinfo->image_height, (long) (cinfo->max_v_samp_factor*DCTSIZE));

      cinfo->blocks_in_MCU = 0;

      for (ci = 0; ci < cinfo->comps_in_scan; ci++) {
         compptr = cinfo->cur_comp_info[ci];
         /* Sampling factors give # of blocks of component in each MCU */
         compptr->MCU_width = compptr->h_samp_factor;
         compptr->MCU_height = compptr->v_samp_factor;
         compptr->MCU_blocks = compptr->MCU_width * compptr->MCU_height;
         compptr->MCU_sample_width = compptr->MCU_width * compptr->DCT_scaled_size;
         /* Figure number of non-dummy blocks in last MCU column & row */
         tmp = (int) (compptr->width_in_blocks % compptr->MCU_width);
         if (tmp == 0) tmp = compptr->MCU_width;
         compptr->last_col_width = tmp;
         tmp = (int) (compptr->height_in_blocks % compptr->MCU_height);
         if (tmp == 0) tmp = compptr->MCU_height;
         compptr->last_row_height = tmp;
         /* Prepare array describing MCU composition */
         mcublks = compptr->MCU_blocks;
         if (cinfo->blocks_in_MCU + mcublks > D_MAX_BLOCKS_IN_MCU)
            return false;//E RREXIT(cinfo, JERR_BAD_MCU_SIZE);
         while (mcublks-- > 0) {
            cinfo->MCU_membership[cinfo->blocks_in_MCU++] = ci;
         }
      }

   }
   return true;
}


/*
 * Save away a copy of the Q-table referenced by each component present
 * in the current scan, unless already saved during a prior scan.
 *
 * In a multiple-scan JPEG file, the encoder could assign different components
 * the same Q-table slot number, but change table definitions between scans
 * so that each component uses a different Q-table.  (The IJG encoder is not
 * currently capable of doing this, but other encoders might.)  Since we want
 * to be able to dequantize all the components at the end of the file, this
 * means that we have to save away the table actually used for each component.
 * We do this by copying the table at the start of the first scan containing
 * the component.
 * The JPEG spec prohibits the encoder from changing the contents of a Q-table
 * slot between scans of a component using that slot.  If the encoder does so
 * anyway, this decoder will simply use the Q-table values that were current
 * at the start of the first scan for the component.
 *
 * The decompressor output side looks only at the saved quant tables,
 * not at the current Q-table slots.
 */

static bool latch_quant_tables (jpeg_decompress_struct* cinfo){
   for (int ci = 0; ci < cinfo->comps_in_scan; ci++) {
      jpeg_component_info *compptr = cinfo->cur_comp_info[ci];
      /* No work if we already saved Q-table for this component */
      if (compptr->quant_table != NULL)
         continue;
      /* Make sure specified quantization table is present */
      int qtblno = compptr->quant_tbl_no;
      if (qtblno < 0 || qtblno >= NUM_QUANT_TBLS || cinfo->quant_tbl_ptrs[qtblno] == NULL)
         return false;//E RREXIT1(cinfo, JERR_NO_QUANT_TABLE, qtblno);
      /* OK, save away the quantization table */
      JQUANT_TBL *qtbl = (JQUANT_TBL *) cinfo->mem.AllocSmall(SIZEOF(JQUANT_TBL));
      if(!qtbl)
         return false;
      MemCpy(qtbl, cinfo->quant_tbl_ptrs[qtblno], SIZEOF(JQUANT_TBL));
      compptr->quant_table = qtbl;
   }
   return true;
}


/*
 * Initialize the input modules to read a scan of compressed data.
 * The first call to this is done by jdmaster.c after initializing
 * the entire decompressor (during jpeg_start_decompress).
 * Subsequent calls come from consume_markers, below.
 */

static bool start_input_pass1 (jpeg_decompress_struct* cinfo){
   if(!per_scan_setup(cinfo))
      return false;
   if(!latch_quant_tables(cinfo))
      return false;
   if(!(*cinfo->entropy->start_pass) (cinfo))
      return false;
   cinfo->coef.start_input_pass(cinfo);
   cinfo->inputctl._consume_input = cinfo->coef.consume_data;
   return true;
}


/*
 * Finish up after inputting a compressed-data scan.
 * This is called by the coefficient controller after it's read all
 * the expected data of the scan.
 */

static void finish_input_pass (jpeg_decompress_struct* cinfo){
   cinfo->inputctl._consume_input = consume_markers;
}


/*
 * Read JPEG markers before, between, or after compressed-data scans.
 * Change state as necessary when a new scan is reached.
 * Return value is JPEG_SUSPENDED, JPEG_REACHED_SOS, or JPEG_REACHED_EOI.
 *
 * The consume_input method pointer points either here or to the
 * coefficient controller's consume_data routine, depending on whether
 * we are reading a compressed data segment or inter-segment markers.
 */

static int consume_markers(jpeg_decompress_struct* cinfo){
   jpeg_decompress_struct::my_input_controller &inputctl = cinfo->inputctl;
   int val;

   if (inputctl.eoi_reached) /* After hitting EOI, read no further */
      return JPEG_REACHED_EOI;

   val = cinfo->marker.read_markers(cinfo);

   switch (val) {
  case JPEG_REACHED_SOS:   /* Found SOS */
     if (inputctl.inheaders) {   /* 1st SOS */
        if(!initial_setup(cinfo))
           return JPEG_ERROR;
        inputctl.inheaders = false;
        /* Note: start_input_pass must be called by jdmaster.c
        * before any more input can be consumed.  jdapi.c is
        * responsible for enforcing this sequencing.
        */
     } else {         /* 2nd or later SOS marker */
        if (! inputctl.has_multiple_scans)
           return JPEG_ERROR;//E RREXIT(cinfo, JERR_EOI_EXPECTED); /* Oops, I wasn't expecting this! */
        if(!start_input_pass1(cinfo))
           return JPEG_ERROR;
     }
     break;
  case JPEG_REACHED_EOI:   /* Found EOI */
     inputctl.eoi_reached = true;
     if (inputctl.inheaders) {   /* Tables-only datastream, apparently */
        if (cinfo->marker.saw_SOF)
           return JPEG_ERROR;//E RREXIT(cinfo, JERR_SOF_NO_SOS);
     } else {
        /* Prevent infinite loop in coef ctlr's decompress_data routine
        * if user set output_scan_number larger than number of scans.
        */
        if (cinfo->output_scan_number > cinfo->input_scan_number)
           cinfo->output_scan_number = cinfo->input_scan_number;
     }
     break;
  case JPEG_SUSPENDED:
     break;
   }

   return val;
}
/*
 * Initialize the input controller module.
 * This is called only once, when the decompression object is created.
 */

void jpeg_decompress_struct::jinit_input_controller(){

   /* Initialize method pointers */
   inputctl._consume_input = consume_markers;
   inputctl.start_input_pass = start_input_pass1;
   inputctl.finish_input_pass = finish_input_pass;
   inputctl.has_multiple_scans = false; /* "unknown" would be better */
   inputctl.eoi_reached = false;
   inputctl.inheaders = true;
}

//----------------------------
/*
 * This is the default resync_to_restart method for data source managers
 * to use if they don't have any better approach.  Some data source managers
 * may be able to back up, or may have additional knowledge about the data
 * which permits a more intelligent recovery strategy; such managers would
 * presumably supply their own resync method.
 *
 * read_restart_marker calls resync_to_restart if it finds a marker other than
 * the restart marker it was expecting.  (This code is *not* used unless
 * a nonzero restart interval has been declared.)  cinfo->unread_marker is
 * the marker code actually found (might be anything, except 0 or FF).
 * The desired restart marker number (0..7) is passed as a parameter.
 * This routine is supposed to apply whatever error recovery strategy seems
 * appropriate in order to position the input stream to the next data segment.
 * Note that cinfo->unread_marker is treated as a marker appearing before
 * the current data-source input point; usually it should be reset to zero
 * before returning.
 * Returns false if suspension is required.
 *
 * This implementation is substantially constrained by wanting to treat the
 * input as a data stream; this means we can't back up.  Therefore, we have
 * only the following actions to work with:
 *   1. Simply discard the marker and let the entropy decoder resume at next
 *      byte of file.
 *   2. Read forward until we find another marker, discarding intervening
 *      data.  (In theory we could look ahead within the current bufferload,
 *      without having to discard data if we don't find the desired marker.
 *      This idea is not implemented here, in part because it makes behavior
 *      dependent on buffer size and chance buffer-boundary positions.)
 *   3. Leave the marker unread (by failing to zero cinfo->unread_marker).
 *      This will cause the entropy decoder to process an empty data segment,
 *      inserting dummy zeroes, and then we will reprocess the marker.
 *
 * #2 is appropriate if we think the desired marker lies ahead, while #3 is
 * appropriate if the found marker is a future restart marker (indicating
 * that we have missed the desired restart marker, probably because it got
 * corrupted).
 * We apply #2 or #3 if the found marker is a restart marker no more than
 * two counts behind or ahead of the expected one.  We also apply #2 if the
 * found marker is not a legal JPEG marker code (it's certainly bogus data).
 * If the found marker is a restart marker more than 2 counts away, we do #1
 * (too much risk that the marker is erroneous; with luck we will be able to
 * resync at some future point).
 * For any valid non-restart JPEG marker, we apply #3.  This keeps us from
 * overrunning the end of a scan.  An implementation limited to single-scan
 * files might find it better to apply #2 for markers other than EOI, since
 * any other marker would have to be bogus data in that case.
 */

bool my_source_mgr::resync_to_restart (jpeg_decompress_struct* cinfo, int desired){
  int marker = cinfo->unread_marker;
  int action = 1;
  
  /* Outer loop handles repeated decision after scanning forward. */
  for (;;) {
    if (marker < (int) M_SOF0)
      action = 2;    /* invalid marker */
    else if (marker < (int) M_RST0 || marker > (int) M_RST7)
      action = 3;    /* valid non-restart marker */
    else {
      if (marker == ((int) M_RST0 + ((desired+1) & 7)) ||
     marker == ((int) M_RST0 + ((desired+2) & 7)))
   action = 3;    /* one of the next two expected restarts */
      else if (marker == ((int) M_RST0 + ((desired-1) & 7)) ||
          marker == ((int) M_RST0 + ((desired-2) & 7)))
   action = 2;    /* a prior restart, so advance */
      else
   action = 1;    /* desired restart or too far away */
    }
    switch (action) {
    case 1:
      /* Discard marker and let entropy decoder resume processing. */
      cinfo->unread_marker = 0;
      return true;
    case 2:
      /* Scan to the next marker, and repeat the decision loop. */
      if (! next_marker(cinfo))
   return false;
      marker = cinfo->unread_marker;
      break;
    case 3:
      /* Return without advancing past this marker. */
      /* Entropy decoder will be forced to process an empty segment. */
      return true;
    }
  } /* end loop */
}



//----------------------------
/*
 * Compute output image dimensions and related values.
 * NOTE: this is exported for possible use by application.
 * Hence it mustn't do anything that can't be done twice.
 * Also note that it may be called before the master module is initialized!
 */
/* Do computations that are needed before master selection phase */
static void jpeg_calc_output_dimensions (jpeg_decompress_struct* cinfo){
   /* Prevent application from calling me at wrong times */
   assert(cinfo->global_state==DSTATE_READY);

#ifdef IDCT_SCALING_SUPPORTED
   int ci;
   jpeg_component_info *compptr;

   /* Compute actual output image dimensions and DCT scaling choices. */
   if (cinfo->scale_num * 8 <= cinfo->scale_denom) {
      /* Provide 1/8 scaling */
      cinfo->output_width = (JDIMENSION) jdiv_round_up((long) cinfo->image_width, 8L);
      cinfo->output_height = (JDIMENSION) jdiv_round_up((long) cinfo->image_height, 8L);
      cinfo->min_DCT_scaled_size = 1;
   } else if (cinfo->scale_num * 4 <= cinfo->scale_denom) {
      /* Provide 1/4 scaling */
      cinfo->output_width = (JDIMENSION) jdiv_round_up((long) cinfo->image_width, 4L);
      cinfo->output_height = (JDIMENSION) jdiv_round_up((long) cinfo->image_height, 4L);
      cinfo->min_DCT_scaled_size = 2;
   } else if (cinfo->scale_num * 2 <= cinfo->scale_denom) {
      /* Provide 1/2 scaling */
      cinfo->output_width = (JDIMENSION) jdiv_round_up((long) cinfo->image_width, 2L);
      cinfo->output_height = (JDIMENSION) jdiv_round_up((long) cinfo->image_height, 2L);
      cinfo->min_DCT_scaled_size = 4;
   } else {
      /* Provide 1/1 scaling */
      cinfo->output_width = cinfo->image_width;
      cinfo->output_height = cinfo->image_height;
      cinfo->min_DCT_scaled_size = DCTSIZE;
   }
   /* In selecting the actual DCT scaling for each component, we try to
   * scale up the chroma components via IDCT scaling rather than upsampling.
   * This saves time if the upsampler gets to use 1:1 scaling.
   * Note this code assumes that the supported DCT scalings are powers of 2.
   */
   for (ci = 0, compptr = cinfo->comp_info; ci < cinfo->num_components; ci++, compptr++){
      int ssize = cinfo->min_DCT_scaled_size;
      while (ssize < DCTSIZE && (compptr->h_samp_factor * ssize * 2 <= cinfo->max_h_samp_factor * cinfo->min_DCT_scaled_size) &&
         (compptr->v_samp_factor * ssize * 2 <= cinfo->max_v_samp_factor * cinfo->min_DCT_scaled_size)) {
            ssize = ssize * 2;
      }
      compptr->DCT_scaled_size = ssize;
   }

   /* Recompute downsampled dimensions of components;
   * application needs to know these if using raw downsampled data.
   */
   for (ci = 0, compptr = cinfo->comp_info; ci < cinfo->num_components; ci++, compptr++) {
      /* Size in samples, after IDCT scaling */
      compptr->downsampled_width = (JDIMENSION) jdiv_round_up((long) cinfo->image_width * (long) (compptr->h_samp_factor * compptr->DCT_scaled_size), (long) (cinfo->max_h_samp_factor * DCTSIZE));
      compptr->downsampled_height = (JDIMENSION) jdiv_round_up((long) cinfo->image_height * (long) (compptr->v_samp_factor * compptr->DCT_scaled_size), (long) (cinfo->max_v_samp_factor * DCTSIZE));
   }

#else /* !IDCT_SCALING_SUPPORTED */

   /* Hardwire it to "no scaling" */
   cinfo->output_width = cinfo->image_width;
   cinfo->output_height = cinfo->image_height;
   /* jdinput.c has already initialized DCT_scaled_size to DCTSIZE,
   * and has computed unscaled downsampled_width and downsampled_height.
   */

#endif /* IDCT_SCALING_SUPPORTED */

   /* Report number of components in selected colorspace. */
   /* Probably this should be in the color conversion module... */
   switch (cinfo->out_color_space) {
   case JCS_GRAYSCALE:
      cinfo->out_color_components = 1;
      break;
   case JCS_RGB:
#if RGB_PIXELSIZE != 3
      cinfo->out_color_components = RGB_PIXELSIZE;
      break;
#endif /* else share code with YCbCr */
   case JCS_YCbCr:
      cinfo->out_color_components = 3;
      break;
   case JCS_CMYK:
   case JCS_YCCK:
      cinfo->out_color_components = 4;
      break;
   default:        /* else must be same colorspace as in file */
      cinfo->out_color_components = cinfo->num_components;
      break;
   }
}

//----------------------------

bool jpeg_decompress_struct::master_selection(){

   bool use_c_buffer;
   long samplesperrow;
   JDIMENSION jd_samplesperrow;

   /* Width of an output scanline must be representable as JDIMENSION. */
   samplesperrow = (long)output_width * (long)out_color_components;
   jd_samplesperrow = (JDIMENSION) samplesperrow;
   if ((long) jd_samplesperrow != samplesperrow)
      return false;//E RREXIT(cinfo, JERR_WIDTH_OVERFLOW);

   if(!jinit_color_deconverter(this))
      return false;
   if(!jinit_upsampler(this))
      return false;
   /* Inverse DCT */
   idct.init(this);
   /* Entropy decoding: either Huffman or arithmetic coding. */
   if (arith_code)
      return false;//E RREXIT(this, JERR_ARITH_NOTIMPL);
   if (progressive_mode) {
#ifdef D_PROGRESSIVE_SUPPORTED
      if(!jinit_phuff_decoder(this))
         return false;
#else
      return false;//E RREXIT(this, JERR_NOT_COMPILED);
#endif
   } else{
      if(!jinit_huff_decoder(this))
         return false;
   }

   /* Initialize principal buffer controllers. */
   use_c_buffer = inputctl.has_multiple_scans || buffered_image;
   if(!jinit_d_coef_controller(this, use_c_buffer))
      return false;

   if(!main.init(this))
      return false;

   /* We can now tell the memory manager to allocate virtual arrays. */
   if(!(*mem.realize_virt_arrays) (this))
      return false;

   /* Initialize input side of decompressor to consume first scan. */
   if(!(*inputctl.start_input_pass) (this))
      return false;
   return true;
}

//----------------------------

#ifdef D_PROGRESSIVE_SUPPORTED

/*
 * Expanded entropy decoder object for progressive Huffman decoding.
 *
 * The savable_state subrecord contains fields that change within an MCU,
 * but must not be updated permanently until we complete the MCU.
 */

typedef struct {
  dword EOBRUN;        /* remaining EOBs in EOBRUN */
  int last_dc_val[MAX_COMPS_IN_SCAN];  /* last DC coef for each component */
} savable_state1;

/* This macro is to work around compilers with missing or broken
 * structure assignment.  You'll need to fix this code if you have
 * such a compiler and you change MAX_COMPS_IN_SCAN.
 */

#ifndef NO_STRUCT_ASSIGN
#define ASSIGN_STATE(dest,src)  ((dest) = (src))
#else
#if MAX_COMPS_IN_SCAN == 4
#define ASSIGN_STATE(dest,src)  \
   ((dest).EOBRUN = (src).EOBRUN, \
    (dest).last_dc_val[0] = (src).last_dc_val[0], \
    (dest).last_dc_val[1] = (src).last_dc_val[1], \
    (dest).last_dc_val[2] = (src).last_dc_val[2], \
    (dest).last_dc_val[3] = (src).last_dc_val[3])
#endif
#endif


struct phuff_entropy_decoder: public huff_entropy_decoder{

  /* These fields are loaded into local variables at start of each MCU.
   * In case of suspension, we exit WITHOUT updating them.
   */
   bitread_perm_state bitstate;   /* Bit buffer at start of MCU */
   savable_state1 saved;     /* Other state at start of MCU */

  /* These fields are NOT loaded into local working state. */
   dword restarts_to_go;   /* MCUs left in this restart interval */

  /* Pointers to derived tables (these workspaces have image lifespan) */
   //d_derived_tbl derived_tbls[NUM_HUFF_TBLS];
   //d_derived_tbl ac_derived_tbl; /* active table during an AC scan */
};

/* Forward declarations */
static bool decode_mcu_DC_first (jpeg_decompress_struct* cinfo, JBLOCKROW *MCU_data);
static bool decode_mcu_AC_first (jpeg_decompress_struct* cinfo, JBLOCKROW *MCU_data);
static bool decode_mcu_DC_refine (jpeg_decompress_struct* cinfo, JBLOCKROW *MCU_data);
static bool decode_mcu_AC_refine (jpeg_decompress_struct* cinfo, JBLOCKROW *MCU_data);


/*
 * Initialize for a Huffman-compressed scan.
 */

static bool start_pass_phuff_decoder (jpeg_decompress_struct* cinfo){
   phuff_entropy_decoder* entropy = (phuff_entropy_decoder*) cinfo->entropy;
   bool is_DC_band, bad;
   int ci, coefi, tbl;
   int *coef_bit_ptr;
   jpeg_component_info * compptr;

   is_DC_band = (cinfo->Ss == 0);

   /* Validate scan parameters */
   bad = false;
   if (is_DC_band) {
      if (cinfo->Se != 0)
         bad = true;
   } else {
      /* need not check Ss/Se < 0 since they came from unsigned bytes */
      if (cinfo->Ss > cinfo->Se || cinfo->Se >= DCTSIZE2)
         bad = true;
      /* AC scans may have only one component */
      if (cinfo->comps_in_scan != 1)
         bad = true;
   }
   if (cinfo->Ah != 0) {
      /* Successive approximation refinement scan: must have Al = Ah-1. */
      if (cinfo->Al != cinfo->Ah-1)
         bad = true;
   }
   if (cinfo->Al > 13)      /* need not check for < 0 */
      bad = true;
   if (bad)
      return false;//E RREXIT4(cinfo, JERR_BAD_PROGRESSION, cinfo->Ss, cinfo->Se, cinfo->Ah, cinfo->Al);
   /* Update progression status, and verify that scan order is legal.
   * Note that inter-scan inconsistencies are treated as warnings
   * not fatal errors ... not clear if this is right way to behave.
   */
   for (ci = 0; ci < cinfo->comps_in_scan; ci++) {
      int cindex = cinfo->cur_comp_info[ci]->component_index;
      coef_bit_ptr = & cinfo->coef_bits[cindex][0];
      if (!is_DC_band && coef_bit_ptr[0] < 0){ /* AC without prior DC scan */
      }
      for (coefi = cinfo->Ss; coefi <= cinfo->Se; coefi++) {
         int expected = (coef_bit_ptr[coefi] < 0) ? 0 : coef_bit_ptr[coefi];
         if (cinfo->Ah != expected){
         }
         coef_bit_ptr[coefi] = cinfo->Al;
      }
   }

   /* Select MCU decoding routine */
   if (cinfo->Ah == 0) {
      if (is_DC_band)
         entropy->decode_mcu = decode_mcu_DC_first;
      else
         entropy->decode_mcu = decode_mcu_AC_first;
   } else {
      if (is_DC_band)
         entropy->decode_mcu = decode_mcu_DC_refine;
      else
         entropy->decode_mcu = decode_mcu_AC_refine;
   }

   for (ci = 0; ci < cinfo->comps_in_scan; ci++) {
      compptr = cinfo->cur_comp_info[ci];
      /* Make sure requested tables are present, and compute derived tables.
      * We may build same derived table more than once, but it's not expensive.
      */
      if (is_DC_band) {
         if (cinfo->Ah == 0) {   /* DC refinement needs no table */
            tbl = compptr->dc_tbl_no;
            if (tbl < 0 || tbl >= NUM_HUFF_TBLS || cinfo->dc_huff_tbl_ptrs[tbl] == NULL)
               return false;//E RREXIT1(cinfo, JERR_NO_HUFF_TABLE, tbl);
            if(!jpeg_make_d_derived_tbl(cinfo, cinfo->dc_huff_tbl_ptrs[tbl], entropy->dc_derived_tbls[tbl]))
               return false;
         }
      } else {
         tbl = compptr->ac_tbl_no;
         if (tbl < 0 || tbl >= NUM_HUFF_TBLS || cinfo->ac_huff_tbl_ptrs[tbl] == NULL)
            return false;//E RREXIT1(cinfo, JERR_NO_HUFF_TABLE, tbl);
         if(!jpeg_make_d_derived_tbl(cinfo, cinfo->ac_huff_tbl_ptrs[tbl], entropy->dc_derived_tbls[tbl]))
            return false;
         /* remember the single active table */
         entropy->ac_derived_tbls[0] = entropy->dc_derived_tbls[tbl];
      }
      /* Initialize DC predictions to 0 */
      entropy->saved.last_dc_val[ci] = 0;
   }

   /* Initialize bitread state variables */
   entropy->bitstate.bits_left = 0;
   entropy->bitstate.get_buffer = 0; /* unnecessary, but keeps Purify quiet */
   entropy->bitstate.printed_eod = false;

   /* Initialize private state variables */
   entropy->saved.EOBRUN = 0;

   /* Initialize restart counter */
   entropy->restarts_to_go = cinfo->restart_interval;
   return true;
}


/*
 * Figure F.12: extend sign bit.
 * On some machines, a shift and add will be faster than a table lookup.
 */

#ifdef AVOID_TABLES

#define HUFF_EXTEND(x,s)  ((x) < (1<<((s)-1)) ? (x) + (((-1)<<(s)) + 1) : (x))

#else

#define HUFF_EXTEND(x,s)  ((x) < extend_test[s] ? (x) + extend_offset[s] : (x))

/*
static const int extend_test[16] =
  { 0, 0x0001, 0x0002, 0x0004, 0x0008, 0x0010, 0x0020, 0x0040, 0x0080,
    0x0100, 0x0200, 0x0400, 0x0800, 0x1000, 0x2000, 0x4000 };

static const int extend_offset[16] =
  { 0, ((-1)<<1) + 1, ((-1)<<2) + 1, ((-1)<<3) + 1, ((-1)<<4) + 1,
    ((-1)<<5) + 1, ((-1)<<6) + 1, ((-1)<<7) + 1, ((-1)<<8) + 1,
    ((-1)<<9) + 1, ((-1)<<10) + 1, ((-1)<<11) + 1, ((-1)<<12) + 1,
    ((-1)<<13) + 1, ((-1)<<14) + 1, ((-1)<<15) + 1 };
    */

#endif /* AVOID_TABLES */


/*
 * Check for a restart marker & resynchronize decoder.
 * Returns false if must suspend.
 */

static bool process_restart1 (jpeg_decompress_struct* cinfo){
  phuff_entropy_decoder* entropy = (phuff_entropy_decoder*) cinfo->entropy;
  int ci;

  /* Throw away any unused bits remaining in bit buffer; */
  /* include any full bytes in next_marker's count of discarded bytes */
  cinfo->marker.discarded_bytes += entropy->bitstate.bits_left / 8;
  entropy->bitstate.bits_left = 0;

  /* Advance past the RSTn marker */
  if (!cinfo->marker.read_restart_marker(cinfo))
    return false;

  /* Re-initialize DC predictions to 0 */
  for (ci = 0; ci < cinfo->comps_in_scan; ci++)
    entropy->saved.last_dc_val[ci] = 0;
  /* Re-init EOB run count, too */
  entropy->saved.EOBRUN = 0;

  /* Reset restart counter */
  entropy->restarts_to_go = cinfo->restart_interval;

  /* Next segment can get another out-of-data warning */
  entropy->bitstate.printed_eod = false;

  return true;
}


/*
 * Huffman MCU decoding.
 * Each of these routines decodes and returns one MCU's worth of
 * Huffman-compressed coefficients. 
 * The coefficients are reordered from zigzag order into natural array order,
 * but are not dequantized.
 *
 * The i'th block of the MCU is stored into the block pointed to by
 * MCU_data[i].  WE ASSUME THIS AREA IS INITIALLY ZEROED BY THE CALLER.
 *
 * We return false if data source requested suspension.  In that case no
 * changes have been made to permanent state.  (Exception: some output
 * coefficients may already have been assigned.  This is harmless for
 * spectral selection, since we'll just re-assign them on the next call.
 * Successive approximation AC refinement has to be more careful, however.)
 */

/*
 * MCU decoding for DC initial scan (either spectral selection,
 * or first pass of successive approximation).
 */

static bool decode_mcu_DC_first (jpeg_decompress_struct* cinfo, JBLOCKROW *MCU_data){   
  phuff_entropy_decoder* entropy = (phuff_entropy_decoder*) cinfo->entropy;
  int Al = cinfo->Al;
  int s, r;
  int blkn, ci;
  JBLOCKROW block;
  BITREAD_STATE_VARS;
  savable_state1 state;
  d_derived_tbl * tbl;
  jpeg_component_info * compptr;

  /* Process restart marker if needed; may have to suspend */
  if (cinfo->restart_interval) {
    if (entropy->restarts_to_go == 0)
      if (! process_restart1(cinfo))
   return false;
  }

  /* Load up working state */
  BITREAD_LOAD_STATE(cinfo,entropy->bitstate);
  ASSIGN_STATE(state, entropy->saved);

  /* Outer loop handles each block in the MCU */

  for (blkn = 0; blkn < cinfo->blocks_in_MCU; blkn++) {
    block = MCU_data[blkn];
    ci = cinfo->MCU_membership[blkn];
    compptr = cinfo->cur_comp_info[ci];
    tbl = &entropy->dc_derived_tbls[compptr->dc_tbl_no];

    /* Decode a single block's worth of coefficients */

    /* Section F.2.2.1: decode the DC coefficient difference */
    HUFF_DECODE(s, br_state, tbl, return false, label1);
    if (s) {
      CHECK_BIT_BUFFER(br_state, s, return false);
      r = GET_BITS(s);
      s = HUFF_EXTEND(r, s);
    }

    /* Convert DC difference to actual value, update last_dc_val */
    s += state.last_dc_val[ci];
    state.last_dc_val[ci] = s;
    /* Scale and output the DC coefficient (assumes jpeg_natural_order[0]=0) */
    (*block)[0] = (JCOEF) (s << Al);
  }

  /* Completed MCU, so update state */
  BITREAD_SAVE_STATE(cinfo,entropy->bitstate);
  ASSIGN_STATE(entropy->saved, state);

  /* Account for restart interval (no-op if not using restarts) */
  entropy->restarts_to_go--;

  return true;
}


/*
 * MCU decoding for AC initial scan (either spectral selection,
 * or first pass of successive approximation).
 */

static bool decode_mcu_AC_first (jpeg_decompress_struct* cinfo, JBLOCKROW *MCU_data){   
  phuff_entropy_decoder* entropy = (phuff_entropy_decoder*) cinfo->entropy;
  int Se = cinfo->Se;
  int Al = cinfo->Al;
  int s, k, r;
  dword EOBRUN;
  JBLOCKROW block;
  BITREAD_STATE_VARS;
  d_derived_tbl * tbl;

  /* Process restart marker if needed; may have to suspend */
  if (cinfo->restart_interval) {
    if (entropy->restarts_to_go == 0)
      if (! process_restart1(cinfo))
   return false;
  }

  /* Load up working state.
   * We can avoid loading/saving bitread state if in an EOB run.
   */
  EOBRUN = entropy->saved.EOBRUN; /* only part of saved state we care about */

  /* There is always only one block per MCU */

  if (EOBRUN > 0)    /* if it's a band of zeroes... */
    EOBRUN--;        /* ...process it now (we do nothing) */
  else {
    BITREAD_LOAD_STATE(cinfo,entropy->bitstate);
    block = MCU_data[0];
    tbl = &entropy->ac_derived_tbls[0];

    for (k = cinfo->Ss; k <= Se; k++) {
      HUFF_DECODE(s, br_state, tbl, return false, label2);
      r = s >> 4;
      s &= 15;
      if (s) {
        k += r;
        CHECK_BIT_BUFFER(br_state, s, return false);
        r = GET_BITS(s);
        s = HUFF_EXTEND(r, s);
   /* Scale and output coefficient in natural (dezigzagged) order */
        (*block)[jpeg_natural_order[k]] = (JCOEF) (s << Al);
      } else {
        if (r == 15) {     /* ZRL */
          k += 15;      /* skip 15 zeroes in band */
        } else {     /* EOBr, run length is 2^r + appended bits */
          EOBRUN = 1 << r;
          if (r) {      /* EOBr, r > 0 */
       CHECK_BIT_BUFFER(br_state, r, return false);
            r = GET_BITS(r);
            EOBRUN += r;
          }
     EOBRUN--;    /* this band is processed at this moment */
     break;    /* force end-of-band */
   }
      }
    }

    BITREAD_SAVE_STATE(cinfo,entropy->bitstate);
  }

  /* Completed MCU, so update state */
  entropy->saved.EOBRUN = EOBRUN; /* only part of saved state we care about */

  /* Account for restart interval (no-op if not using restarts) */
  entropy->restarts_to_go--;

  return true;
}


/*
 * MCU decoding for DC successive approximation refinement scan.
 * Note: we assume such scans can be multi-component, although the spec
 * is not very clear on the point.
 */

static bool decode_mcu_DC_refine (jpeg_decompress_struct* cinfo, JBLOCKROW *MCU_data){   
  phuff_entropy_decoder* entropy = (phuff_entropy_decoder*) cinfo->entropy;
  int p1 = 1 << cinfo->Al; /* 1 in the bit position being coded */
  int blkn;
  JBLOCKROW block;
  BITREAD_STATE_VARS;

  /* Process restart marker if needed; may have to suspend */
  if (cinfo->restart_interval) {
    if (entropy->restarts_to_go == 0)
      if (! process_restart1(cinfo))
   return false;
  }

  /* Load up working state */
  BITREAD_LOAD_STATE(cinfo,entropy->bitstate);

  /* Outer loop handles each block in the MCU */

  for (blkn = 0; blkn < cinfo->blocks_in_MCU; blkn++) {
    block = MCU_data[blkn];

    /* Encoded data is simply the next bit of the two's-complement DC value */
    CHECK_BIT_BUFFER(br_state, 1, return false);
    if (GET_BITS(1))
      (*block)[0] |= p1;
    /* Note: since we use |=, repeating the assignment later is safe */
  }

  /* Completed MCU, so update state */
  BITREAD_SAVE_STATE(cinfo,entropy->bitstate);

  /* Account for restart interval (no-op if not using restarts) */
  entropy->restarts_to_go--;

  return true;
}


/*
 * MCU decoding for AC successive approximation refinement scan.
 */

static bool decode_mcu_AC_refine (jpeg_decompress_struct* cinfo, JBLOCKROW *MCU_data){   
  phuff_entropy_decoder* entropy = (phuff_entropy_decoder*) cinfo->entropy;
  int Se = cinfo->Se;
  int p1 = 1 << cinfo->Al; /* 1 in the bit position being coded */
  int m1 = (-1) << cinfo->Al; /* -1 in the bit position being coded */
  int s, k, r;
  dword EOBRUN;
  JBLOCKROW block;
  JCOEFPTR thiscoef;
  BITREAD_STATE_VARS;
  d_derived_tbl * tbl;
  int num_newnz;
  int newnz_pos[DCTSIZE2];

  /* Process restart marker if needed; may have to suspend */
  if (cinfo->restart_interval) {
    if (entropy->restarts_to_go == 0)
      if (! process_restart1(cinfo))
   return false;
  }

  /* Load up working state */
  BITREAD_LOAD_STATE(cinfo,entropy->bitstate);
  EOBRUN = entropy->saved.EOBRUN; /* only part of saved state we care about */

  /* There is always only one block per MCU */
  block = MCU_data[0];
  tbl = &entropy->ac_derived_tbls[0];

  /* If we are forced to suspend, we must undo the assignments to any newly
   * nonzero coefficients in the block, because otherwise we'd get confused
   * next time about which coefficients were already nonzero.
   * But we need not undo addition of bits to already-nonzero coefficients;
   * instead, we can test the current bit position to see if we already did it.
   */
  num_newnz = 0;

  /* initialize coefficient loop counter to start of band */
  k = cinfo->Ss;

  if (EOBRUN == 0) {
    for (; k <= Se; k++) {
      HUFF_DECODE(s, br_state, tbl, goto undoit, label3);
      r = s >> 4;
      s &= 15;
      if (s) {
   if (s != 1)    /* size of new coef should always be 1 */
        CHECK_BIT_BUFFER(br_state, 1, goto undoit);
        if (GET_BITS(1))
     s = p1;      /* newly nonzero coef is positive */
   else
     s = m1;      /* newly nonzero coef is negative */
      } else {
   if (r != 15) {
     EOBRUN = 1 << r;   /* EOBr, run length is 2^r + appended bits */
     if (r) {
       CHECK_BIT_BUFFER(br_state, r, goto undoit);
       r = GET_BITS(r);
       EOBRUN += r;
     }
     break;    /* rest of block is handled by EOB logic */
   }
   /* note s = 0 for processing ZRL */
      }
      /* Advance over already-nonzero coefs and r still-zero coefs,
       * appending correction bits to the nonzeroes.  A correction bit is 1
       * if the absolute value of the coefficient must be increased.
       */
      do {
   thiscoef = *block + jpeg_natural_order[k];
   if (*thiscoef != 0) {
     CHECK_BIT_BUFFER(br_state, 1, goto undoit);
     if (GET_BITS(1)) {
       if ((*thiscoef & p1) == 0) { /* do nothing if already changed it */
         if (*thiscoef >= 0)
      *thiscoef += p1;
         else
      *thiscoef += m1;
       }
     }
   } else {
     if (--r < 0)
       break;     /* reached target zero coefficient */
   }
   k++;
      } while (k <= Se);
      if (s) {
   int pos = jpeg_natural_order[k];
   /* Output newly nonzero coefficient */
   (*block)[pos] = (JCOEF) s;
   /* Remember its position in case we have to suspend */
   newnz_pos[num_newnz++] = pos;
      }
    }
  }

  if (EOBRUN > 0) {
    /* Scan any remaining coefficient positions after the end-of-band
     * (the last newly nonzero coefficient, if any).  Append a correction
     * bit to each already-nonzero coefficient.  A correction bit is 1
     * if the absolute value of the coefficient must be increased.
     */
    for (; k <= Se; k++) {
      thiscoef = *block + jpeg_natural_order[k];
      if (*thiscoef != 0) {
   CHECK_BIT_BUFFER(br_state, 1, goto undoit);
   if (GET_BITS(1)) {
     if ((*thiscoef & p1) == 0) { /* do nothing if already changed it */
       if (*thiscoef >= 0)
         *thiscoef += p1;
       else
         *thiscoef += m1;
     }
   }
      }
    }
    /* Count one block completed in EOB run */
    EOBRUN--;
  }

  /* Completed MCU, so update state */
  BITREAD_SAVE_STATE(cinfo,entropy->bitstate);
  entropy->saved.EOBRUN = EOBRUN; /* only part of saved state we care about */

  /* Account for restart interval (no-op if not using restarts) */
  entropy->restarts_to_go--;

  return true;

undoit:
  /* Re-zero any output coefficients that we made newly nonzero */
  while (num_newnz > 0)
    (*block)[newnz_pos[--num_newnz]] = 0;

  return false;
}


/*
 * Module initialization routine for progressive Huffman entropy decoding.
 */
static bool jinit_phuff_decoder (jpeg_decompress_struct* cinfo){
   int *coef_bit_ptr;
   int ci, i;

   phuff_entropy_decoder* entropy = new phuff_entropy_decoder;
   if(!entropy)
      return false;
   cinfo->entropy = entropy;
   entropy->start_pass = start_pass_phuff_decoder;

   /* Create progression status table */
   cinfo->coef_bits = (int (*)[DCTSIZE2]) cinfo->mem.AllocSmall(cinfo->num_components*DCTSIZE2*SIZEOF(int));
   if(!cinfo->coef_bits)
      return false;
   coef_bit_ptr = & cinfo->coef_bits[0][0];
   for (ci = 0; ci < cinfo->num_components; ci++) 
      for (i = 0; i < DCTSIZE2; i++)
         *coef_bit_ptr++ = -1;
   return true;
}

#endif /* D_PROGRESSIVE_SUPPORTED */

//----------------------------
/*
 * Control routine to do upsampling (and color conversion).
 *
 * In this version we upsample each component independently.
 * We upsample one row group into the conversion buffer, then apply
 * color conversion a row at a time.
 */
void jpeg_decompress_struct::my_upsampler::sep_upsample (jpeg_decompress_struct* cinfo, JSAMPIMAGE input_buf, JDIMENSION *in_row_group_ctr, JDIMENSION in_row_groups_avail, JSAMPARRAY output_buf, JDIMENSION *out_row_ctr, JDIMENSION out_rows_avail){

   jpeg_decompress_struct::my_upsampler * upsample = &cinfo->upsample;
   int ci;
   jpeg_component_info * compptr;
   JDIMENSION num_rows;

  /* Fill the conversion buffer, if it's empty */
   if(upsample->next_row_out >= cinfo->max_v_samp_factor) {
      for(ci = 0, compptr = cinfo->comp_info; ci < cinfo->num_components;
         ci++, compptr++){
      /* Invoke per-component upsample method.  Notice we pass a POINTER
       * to color_buf[ci], so that fullsize_upsample can change it.
       */
         (*upsample->methods[ci]) (cinfo, compptr, input_buf[ci] + (*in_row_group_ctr * upsample->rowgroup_height[ci]), upsample->color_buf + ci);
      }
      upsample->next_row_out = 0;
   }

  /* Color-convert and emit rows */

  /* How many we have in the buffer: */
   num_rows = (JDIMENSION) (cinfo->max_v_samp_factor - upsample->next_row_out);
  /* Not more than the distance to the end of the image.  Need this test
   * in case the image height is not a multiple of max_v_samp_factor:
   */
   if (num_rows > upsample->rows_to_go) 
      num_rows = upsample->rows_to_go;
  /* And not more than what the client can accept: */
   out_rows_avail -= *out_row_ctr;
   if (num_rows > out_rows_avail)
      num_rows = out_rows_avail;

   (*cinfo->cconvert.color_convert) (cinfo, upsample->color_buf, (JDIMENSION) upsample->next_row_out, output_buf + *out_row_ctr, (int)num_rows);

  /* Adjust counts */
   *out_row_ctr += num_rows;
   upsample->rows_to_go -= num_rows;
   upsample->next_row_out += num_rows;
  /* When the buffer is emptied, declare this input row group consumed */
   if (upsample->next_row_out >= cinfo->max_v_samp_factor)
      (*in_row_group_ctr)++;
}

//----------------------------
/*
 * For full-size components, we just make color_buf[ci] point at the
 * input buffer, and thus avoid copying any data.  Note that this is
 * safe only because sep_upsample doesn't declare the input row group
 * "consumed" until we are done color converting and emitting it.
 */
static void fullsize_upsample (jpeg_decompress_struct* cinfo, jpeg_component_info * compptr,
   JSAMPARRAY input_data, JSAMPARRAY * output_data_ptr){

   *output_data_ptr = input_data;
}

//----------------------------
/*
 * This is a no-op version used for "uninteresting" components.
 * These components will not be referenced by color conversion.
 */
static void
noop_upsample (jpeg_decompress_struct* cinfo, jpeg_component_info * compptr,
          JSAMPARRAY input_data, JSAMPARRAY * output_data_ptr)
{
  *output_data_ptr = NULL; /* safety check */
}


/*
 * This version handles any integral sampling ratios.
 * This is not used for typical JPEG files, so it need not be fast.
 * Nor, for that matter, is it particularly accurate: the algorithm is
 * simple replication of the input pixel onto the corresponding output
 * pixels.  The hi-falutin sampling literature refers to this as a
 * "box filter".  A box filter tends to introduce visible artifacts,
 * so if you are actually going to use 3:1 or 4:1 sampling ratios
 * you would be well advised to improve this code.
 */

static void int_upsample (jpeg_decompress_struct* cinfo, jpeg_component_info * compptr, JSAMPARRAY input_data, JSAMPARRAY * output_data_ptr){
   jpeg_decompress_struct::my_upsampler * upsample = &cinfo->upsample;
  JSAMPARRAY output_data = *output_data_ptr;
  JSAMPROW inptr, outptr;
  JSAMPLE invalue;
  int h;
  JSAMPROW outend;
  int h_expand, v_expand;
  int inrow, outrow;

  h_expand = upsample->h_expand[compptr->component_index];
  v_expand = upsample->v_expand[compptr->component_index];

  inrow = outrow = 0;
  while (outrow < cinfo->max_v_samp_factor) {
    /* Generate one output row with proper horizontal expansion */
    inptr = input_data[inrow];
    outptr = output_data[outrow];
    outend = outptr + cinfo->output_width;
    while (outptr < outend) {
      invalue = *inptr++;  /* don't need GETJSAMPLE() here */
      for (h = h_expand; h > 0; h--) {
   *outptr++ = invalue;
      }
    }
    /* Generate any additional output rows by duplicating the first one */
    if (v_expand > 1) {
      jcopy_sample_rows(output_data, outrow, output_data, outrow+1, v_expand-1, cinfo->output_width);
    }
    inrow++;
    outrow += v_expand;
  }
}


/*
 * Fast processing for the common case of 2:1 horizontal and 1:1 vertical.
 * It's still a box filter.
 */

static void
h2v1_upsample (jpeg_decompress_struct* cinfo, jpeg_component_info * compptr,
          JSAMPARRAY input_data, JSAMPARRAY * output_data_ptr)
{
  JSAMPARRAY output_data = *output_data_ptr;
  JSAMPROW inptr, outptr;
  JSAMPLE invalue;
  JSAMPROW outend;
  int inrow;

  for (inrow = 0; inrow < cinfo->max_v_samp_factor; inrow++) {
    inptr = input_data[inrow];
    outptr = output_data[inrow];
    outend = outptr + cinfo->output_width;
    while (outptr < outend) {
      invalue = *inptr++;  /* don't need GETJSAMPLE() here */
      *outptr++ = invalue;
      *outptr++ = invalue;
    }
  }
}


/*
 * Fast processing for the common case of 2:1 horizontal and 2:1 vertical.
 * It's still a box filter.
 */

static void
h2v2_upsample (jpeg_decompress_struct* cinfo, jpeg_component_info * compptr,
          JSAMPARRAY input_data, JSAMPARRAY * output_data_ptr)
{
  JSAMPARRAY output_data = *output_data_ptr;
  JSAMPROW inptr, outptr;
  JSAMPLE invalue;
  JSAMPROW outend;
  int inrow, outrow;

  inrow = outrow = 0;
  while (outrow < cinfo->max_v_samp_factor) {
    inptr = input_data[inrow];
    outptr = output_data[outrow];
    outend = outptr + cinfo->output_width;
    while (outptr < outend) {
      invalue = *inptr++;  /* don't need GETJSAMPLE() here */
      *outptr++ = invalue;
      *outptr++ = invalue;
    }
    jcopy_sample_rows(output_data, outrow, output_data, outrow+1, 1, cinfo->output_width);
    inrow++;
    outrow += 2;
  }
}


/*
 * Fancy processing for the common case of 2:1 horizontal and 1:1 vertical.
 *
 * The upsampling algorithm is linear interpolation between pixel centers,
 * also known as a "triangle filter".  This is a good compromise between
 * speed and visual quality.  The centers of the output pixels are 1/4 and 3/4
 * of the way between input pixel centers.
 *
 * A note about the "bias" calculations: when rounding fractional values to
 * integer, we do not want to always round 0.5 up to the next integer.
 * If we did that, we'd introduce a noticeable bias towards larger values.
 * Instead, this code is arranged so that 0.5 will be rounded up or down at
 * alternate pixel locations (a simple ordered dither pattern).
 */

static void
h2v1_fancy_upsample (jpeg_decompress_struct* cinfo, jpeg_component_info * compptr,
           JSAMPARRAY input_data, JSAMPARRAY * output_data_ptr)
{
  JSAMPARRAY output_data = *output_data_ptr;
  JSAMPROW inptr, outptr;
  int invalue;
  JDIMENSION colctr;
  int inrow;

  for (inrow = 0; inrow < cinfo->max_v_samp_factor; inrow++) {
    inptr = input_data[inrow];
    outptr = output_data[inrow];
    /* Special case for first column */
    invalue = GETJSAMPLE(*inptr++);
    *outptr++ = (JSAMPLE) invalue;
    *outptr++ = (JSAMPLE) ((invalue * 3 + GETJSAMPLE(*inptr) + 2) >> 2);

    for (colctr = compptr->downsampled_width - 2; colctr > 0; colctr--) {
      /* General case: 3/4 * nearer pixel + 1/4 * further pixel */
      invalue = GETJSAMPLE(*inptr++) * 3;
      *outptr++ = (JSAMPLE) ((invalue + GETJSAMPLE(inptr[-2]) + 1) >> 2);
      *outptr++ = (JSAMPLE) ((invalue + GETJSAMPLE(*inptr) + 2) >> 2);
    }

    /* Special case for last column */
    invalue = GETJSAMPLE(*inptr);
    *outptr++ = (JSAMPLE) ((invalue * 3 + GETJSAMPLE(inptr[-1]) + 1) >> 2);
    *outptr++ = (JSAMPLE) invalue;
  }
}

//----------------------------
/*
static void h2v2_fancy_upsample (jpeg_decompress_struct* cinfo, jpeg_component_info * compptr,
   JSAMPARRAY input_data, JSAMPARRAY * output_data_ptr){

   JSAMPARRAY output_data = *output_data_ptr;
   JSAMPROW inptr0, inptr1, outptr;
#if BITS_IN_JSAMPLE == 8
   int thiscolsum, lastcolsum, nextcolsum;
#else
   int thiscolsum, lastcolsum, nextcolsum;
#endif
   JDIMENSION colctr;
   int inrow, outrow, v;

   inrow = outrow = 0;
   while(outrow < cinfo->max_v_samp_factor){
      for(v = 0; v < 2; v++){
         inptr0 = input_data[inrow];
         if (v == 0) 
            inptr1 = input_data[inrow-1];
         else        
            inptr1 = input_data[inrow+1];
         outptr = output_data[outrow++];

         thiscolsum = GETJSAMPLE(*inptr0++) * 3 + GETJSAMPLE(*inptr1++);
         nextcolsum = GETJSAMPLE(*inptr0++) * 3 + GETJSAMPLE(*inptr1++);
         *outptr++ = (JSAMPLE) ((thiscolsum * 4 + 8) >> 4);
         *outptr++ = (JSAMPLE) ((thiscolsum * 3 + nextcolsum + 7) >> 4);
         lastcolsum = thiscolsum; thiscolsum = nextcolsum;

         for (colctr = compptr->downsampled_width - 2; colctr > 0; colctr--) {
            nextcolsum = GETJSAMPLE(*inptr0++) * 3 + GETJSAMPLE(*inptr1++);
            *outptr++ = (JSAMPLE) ((thiscolsum * 3 + lastcolsum + 8) >> 4);
            *outptr++ = (JSAMPLE) ((thiscolsum * 3 + nextcolsum + 7) >> 4);
            lastcolsum = thiscolsum; thiscolsum = nextcolsum;
         }
         *outptr++ = (JSAMPLE) ((thiscolsum * 3 + lastcolsum + 8) >> 4);
         *outptr++ = (JSAMPLE) ((thiscolsum * 4 + 7) >> 4);
      }
      inrow++;
   }
}
*/

//----------------------------
/*
 * Module initialization routine for upsampling.
 */

static bool jinit_upsampler (jpeg_decompress_struct* cinfo){
   int ci;
   jpeg_component_info * compptr;
   bool need_buffer;
   int h_in_group, v_in_group, h_out_group, v_out_group;

   jpeg_decompress_struct::my_upsampler *upsample = &cinfo->upsample;

   if (cinfo->CCIR601_sampling)   /* this isn't supported */
      return false;//E RREXIT(cinfo, JERR_CCIR601_NOTIMPL);

   /* jdmainct.c doesn't support context rows when min_DCT_scaled_size = 1,
   * so don't ask for it.
   */
   //bool do_fancy = cinfo->do_fancy_upsampling && cinfo->min_DCT_scaled_size > 1;

   /* Verify we can handle the sampling factors, select per-component methods,
   * and create storage as needed.
   */
   for (ci = 0, compptr = cinfo->comp_info; ci < cinfo->num_components; ci++, compptr++) {
      /* Compute size of an "input group" after IDCT scaling.  This many samples
      * are to be converted to max_h_samp_factor * max_v_samp_factor pixels.
      */
      h_in_group = (compptr->h_samp_factor * compptr->DCT_scaled_size) / cinfo->min_DCT_scaled_size;
      v_in_group = (compptr->v_samp_factor * compptr->DCT_scaled_size) / cinfo->min_DCT_scaled_size;
      h_out_group = cinfo->max_h_samp_factor;
      v_out_group = cinfo->max_v_samp_factor;
      upsample->rowgroup_height[ci] = v_in_group; /* save for use later */
      need_buffer = true;
      if (! compptr->component_needed) {
         /* Don't bother to upsample an uninteresting component. */
         upsample->methods[ci] = noop_upsample;
         need_buffer = false;
      } else if (h_in_group == h_out_group && v_in_group == v_out_group) {
         /* Fullsize components can be processed without any work. */
         upsample->methods[ci] = fullsize_upsample;
         need_buffer = false;
      } else
      if (h_in_group * 2 == h_out_group && v_in_group == v_out_group) {
         /* Special cases for 2h1v upsampling */
         /*
         if (do_fancy && compptr->downsampled_width > 2)
            upsample->methods[ci] = h2v1_fancy_upsample;
         else
         */
            upsample->methods[ci] = h2v1_upsample;
      } else
      if (h_in_group * 2 == h_out_group && v_in_group * 2 == v_out_group) {
            /* Special cases for 2h2v upsampling */
         /*
         if (do_fancy && compptr->downsampled_width > 2) {
            upsample->methods[ci] = h2v2_fancy_upsample;
            upsample->pub.need_context_rows = true;
         } else
         */
            upsample->methods[ci] = h2v2_upsample;
      } else
      if ((h_out_group % h_in_group) == 0 && (v_out_group % v_in_group) == 0) {
            /* Generic integral-factors upsampling method */
         upsample->methods[ci] = int_upsample;
         upsample->h_expand[ci] = (byte) (h_out_group / h_in_group);
         upsample->v_expand[ci] = (byte) (v_out_group / v_in_group);
      } else
         return false;//E RREXIT(cinfo, JERR_FRACT_SAMPLE_NOTIMPL);
      if (need_buffer) {
         upsample->color_buf[ci] = (*cinfo->mem.AllocSarray) (cinfo, (JDIMENSION) jround_up(cinfo->output_width, cinfo->max_h_samp_factor), (JDIMENSION) cinfo->max_v_samp_factor);
         if(!upsample->color_buf[ci])
            return false;
      }
   }
   return true;
}

//----------------------------

#ifndef EXIT_FAILURE    /* define exit() codes if not provided */
#define EXIT_FAILURE  1
#endif

//----------------------------

#ifdef DCT_IFAST_SUPPORTED


/*
 * This module is specialized to the case DCTSIZE = 8.
 */

#if DCTSIZE != 8
#error Sorry, this code only copes with 8x8 DCTs. /* deliberate syntax err */
#endif


/* Scaling decisions are generally the same as in the LL&M algorithm;
 * see jidctint.c for more details.  However, we choose to descale
 * (right shift) multiplication products as soon as they are formed,
 * rather than carrying additional fractional bits into subsequent additions.
 * This compromises accuracy slightly, but it lets us save a few shifts.
 * More importantly, 16-bit arithmetic is then adequate (for 8-bit samples)
 * everywhere except in the multiplications proper; this saves a good deal
 * of work on 16-bit-int machines.
 *
 * The dequantized coefficients are not integers because the AA&N scaling
 * factors have been incorporated.  We represent them scaled up by PASS1_BITS,
 * so that the first and second IDCT rounds have the same input scaling.
 * For 8-bit JSAMPLEs, we choose IFAST_SCALE_BITS = PASS1_BITS so as to
 * avoid a descaling shift; this compromises accuracy rather drastically
 * for small quantization table entries, but it saves a lot of shifts.
 * For 12-bit JSAMPLEs, there's no hope of using 16x16 multiplies anyway,
 * so we use a much larger scaling factor to preserve accuracy.
 *
 * A final compromise is to represent the multiplicative constants to only
 * 8 fractional bits, rather than 13.  This saves some shifting work on some
 * machines, and may also reduce the cost of multiplication (since there
 * are fewer one-bits in the constants).
 */

#if BITS_IN_JSAMPLE == 8
#define CONST_BITS1  8
#define PASS1_BITS  2
#else
#define CONST_BITS1  8
#define PASS1_BITS  1      /* lose a little precision to avoid overflow */
#endif

/* Some C compilers fail to reduce "FIX(constant)" at compile time, thus
 * causing a lot of useless floating-point operations at run time.
 * To get around this we use the following pre-calculated constants.
 * If you change CONST_BITS1 you may want to add appropriate values.
 * (With a reasonable C compiler, you can just rely on the FIX() macro...)
 */

#if CONST_BITS1 == 8
#define FIX_1_082392200  ((int)  277)      /* FIX(1.082392200) */
#define FIX_1_414213562  ((int)  362)      /* FIX(1.414213562) */
#define FIX_1_847759065  ((int)  473)      /* FIX(1.847759065) */
#define FIX_2_613125930  ((int)  669)      /* FIX(2.613125930) */
#else
#define FIX_1_082392200  FIX(1.082392200)
#define FIX_1_414213562  FIX(1.414213562)
#define FIX_1_847759065  FIX(1.847759065)
#define FIX_2_613125930  FIX(2.613125930)
#endif


/* We can gain a little more speed, with a further compromise in accuracy,
 * by omitting the addition in a descaling shift.  This yields an incorrectly
 * rounded result half the time...
 */

#ifndef USE_ACCURATE_ROUNDING
#undef DESCALE
#define DESCALE(x,n)  RIGHT_SHIFT(x, n)
#endif


/* Multiply a DCTELEM variable by an int constant, and immediately
 * descale to yield a DCTELEM result.
 */

#define MULTIPLY(var,const)  ((DCTELEM) DESCALE((var) * (const), CONST_BITS1))


/* Dequantize a coefficient by multiplying it by the multiplier-table
 * entry; produce a DCTELEM result.  For 8-bit data a 16x16->16
 * multiplication will do.  For 12-bit data, the multiplier table is
 * declared int, so a 32-bit multiply will be used.
 */

#if BITS_IN_JSAMPLE == 8
#define DEQUANTIZE1(coef,quantval)  (((IFAST_MULT_TYPE) (coef)) * (quantval))
#else
#define DEQUANTIZE1(coef,quantval)  \
   DESCALE((coef)*(quantval), IFAST_SCALE_BITS-PASS1_BITS)
#endif


/* Like DESCALE, but applies to a DCTELEM and produces an int.
 * We assume that int right shift is unsigned if int right shift is.
 */

#define IRIGHT_SHIFT(x,shft)  ((x) >> (shft))

#ifdef USE_ACCURATE_ROUNDING
#define IDESCALE(x,n)  ((int) IRIGHT_SHIFT((x) + (1 << ((n)-1)), n))
#else
#define IDESCALE(x,n)  ((int) IRIGHT_SHIFT(x, n))
#endif


/*
 * Perform dequantization and inverse DCT on one block of coefficients.
 */

static void jpeg_idct_ifast(jpeg_decompress_struct* cinfo, jpeg_component_info * compptr, JCOEFPTR coef_block, JSAMPARRAY output_buf, JDIMENSION output_col){

  DCTELEM tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7;
  DCTELEM tmp10, tmp11, tmp12, tmp13;
  DCTELEM z5, z10, z11, z12, z13;
  JCOEFPTR inptr;
  IFAST_MULT_TYPE * quantptr;
  int * wsptr;
  JSAMPROW outptr;
  JSAMPLE *range_limit = IDCT_range_limit(cinfo);
  int ctr;
  int workspace[DCTSIZE2]; /* buffers data between passes */

  /* Pass 1: process columns from input, store into work array. */

  inptr = coef_block;
  quantptr = compptr->dct_table.ifast_array;
  wsptr = workspace;
  for (ctr = DCTSIZE; ctr > 0; ctr--) {
    /* Due to quantization, we will usually find that many of the input
     * coefficients are zero, especially the AC terms.  We can exploit this
     * by short-circuiting the IDCT calculation for any column in which all
     * the AC terms are zero.  In that case each output is equal to the
     * DC coefficient (with scale factor as needed).
     * With typical images and quantization tables, half or more of the
     * column DCT calculations can be simplified this way.
     */
    
    if ((inptr[DCTSIZE*1] | inptr[DCTSIZE*2] | inptr[DCTSIZE*3] |
    inptr[DCTSIZE*4] | inptr[DCTSIZE*5] | inptr[DCTSIZE*6] |
    inptr[DCTSIZE*7]) == 0) {
      /* AC terms all zero */
      int dcval = (int) DEQUANTIZE1(inptr[DCTSIZE*0], quantptr[DCTSIZE*0]);

      wsptr[DCTSIZE*0] = dcval;
      wsptr[DCTSIZE*1] = dcval;
      wsptr[DCTSIZE*2] = dcval;
      wsptr[DCTSIZE*3] = dcval;
      wsptr[DCTSIZE*4] = dcval;
      wsptr[DCTSIZE*5] = dcval;
      wsptr[DCTSIZE*6] = dcval;
      wsptr[DCTSIZE*7] = dcval;
      
      inptr++;       /* advance pointers to next column */
      quantptr++;
      wsptr++;
      continue;
    }
    
    /* Even part */

    tmp0 = DEQUANTIZE1(inptr[DCTSIZE*0], quantptr[DCTSIZE*0]);
    tmp1 = DEQUANTIZE1(inptr[DCTSIZE*2], quantptr[DCTSIZE*2]);
    tmp2 = DEQUANTIZE1(inptr[DCTSIZE*4], quantptr[DCTSIZE*4]);
    tmp3 = DEQUANTIZE1(inptr[DCTSIZE*6], quantptr[DCTSIZE*6]);

    tmp10 = tmp0 + tmp2;   /* phase 3 */
    tmp11 = tmp0 - tmp2;

    tmp13 = tmp1 + tmp3;   /* phases 5-3 */
    tmp12 = MULTIPLY(tmp1 - tmp3, FIX_1_414213562) - tmp13; /* 2*c4 */

    tmp0 = tmp10 + tmp13;  /* phase 2 */
    tmp3 = tmp10 - tmp13;
    tmp1 = tmp11 + tmp12;
    tmp2 = tmp11 - tmp12;
    
    /* Odd part */

    tmp4 = DEQUANTIZE1(inptr[DCTSIZE*1], quantptr[DCTSIZE*1]);
    tmp5 = DEQUANTIZE1(inptr[DCTSIZE*3], quantptr[DCTSIZE*3]);
    tmp6 = DEQUANTIZE1(inptr[DCTSIZE*5], quantptr[DCTSIZE*5]);
    tmp7 = DEQUANTIZE1(inptr[DCTSIZE*7], quantptr[DCTSIZE*7]);

    z13 = tmp6 + tmp5;     /* phase 6 */
    z10 = tmp6 - tmp5;
    z11 = tmp4 + tmp7;
    z12 = tmp4 - tmp7;

    tmp7 = z11 + z13;      /* phase 5 */
    tmp11 = MULTIPLY(z11 - z13, FIX_1_414213562); /* 2*c4 */

    z5 = MULTIPLY(z10 + z12, FIX_1_847759065); /* 2*c2 */
    tmp10 = MULTIPLY(z12, FIX_1_082392200) - z5; /* 2*(c2-c6) */
    tmp12 = MULTIPLY(z10, - FIX_2_613125930) + z5; /* -2*(c2+c6) */

    tmp6 = tmp12 - tmp7;   /* phase 2 */
    tmp5 = tmp11 - tmp6;
    tmp4 = tmp10 + tmp5;

    wsptr[DCTSIZE*0] = (int) (tmp0 + tmp7);
    wsptr[DCTSIZE*7] = (int) (tmp0 - tmp7);
    wsptr[DCTSIZE*1] = (int) (tmp1 + tmp6);
    wsptr[DCTSIZE*6] = (int) (tmp1 - tmp6);
    wsptr[DCTSIZE*2] = (int) (tmp2 + tmp5);
    wsptr[DCTSIZE*5] = (int) (tmp2 - tmp5);
    wsptr[DCTSIZE*4] = (int) (tmp3 + tmp4);
    wsptr[DCTSIZE*3] = (int) (tmp3 - tmp4);

    inptr++;         /* advance pointers to next column */
    quantptr++;
    wsptr++;
  }
  
  /* Pass 2: process rows from work array, store into output array. */
  /* Note that we must descale the results by a factor of 8 == 2**3, */
  /* and also undo the PASS1_BITS scaling. */

  wsptr = workspace;
  for (ctr = 0; ctr < DCTSIZE; ctr++) {
    outptr = output_buf[ctr] + output_col;
    /* Rows of zeroes can be exploited in the same way as we did with columns.
     * However, the column calculation has created many nonzero AC terms, so
     * the simplification applies less often (typically 5% to 10% of the time).
     * On machines with very fast multiplication, it's possible that the
     * test takes more time than it's worth.  In that case this section
     * may be commented out.
     */
    
#ifndef NO_ZERO_ROW_TEST
    if ((wsptr[1] | wsptr[2] | wsptr[3] | wsptr[4] | wsptr[5] | wsptr[6] |
    wsptr[7]) == 0) {
      /* AC terms all zero */
      JSAMPLE dcval = range_limit[IDESCALE(wsptr[0], PASS1_BITS+3)
              & RANGE_MASK];
      
      outptr[0] = dcval;
      outptr[1] = dcval;
      outptr[2] = dcval;
      outptr[3] = dcval;
      outptr[4] = dcval;
      outptr[5] = dcval;
      outptr[6] = dcval;
      outptr[7] = dcval;

      wsptr += DCTSIZE;    /* advance pointer to next row */
      continue;
    }
#endif
    
    /* Even part */

    tmp10 = ((DCTELEM) wsptr[0] + (DCTELEM) wsptr[4]);
    tmp11 = ((DCTELEM) wsptr[0] - (DCTELEM) wsptr[4]);

    tmp13 = ((DCTELEM) wsptr[2] + (DCTELEM) wsptr[6]);
    tmp12 = MULTIPLY((DCTELEM) wsptr[2] - (DCTELEM) wsptr[6], FIX_1_414213562)
       - tmp13;

    tmp0 = tmp10 + tmp13;
    tmp3 = tmp10 - tmp13;
    tmp1 = tmp11 + tmp12;
    tmp2 = tmp11 - tmp12;

    /* Odd part */

    z13 = (DCTELEM) wsptr[5] + (DCTELEM) wsptr[3];
    z10 = (DCTELEM) wsptr[5] - (DCTELEM) wsptr[3];
    z11 = (DCTELEM) wsptr[1] + (DCTELEM) wsptr[7];
    z12 = (DCTELEM) wsptr[1] - (DCTELEM) wsptr[7];

    tmp7 = z11 + z13;      /* phase 5 */
    tmp11 = MULTIPLY(z11 - z13, FIX_1_414213562); /* 2*c4 */

    z5 = MULTIPLY(z10 + z12, FIX_1_847759065); /* 2*c2 */
    tmp10 = MULTIPLY(z12, FIX_1_082392200) - z5; /* 2*(c2-c6) */
    tmp12 = MULTIPLY(z10, - FIX_2_613125930) + z5; /* -2*(c2+c6) */

    tmp6 = tmp12 - tmp7;   /* phase 2 */
    tmp5 = tmp11 - tmp6;
    tmp4 = tmp10 + tmp5;

    /* Final output stage: scale down by a factor of 8 and range-limit */

    outptr[0] = range_limit[IDESCALE(tmp0 + tmp7, PASS1_BITS+3)
             & RANGE_MASK];
    outptr[7] = range_limit[IDESCALE(tmp0 - tmp7, PASS1_BITS+3)
             & RANGE_MASK];
    outptr[1] = range_limit[IDESCALE(tmp1 + tmp6, PASS1_BITS+3)
             & RANGE_MASK];
    outptr[6] = range_limit[IDESCALE(tmp1 - tmp6, PASS1_BITS+3)
             & RANGE_MASK];
    outptr[2] = range_limit[IDESCALE(tmp2 + tmp5, PASS1_BITS+3)
             & RANGE_MASK];
    outptr[5] = range_limit[IDESCALE(tmp2 - tmp5, PASS1_BITS+3)
             & RANGE_MASK];
    outptr[4] = range_limit[IDESCALE(tmp3 + tmp4, PASS1_BITS+3)
             & RANGE_MASK];
    outptr[3] = range_limit[IDESCALE(tmp3 - tmp4, PASS1_BITS+3)
             & RANGE_MASK];

    wsptr += DCTSIZE;      /* advance pointer to next row */
  }
}

#endif //DCT_IFAST_SUPPORTED

//----------------------------

#ifdef DCT_ISLOW_SUPPORTED
/*
 * This module is specialized to the case DCTSIZE = 8.
 */

#if DCTSIZE != 8
#error Sorry, this code only copes with 8x8 DCTs. /* deliberate syntax err */
#endif


/*
 * The poop on this scaling stuff is as follows:
 *
 * Each 1-D IDCT step produces outputs which are a factor of sqrt(N)
 * larger than the true IDCT outputs.  The final outputs are therefore
 * a factor of N larger than desired; since N=8 this can be cured by
 * a simple right shift at the end of the algorithm.  The advantage of
 * this arrangement is that we save two multiplications per 1-D IDCT,
 * because the y0 and y4 inputs need not be divided by sqrt(N).
 *
 * We have to do addition and subtraction of the integer inputs, which
 * is no problem, and multiplication by fractional constants, which is
 * a problem to do in integer arithmetic.  We multiply all the constants
 * by CONST_SCALE and convert them to integer constants (thus retaining
 * CONST_BITS bits of precision in the constants).  After doing a
 * multiplication we have to divide the product by CONST_SCALE, with proper
 * rounding, to produce the correct output.  This division can be done
 * cheaply as a right shift of CONST_BITS bits.  We postpone shifting
 * as long as possible so that partial sums can be added together with
 * full fractional precision.
 *
 * The outputs of the first pass are scaled up by PASS1_BITS bits so that
 * they are represented to better-than-integral precision.  These outputs
 * require BITS_IN_JSAMPLE + PASS1_BITS + 3 bits; this fits in a 16-bit word
 * with the recommended scaling.  (To scale up 12-bit sample data further, an
 * intermediate int array would be needed.)
 *
 * To avoid overflow of the 32-bit intermediate results in pass 2, we must
 * have BITS_IN_JSAMPLE + CONST_BITS + PASS1_BITS <= 26.  Error analysis
 * shows that the values given below are the most effective.
 */

#if BITS_IN_JSAMPLE == 8
#define CONST_BITS2  13
#define PASS1_BITS  2
#else
#define CONST_BITS2  13
#define PASS1_BITS  1      /* lose a little precision to avoid overflow */
#endif

/* Some C compilers fail to reduce "FIX(constant)" at compile time, thus
 * causing a lot of useless floating-point operations at run time.
 * To get around this we use the following pre-calculated constants.
 * If you change CONST_BITS2 you may want to add appropriate values.
 * (With a reasonable C compiler, you can just rely on the FIX() macro...)
 */

#if CONST_BITS2 == 13
#define FIX_0_298631336  ((int)  2446)  /* FIX(0.298631336) */
#define FIX_0_390180644  ((int)  3196)  /* FIX(0.390180644) */
#define FIX_0_541196100  ((int)  4433)  /* FIX(0.541196100) */
#define FIX_0_765366865  ((int)  6270)  /* FIX(0.765366865) */
#define FIX_0_899976223  ((int)  7373)  /* FIX(0.899976223) */
#define FIX_1_175875602  ((int)  9633)  /* FIX(1.175875602) */
#define FIX_1_501321110  ((int)  12299) /* FIX(1.501321110) */
#define FIX_1_847759065a  ((int)  15137) /* FIX(1.847759065) */
#define FIX_1_961570560  ((int)  16069) /* FIX(1.961570560) */
#define FIX_2_053119869  ((int)  16819) /* FIX(2.053119869) */
#define FIX_2_562915447  ((int)  20995) /* FIX(2.562915447) */
#define FIX_3_072711026  ((int)  25172) /* FIX(3.072711026) */
#else
#define FIX_0_298631336  FIX(0.298631336)
#define FIX_0_390180644  FIX(0.390180644)
#define FIX_0_541196100  FIX(0.541196100)
#define FIX_0_765366865  FIX(0.765366865)
#define FIX_0_899976223  FIX(0.899976223)
#define FIX_1_175875602  FIX(1.175875602)
#define FIX_1_501321110  FIX(1.501321110)
#define FIX_1_847759065a  FIX(1.847759065)
#define FIX_1_961570560  FIX(1.961570560)
#define FIX_2_053119869  FIX(2.053119869)
#define FIX_2_562915447  FIX(2.562915447)
#define FIX_3_072711026  FIX(3.072711026)
#endif


/* Multiply an int variable by an int constant to yield an int result.
 * For 8-bit samples with the recommended scaling, all the variable
 * and constant values involved are no more than 16 bits wide, so a
 * 16x16->32 bit multiply can be used instead of a full 32x32 multiply.
 * For 12-bit samples, a full 32-bit multiplication will be needed.
 */

#if BITS_IN_JSAMPLE == 8
#define MULTIPLY1(var,const)  MULTIPLY16C16(var,const)
#else
#define MULTIPLY1(var,const)  ((var) * (const))
#endif


/* Dequantize a coefficient by MULTIPLY1ing it by the multiplier-table
 * entry; produce an int result.  In this module, both inputs and result
 * are 16 bits or less, so either int or short MULTIPLY1 will work.
 */

#define DEQUANTIZE2(coef,quantval)  (((ISLOW_MULT_TYPE) (coef)) * (quantval))

//----------------------------
/*
 * Perform dequantization and inverse DCT on one block of coefficients.
 */
static void jpeg_idct_islow (jpeg_decompress_struct* cinfo, jpeg_component_info * compptr,
   JCOEFPTR coef_block, JSAMPARRAY output_buf, JDIMENSION output_col){

   int tmp0, tmp1, tmp2, tmp3;
   int tmp10, tmp11, tmp12, tmp13;
   int z1, z2, z3, z4, z5;
   JCOEFPTR inptr;
   ISLOW_MULT_TYPE * quantptr;
   int * wsptr;
   JSAMPROW outptr;
   JSAMPLE *range_limit = IDCT_range_limit(cinfo);
   int ctr;
   int workspace[DCTSIZE2]; /* buffers data between passes */

  /* Pass 1: process columns from input, store into work array. */
  /* Note results are scaled up by sqrt(8) compared to a true IDCT; */
  /* furthermore, we scale the results by 2**PASS1_BITS. */

   inptr = coef_block;
   quantptr = (ISLOW_MULT_TYPE *) compptr->dct_table;
   wsptr = workspace;
   for (ctr = DCTSIZE; ctr > 0; ctr--) {
    /* Due to quantization, we will usually find that many of the input
     * coefficients are zero, especially the AC terms.  We can exploit this
     * by short-circuiting the IDCT calculation for any column in which all
     * the AC terms are zero.  In that case each output is equal to the
     * DC coefficient (with scale factor as needed).
     * With typical images and quantization tables, half or more of the
     * column DCT calculations can be simplified this way.
     */
    
      if ((inptr[DCTSIZE*1] | inptr[DCTSIZE*2] | inptr[DCTSIZE*3] |
         inptr[DCTSIZE*4] | inptr[DCTSIZE*5] | inptr[DCTSIZE*6] |
         inptr[DCTSIZE*7]) == 0) {
      /* AC terms all zero */
         int dcval = DEQUANTIZE2(inptr[DCTSIZE*0], quantptr[DCTSIZE*0]) << PASS1_BITS;
      
         wsptr[DCTSIZE*0] = dcval;
         wsptr[DCTSIZE*1] = dcval;
         wsptr[DCTSIZE*2] = dcval;
         wsptr[DCTSIZE*3] = dcval;
         wsptr[DCTSIZE*4] = dcval;
         wsptr[DCTSIZE*5] = dcval;
         wsptr[DCTSIZE*6] = dcval;
         wsptr[DCTSIZE*7] = dcval;
      
         inptr++;       /* advance pointers to next column */
         quantptr++;
         wsptr++;
         continue;
      }
    
    /* Even part: reverse the even part of the forward DCT. */
    /* The rotator is sqrt(2)*c(-6). */
    
      z2 = DEQUANTIZE2(inptr[DCTSIZE*2], quantptr[DCTSIZE*2]);
      z3 = DEQUANTIZE2(inptr[DCTSIZE*6], quantptr[DCTSIZE*6]);
    
      z1 = MULTIPLY1(z2 + z3, FIX_0_541196100);
      tmp2 = z1 + MULTIPLY1(z3, - FIX_1_847759065a);
      tmp3 = z1 + MULTIPLY1(z2, FIX_0_765366865);
    
      z2 = DEQUANTIZE2(inptr[DCTSIZE*0], quantptr[DCTSIZE*0]);
      z3 = DEQUANTIZE2(inptr[DCTSIZE*4], quantptr[DCTSIZE*4]);

      tmp0 = (z2 + z3) << CONST_BITS2;
      tmp1 = (z2 - z3) << CONST_BITS2;
    
      tmp10 = tmp0 + tmp3;
      tmp13 = tmp0 - tmp3;
      tmp11 = tmp1 + tmp2;
      tmp12 = tmp1 - tmp2;
    
    /* Odd part per figure 8; the matrix is unitary and hence its
     * transpose is its inverse.  i0..i3 are y7,y5,y3,y1 respectively.
     */
    
      tmp0 = DEQUANTIZE2(inptr[DCTSIZE*7], quantptr[DCTSIZE*7]);
      tmp1 = DEQUANTIZE2(inptr[DCTSIZE*5], quantptr[DCTSIZE*5]);
      tmp2 = DEQUANTIZE2(inptr[DCTSIZE*3], quantptr[DCTSIZE*3]);
      tmp3 = DEQUANTIZE2(inptr[DCTSIZE*1], quantptr[DCTSIZE*1]);
    
      z1 = tmp0 + tmp3;
      z2 = tmp1 + tmp2;
      z3 = tmp0 + tmp2;
      z4 = tmp1 + tmp3;
      z5 = MULTIPLY1(z3 + z4, FIX_1_175875602); /* sqrt(2) * c3 */
    
      tmp0 = MULTIPLY1(tmp0, FIX_0_298631336); /* sqrt(2) * (-c1+c3+c5-c7) */
      tmp1 = MULTIPLY1(tmp1, FIX_2_053119869); /* sqrt(2) * ( c1+c3-c5+c7) */
      tmp2 = MULTIPLY1(tmp2, FIX_3_072711026); /* sqrt(2) * ( c1+c3+c5-c7) */
      tmp3 = MULTIPLY1(tmp3, FIX_1_501321110); /* sqrt(2) * ( c1+c3-c5-c7) */
      z1 = MULTIPLY1(z1, - FIX_0_899976223); /* sqrt(2) * (c7-c3) */
      z2 = MULTIPLY1(z2, - FIX_2_562915447); /* sqrt(2) * (-c1-c3) */
      z3 = MULTIPLY1(z3, - FIX_1_961570560); /* sqrt(2) * (-c3-c5) */
      z4 = MULTIPLY1(z4, - FIX_0_390180644); /* sqrt(2) * (c5-c3) */
    
      z3 += z5;
      z4 += z5;
    
      tmp0 += z1 + z3;
      tmp1 += z2 + z4;
      tmp2 += z2 + z3;
      tmp3 += z1 + z4;
    
    /* Final output stage: inputs are tmp10..tmp13, tmp0..tmp3 */
    
      wsptr[DCTSIZE*0] = (int) DESCALE(tmp10 + tmp3, CONST_BITS2-PASS1_BITS);
      wsptr[DCTSIZE*7] = (int) DESCALE(tmp10 - tmp3, CONST_BITS2-PASS1_BITS);
      wsptr[DCTSIZE*1] = (int) DESCALE(tmp11 + tmp2, CONST_BITS2-PASS1_BITS);
      wsptr[DCTSIZE*6] = (int) DESCALE(tmp11 - tmp2, CONST_BITS2-PASS1_BITS);
      wsptr[DCTSIZE*2] = (int) DESCALE(tmp12 + tmp1, CONST_BITS2-PASS1_BITS);
      wsptr[DCTSIZE*5] = (int) DESCALE(tmp12 - tmp1, CONST_BITS2-PASS1_BITS);
      wsptr[DCTSIZE*3] = (int) DESCALE(tmp13 + tmp0, CONST_BITS2-PASS1_BITS);
      wsptr[DCTSIZE*4] = (int) DESCALE(tmp13 - tmp0, CONST_BITS2-PASS1_BITS);
    
      inptr++;         /* advance pointers to next column */
      quantptr++;
      wsptr++;
   }
  
  /* Pass 2: process rows from work array, store into output array. */
  /* Note that we must descale the results by a factor of 8 == 2**3, */
  /* and also undo the PASS1_BITS scaling. */

   wsptr = workspace;
   for (ctr = 0; ctr < DCTSIZE; ctr++) {
      outptr = output_buf[ctr] + output_col;
    /* Rows of zeroes can be exploited in the same way as we did with columns.
     * However, the column calculation has created many nonzero AC terms, so
     * the simplification applies less often (typically 5% to 10% of the time).
     * On machines with very fast multiplication, it's possible that the
     * test takes more time than it's worth.  In that case this section
     * may be commented out.
     */
    
#ifndef NO_ZERO_ROW_TEST
      if ((wsptr[1] | wsptr[2] | wsptr[3] | wsptr[4] | wsptr[5] | wsptr[6] |
         wsptr[7]) == 0) {
      /* AC terms all zero */
         JSAMPLE dcval = range_limit[(int) DESCALE((int) wsptr[0], PASS1_BITS+3)
                 & RANGE_MASK];
      
         outptr[0] = dcval;
         outptr[1] = dcval;
         outptr[2] = dcval;
         outptr[3] = dcval;
         outptr[4] = dcval;
         outptr[5] = dcval;
         outptr[6] = dcval;
         outptr[7] = dcval;

         wsptr += DCTSIZE;    /* advance pointer to next row */
         continue;
      }
#endif
    
    /* Even part: reverse the even part of the forward DCT. */
    /* The rotator is sqrt(2)*c(-6). */
    
      z2 = (int) wsptr[2];
      z3 = (int) wsptr[6];
    
      z1 = MULTIPLY1(z2 + z3, FIX_0_541196100);
      tmp2 = z1 + MULTIPLY1(z3, - FIX_1_847759065a);
      tmp3 = z1 + MULTIPLY1(z2, FIX_0_765366865);
    
      tmp0 = ((int) wsptr[0] + (int) wsptr[4]) << CONST_BITS2;
      tmp1 = ((int) wsptr[0] - (int) wsptr[4]) << CONST_BITS2;
    
      tmp10 = tmp0 + tmp3;
      tmp13 = tmp0 - tmp3;
      tmp11 = tmp1 + tmp2;
      tmp12 = tmp1 - tmp2;
    
    /* Odd part per figure 8; the matrix is unitary and hence its
     * transpose is its inverse.  i0..i3 are y7,y5,y3,y1 respectively.
     */
    
      tmp0 = (int) wsptr[7];
      tmp1 = (int) wsptr[5];
      tmp2 = (int) wsptr[3];
      tmp3 = (int) wsptr[1];
    
      z1 = tmp0 + tmp3;
      z2 = tmp1 + tmp2;
      z3 = tmp0 + tmp2;
      z4 = tmp1 + tmp3;
      z5 = MULTIPLY1(z3 + z4, FIX_1_175875602); /* sqrt(2) * c3 */
    
      tmp0 = MULTIPLY1(tmp0, FIX_0_298631336); /* sqrt(2) * (-c1+c3+c5-c7) */
      tmp1 = MULTIPLY1(tmp1, FIX_2_053119869); /* sqrt(2) * ( c1+c3-c5+c7) */
      tmp2 = MULTIPLY1(tmp2, FIX_3_072711026); /* sqrt(2) * ( c1+c3+c5-c7) */
      tmp3 = MULTIPLY1(tmp3, FIX_1_501321110); /* sqrt(2) * ( c1+c3-c5-c7) */
      z1 = MULTIPLY1(z1, - FIX_0_899976223); /* sqrt(2) * (c7-c3) */
      z2 = MULTIPLY1(z2, - FIX_2_562915447); /* sqrt(2) * (-c1-c3) */
      z3 = MULTIPLY1(z3, - FIX_1_961570560); /* sqrt(2) * (-c3-c5) */
      z4 = MULTIPLY1(z4, - FIX_0_390180644); /* sqrt(2) * (c5-c3) */
    
      z3 += z5;
      z4 += z5;
    
    tmp0 += z1 + z3;
    tmp1 += z2 + z4;
    tmp2 += z2 + z3;
    tmp3 += z1 + z4;
    
    /* Final output stage: inputs are tmp10..tmp13, tmp0..tmp3 */
    
    outptr[0] = range_limit[(int) DESCALE(tmp10 + tmp3,
                 CONST_BITS2+PASS1_BITS+3)
             & RANGE_MASK];
    outptr[7] = range_limit[(int) DESCALE(tmp10 - tmp3,
                 CONST_BITS2+PASS1_BITS+3)
             & RANGE_MASK];
    outptr[1] = range_limit[(int) DESCALE(tmp11 + tmp2,
                 CONST_BITS2+PASS1_BITS+3)
             & RANGE_MASK];
    outptr[6] = range_limit[(int) DESCALE(tmp11 - tmp2,
                 CONST_BITS2+PASS1_BITS+3)
             & RANGE_MASK];
    outptr[2] = range_limit[(int) DESCALE(tmp12 + tmp1,
                 CONST_BITS2+PASS1_BITS+3)
             & RANGE_MASK];
    outptr[5] = range_limit[(int) DESCALE(tmp12 - tmp1,
                 CONST_BITS2+PASS1_BITS+3)
             & RANGE_MASK];
    outptr[3] = range_limit[(int) DESCALE(tmp13 + tmp0,
                 CONST_BITS2+PASS1_BITS+3)
             & RANGE_MASK];
    outptr[4] = range_limit[(int) DESCALE(tmp13 - tmp0,
                 CONST_BITS2+PASS1_BITS+3)
             & RANGE_MASK];
    
    wsptr += DCTSIZE;      /* advance pointer to next row */
   }
}

//----------------------------
#endif //DCT_ISLOW_SUPPORTED

//----------------------------

#ifdef IDCT_SCALING_SUPPORTED


/*
 * This module is specialized to the case DCTSIZE = 8.
 */

#if DCTSIZE != 8
#error Sorry, this code only copes with 8x8 DCTs. /* deliberate syntax err */
#endif


/* Scaling is the same as in jidctint.c. */

#if BITS_IN_JSAMPLE == 8
#define CONST_BITS3  13
#define PASS1_BITS  2
#else
#define CONST_BITS3  13
#define PASS1_BITS  1      /* lose a little precision to avoid overflow */
#endif

/* Some C compilers fail to reduce "FIX(constant)" at compile time, thus
 * causing a lot of useless floating-point operations at run time.
 * To get around this we use the following pre-calculated constants.
 * If you change CONST_BITS3 you may want to add appropriate values.
 * (With a reasonable C compiler, you can just rely on the FIX() macro...)
 */

#if CONST_BITS3 == 13
#define FIX_0_211164243  ((int)  1730)  /* FIX(0.211164243) */
#define FIX_0_509795579  ((int)  4176)  /* FIX(0.509795579) */
#define FIX_0_601344887  ((int)  4926)  /* FIX(0.601344887) */
#define FIX_0_720959822  ((int)  5906)  /* FIX(0.720959822) */
#define FIX_0_765366865  ((int)  6270)  /* FIX(0.765366865) */
#define FIX_0_850430095  ((int)  6967)  /* FIX(0.850430095) */
#define FIX_0_899976223  ((int)  7373)  /* FIX(0.899976223) */
#define FIX_1_061594337  ((int)  8697)  /* FIX(1.061594337) */
#define FIX_1_272758580  ((int)  10426) /* FIX(1.272758580) */
#define FIX_1_451774981  ((int)  11893) /* FIX(1.451774981) */
#define FIX_1_847759065b  ((int)  15137) /* FIX(1.847759065) */
#define FIX_2_172734803  ((int)  17799) /* FIX(2.172734803) */
#define FIX_2_562915447  ((int)  20995) /* FIX(2.562915447) */
#define FIX_3_624509785  ((int)  29692) /* FIX(3.624509785) */
#else
#define FIX_0_211164243  FIX(0.211164243)
#define FIX_0_509795579  FIX(0.509795579)
#define FIX_0_601344887  FIX(0.601344887)
#define FIX_0_720959822  FIX(0.720959822)
#define FIX_0_765366865  FIX(0.765366865)
#define FIX_0_850430095  FIX(0.850430095)
#define FIX_0_899976223  FIX(0.899976223)
#define FIX_1_061594337  FIX(1.061594337)
#define FIX_1_272758580  FIX(1.272758580)
#define FIX_1_451774981  FIX(1.451774981)
#define FIX_1_847759065b  FIX(1.847759065)
#define FIX_2_172734803  FIX(2.172734803)
#define FIX_2_562915447  FIX(2.562915447)
#define FIX_3_624509785  FIX(3.624509785)
#endif


/* Multiply an int variable by an int constant to yield an int result.
 * For 8-bit samples with the recommended scaling, all the variable
 * and constant values involved are no more than 16 bits wide, so a
 * 16x16->32 bit multiply can be used instead of a full 32x32 multiply.
 * For 12-bit samples, a full 32-bit multiplication will be needed.
 */

#if BITS_IN_JSAMPLE == 8
#define MULTIPLY5(var,const)  MULTIPLY16C16(var,const)
#else
#define MULTIPLY5(var,const)  ((var) * (const))
#endif


/* Dequantize a coefficient by multiplying it by the multiplier-table
 * entry; produce an int result.  In this module, both inputs and result
 * are 16 bits or less, so either int or short MULTIPLY5 will work.
 */

#define DEQUANTIZE5(coef,quantval)  (((ISLOW_MULT_TYPE) (coef)) * (quantval))


/*
 * Perform dequantization and inverse DCT on one block of coefficients,
 * producing a reduced-size 4x4 output block.
 */

static void jpeg_idct_4x4 (jpeg_decompress_struct* cinfo, jpeg_component_info * compptr, JCOEFPTR coef_block, JSAMPARRAY output_buf, JDIMENSION output_col){
  int tmp0, tmp2, tmp10, tmp12;
  int z1, z2, z3, z4;
  JCOEFPTR inptr;
  ISLOW_MULT_TYPE * quantptr;
  int * wsptr;
  JSAMPROW outptr;
  JSAMPLE *range_limit = IDCT_range_limit(cinfo);
  int ctr;
  int workspace[DCTSIZE*4];   /* buffers data between passes */

  /* Pass 1: process columns from input, store into work array. */

  inptr = coef_block;
  quantptr = compptr->dct_table.islow_array;
  wsptr = workspace;
  for (ctr = DCTSIZE; ctr > 0; inptr++, quantptr++, wsptr++, ctr--) {
    /* Don't bother to process column 4, because second pass won't use it */
    if (ctr == DCTSIZE-4)
      continue;
    if ((inptr[DCTSIZE*1] | inptr[DCTSIZE*2] | inptr[DCTSIZE*3] |
    inptr[DCTSIZE*5] | inptr[DCTSIZE*6] | inptr[DCTSIZE*7]) == 0) {
      /* AC terms all zero; we need not examine term 4 for 4x4 output */
      int dcval = DEQUANTIZE5(inptr[DCTSIZE*0], quantptr[DCTSIZE*0]) << PASS1_BITS;
      
      wsptr[DCTSIZE*0] = dcval;
      wsptr[DCTSIZE*1] = dcval;
      wsptr[DCTSIZE*2] = dcval;
      wsptr[DCTSIZE*3] = dcval;
      
      continue;
    }
    
    /* Even part */
    
    tmp0 = DEQUANTIZE5(inptr[DCTSIZE*0], quantptr[DCTSIZE*0]);
    tmp0 <<= (CONST_BITS3+1);
    
    z2 = DEQUANTIZE5(inptr[DCTSIZE*2], quantptr[DCTSIZE*2]);
    z3 = DEQUANTIZE5(inptr[DCTSIZE*6], quantptr[DCTSIZE*6]);

    tmp2 = MULTIPLY5(z2, FIX_1_847759065b) + MULTIPLY5(z3, - FIX_0_765366865);
    
    tmp10 = tmp0 + tmp2;
    tmp12 = tmp0 - tmp2;
    
    /* Odd part */
    
    z1 = DEQUANTIZE5(inptr[DCTSIZE*7], quantptr[DCTSIZE*7]);
    z2 = DEQUANTIZE5(inptr[DCTSIZE*5], quantptr[DCTSIZE*5]);
    z3 = DEQUANTIZE5(inptr[DCTSIZE*3], quantptr[DCTSIZE*3]);
    z4 = DEQUANTIZE5(inptr[DCTSIZE*1], quantptr[DCTSIZE*1]);
    
    tmp0 = MULTIPLY5(z1, - FIX_0_211164243) /* sqrt(2) * (c3-c1) */
    + MULTIPLY5(z2, FIX_1_451774981) /* sqrt(2) * (c3+c7) */
    + MULTIPLY5(z3, - FIX_2_172734803) /* sqrt(2) * (-c1-c5) */
    + MULTIPLY5(z4, FIX_1_061594337); /* sqrt(2) * (c5+c7) */
    
    tmp2 = MULTIPLY5(z1, - FIX_0_509795579) /* sqrt(2) * (c7-c5) */
    + MULTIPLY5(z2, - FIX_0_601344887) /* sqrt(2) * (c5-c1) */
    + MULTIPLY5(z3, FIX_0_899976223) /* sqrt(2) * (c3-c7) */
    + MULTIPLY5(z4, FIX_2_562915447); /* sqrt(2) * (c1+c3) */

    /* Final output stage */
    
    wsptr[DCTSIZE*0] = (int) DESCALE(tmp10 + tmp2, CONST_BITS3-PASS1_BITS+1);
    wsptr[DCTSIZE*3] = (int) DESCALE(tmp10 - tmp2, CONST_BITS3-PASS1_BITS+1);
    wsptr[DCTSIZE*1] = (int) DESCALE(tmp12 + tmp0, CONST_BITS3-PASS1_BITS+1);
    wsptr[DCTSIZE*2] = (int) DESCALE(tmp12 - tmp0, CONST_BITS3-PASS1_BITS+1);
  }
  
  /* Pass 2: process 4 rows from work array, store into output array. */

  wsptr = workspace;
  for (ctr = 0; ctr < 4; ctr++) {
    outptr = output_buf[ctr] + output_col;
    /* It's not clear whether a zero row test is worthwhile here ... */

#ifndef NO_ZERO_ROW_TEST
    if ((wsptr[1] | wsptr[2] | wsptr[3] | wsptr[5] | wsptr[6] |
    wsptr[7]) == 0) {
      /* AC terms all zero */
      JSAMPLE dcval = range_limit[(int) DESCALE((int) wsptr[0], PASS1_BITS+3)
              & RANGE_MASK];
      
      outptr[0] = dcval;
      outptr[1] = dcval;
      outptr[2] = dcval;
      outptr[3] = dcval;
      
      wsptr += DCTSIZE;    /* advance pointer to next row */
      continue;
    }
#endif
    
    /* Even part */
    
    tmp0 = ((int) wsptr[0]) << (CONST_BITS3+1);
    
    tmp2 = MULTIPLY5((int) wsptr[2], FIX_1_847759065b)
    + MULTIPLY5((int) wsptr[6], - FIX_0_765366865);
    
    tmp10 = tmp0 + tmp2;
    tmp12 = tmp0 - tmp2;
    
    /* Odd part */
    
    z1 = (int) wsptr[7];
    z2 = (int) wsptr[5];
    z3 = (int) wsptr[3];
    z4 = (int) wsptr[1];
    
    tmp0 = MULTIPLY5(z1, - FIX_0_211164243) /* sqrt(2) * (c3-c1) */
    + MULTIPLY5(z2, FIX_1_451774981) /* sqrt(2) * (c3+c7) */
    + MULTIPLY5(z3, - FIX_2_172734803) /* sqrt(2) * (-c1-c5) */
    + MULTIPLY5(z4, FIX_1_061594337); /* sqrt(2) * (c5+c7) */
    
    tmp2 = MULTIPLY5(z1, - FIX_0_509795579) /* sqrt(2) * (c7-c5) */
    + MULTIPLY5(z2, - FIX_0_601344887) /* sqrt(2) * (c5-c1) */
    + MULTIPLY5(z3, FIX_0_899976223) /* sqrt(2) * (c3-c7) */
    + MULTIPLY5(z4, FIX_2_562915447); /* sqrt(2) * (c1+c3) */

    /* Final output stage */
    
    outptr[0] = range_limit[(int) DESCALE(tmp10 + tmp2,
                 CONST_BITS3+PASS1_BITS+3+1)
             & RANGE_MASK];
    outptr[3] = range_limit[(int) DESCALE(tmp10 - tmp2,
                 CONST_BITS3+PASS1_BITS+3+1)
             & RANGE_MASK];
    outptr[1] = range_limit[(int) DESCALE(tmp12 + tmp0,
                 CONST_BITS3+PASS1_BITS+3+1)
             & RANGE_MASK];
    outptr[2] = range_limit[(int) DESCALE(tmp12 - tmp0,
                 CONST_BITS3+PASS1_BITS+3+1)
             & RANGE_MASK];
    
    wsptr += DCTSIZE;      /* advance pointer to next row */
  }
}


/*
 * Perform dequantization and inverse DCT on one block of coefficients,
 * producing a reduced-size 2x2 output block.
 */

static void jpeg_idct_2x2 (jpeg_decompress_struct* cinfo, jpeg_component_info * compptr, JCOEFPTR coef_block, JSAMPARRAY output_buf, JDIMENSION output_col){
  int tmp0, tmp10, z1;
  JCOEFPTR inptr;
  ISLOW_MULT_TYPE * quantptr;
  int * wsptr;
  JSAMPROW outptr;
  JSAMPLE *range_limit = IDCT_range_limit(cinfo);
  int ctr;
  int workspace[DCTSIZE*2];   /* buffers data between passes */

  /* Pass 1: process columns from input, store into work array. */

  inptr = coef_block;
  quantptr = compptr->dct_table.islow_array;
  wsptr = workspace;
  for (ctr = DCTSIZE; ctr > 0; inptr++, quantptr++, wsptr++, ctr--) {
    /* Don't bother to process columns 2,4,6 */
    if (ctr == DCTSIZE-2 || ctr == DCTSIZE-4 || ctr == DCTSIZE-6)
      continue;
    if ((inptr[DCTSIZE*1] | inptr[DCTSIZE*3] |
    inptr[DCTSIZE*5] | inptr[DCTSIZE*7]) == 0) {
      /* AC terms all zero; we need not examine terms 2,4,6 for 2x2 output */
      int dcval = DEQUANTIZE5(inptr[DCTSIZE*0], quantptr[DCTSIZE*0]) << PASS1_BITS;
      
      wsptr[DCTSIZE*0] = dcval;
      wsptr[DCTSIZE*1] = dcval;
      
      continue;
    }
    
    /* Even part */
    
    z1 = DEQUANTIZE5(inptr[DCTSIZE*0], quantptr[DCTSIZE*0]);
    tmp10 = z1 << (CONST_BITS3+2);
    
    /* Odd part */

    z1 = DEQUANTIZE5(inptr[DCTSIZE*7], quantptr[DCTSIZE*7]);
    tmp0 = MULTIPLY5(z1, - FIX_0_720959822); /* sqrt(2) * (c7-c5+c3-c1) */
    z1 = DEQUANTIZE5(inptr[DCTSIZE*5], quantptr[DCTSIZE*5]);
    tmp0 += MULTIPLY5(z1, FIX_0_850430095); /* sqrt(2) * (-c1+c3+c5+c7) */
    z1 = DEQUANTIZE5(inptr[DCTSIZE*3], quantptr[DCTSIZE*3]);
    tmp0 += MULTIPLY5(z1, - FIX_1_272758580); /* sqrt(2) * (-c1+c3-c5-c7) */
    z1 = DEQUANTIZE5(inptr[DCTSIZE*1], quantptr[DCTSIZE*1]);
    tmp0 += MULTIPLY5(z1, FIX_3_624509785); /* sqrt(2) * (c1+c3+c5+c7) */

    /* Final output stage */
    
    wsptr[DCTSIZE*0] = (int) DESCALE(tmp10 + tmp0, CONST_BITS3-PASS1_BITS+2);
    wsptr[DCTSIZE*1] = (int) DESCALE(tmp10 - tmp0, CONST_BITS3-PASS1_BITS+2);
  }
  
  /* Pass 2: process 2 rows from work array, store into output array. */

  wsptr = workspace;
  for (ctr = 0; ctr < 2; ctr++) {
    outptr = output_buf[ctr] + output_col;
    /* It's not clear whether a zero row test is worthwhile here ... */

#ifndef NO_ZERO_ROW_TEST
    if ((wsptr[1] | wsptr[3] | wsptr[5] | wsptr[7]) == 0) {
      /* AC terms all zero */
      JSAMPLE dcval = range_limit[(int) DESCALE((int) wsptr[0], PASS1_BITS+3)
              & RANGE_MASK];
      
      outptr[0] = dcval;
      outptr[1] = dcval;
      
      wsptr += DCTSIZE;    /* advance pointer to next row */
      continue;
    }
#endif
    
    /* Even part */
    
    tmp10 = ((int) wsptr[0]) << (CONST_BITS3+2);
    
    /* Odd part */

    tmp0 = MULTIPLY5((int) wsptr[7], - FIX_0_720959822) /* sqrt(2) * (c7-c5+c3-c1) */
    + MULTIPLY5((int) wsptr[5], FIX_0_850430095) /* sqrt(2) * (-c1+c3+c5+c7) */
    + MULTIPLY5((int) wsptr[3], - FIX_1_272758580) /* sqrt(2) * (-c1+c3-c5-c7) */
    + MULTIPLY5((int) wsptr[1], FIX_3_624509785); /* sqrt(2) * (c1+c3+c5+c7) */

    /* Final output stage */
    
    outptr[0] = range_limit[(int) DESCALE(tmp10 + tmp0,
                 CONST_BITS3+PASS1_BITS+3+2)
             & RANGE_MASK];
    outptr[1] = range_limit[(int) DESCALE(tmp10 - tmp0,
                 CONST_BITS3+PASS1_BITS+3+2)
             & RANGE_MASK];
    
    wsptr += DCTSIZE;      /* advance pointer to next row */
  }
}


/*
 * Perform dequantization and inverse DCT on one block of coefficients,
 * producing a reduced-size 1x1 output block.
 */

static void jpeg_idct_1x1 (jpeg_decompress_struct* cinfo, jpeg_component_info * compptr, JCOEFPTR coef_block, JSAMPARRAY output_buf, JDIMENSION output_col){
  int dcval;
  ISLOW_MULT_TYPE * quantptr;
  JSAMPLE *range_limit = IDCT_range_limit(cinfo);

  /* We hardly need an inverse DCT routine for this: just take the
   * average pixel value, which is one-eighth of the DC coefficient.
   */
  quantptr = compptr->dct_table.islow_array;
  dcval = DEQUANTIZE5(coef_block[0], quantptr[0]);
  dcval = (int) DESCALE((int) dcval, 3);

  output_buf[0][output_col] = range_limit[dcval & RANGE_MASK];
}

#endif /* IDCT_SCALING_SUPPORTED */

//----------------------------

/*
 * The control blocks for virtual arrays.
 * Note that these blocks are allocated in the "small" pool area.
 * System-dependent info for the associated backing store (if any) is hidden
 * inside the backing_store_info struct.
 */
struct jvirt_array_base{
   JDIMENSION rows_in_array;   /* total virtual array height */
   JDIMENSION maxaccess;    /* max rows accessed by access_virt_?array */
   JDIMENSION rows_in_mem;  /* height of memory buffer */
   JDIMENSION rowsperchunk; /* allocation chunk size in mem_buffer */
   JDIMENSION cur_start_row;   /* first logical row # in the buffer */
   JDIMENSION first_undef_row; /* row # of first uninitialized row */
   bool pre_zero;     /* pre-zero mode requested? */
   bool dirty;     /* do current buffer contents need written? */
   //bool b_s_open;     /* is backing-store data valid? */
   jvirt_array_base *next;
   //backing_store_info b_s_info;   /* System-dependent control info */
};

struct jvirt_sarray_control: public jvirt_array_base{
   JSAMPARRAY mem_buffer;   /* => the in-memory buffer */
   JDIMENSION samplesperrow;   /* width of array (and of memory buffer) */
};

struct jvirt_barray_control: public jvirt_array_base{
   JBLOCKARRAY _mem_buffer;  /* => the in-memory buffer */
   JDIMENSION blocksperrow; /* width of array (and of memory buffer) */
};

//----------------------------

/*
 * Allocation of "small" objects.
 *
 * For these, we use pooled storage.  When a new pool must be created,
 * we try to get enough space for the current request plus a "slop" factor,
 * where the slop will be the amount of leftover space in the new pool.
 * The speed vs. space tradeoff is largely determined by the slop values.
 * A different slop value is provided for each pool class (lifetime),
 * and we also distinguish the first pool of a class from later ones.
 * NOTE: the values given work fairly well on both 16- and 32-bit-int
 * machines, but may be too small if longs are 64 bits or more.
 */

static const size_t first_pool_slop = 16000;       /* first IMAGE pool */

static const size_t extra_pool_slop = 5000;        /* additional IMAGE pools */

#define MIN_SLOP  50    /* greater than 0 to avoid futile looping */

//----------------------------

void *my_memory_mgr::AllocSmall(size_t sizeofobject){
   mem_pool_hdr *hdr_ptr, *prev_hdr_ptr;
   char * data_ptr;
   size_t min_request, slop;

   sizeofobject = (sizeofobject+3) & -4;
   if (sizeofobject > (size_t) (MAX_ALLOC_CHUNK-SIZEOF(mem_pool_hdr)))
      return NULL;

   prev_hdr_ptr = NULL;
   hdr_ptr = small_list;
   while (hdr_ptr != NULL) {
      if (hdr_ptr->bytes_left >= sizeofobject)
         break;
      prev_hdr_ptr = hdr_ptr;
      hdr_ptr = hdr_ptr->next;
   }
   if (hdr_ptr == NULL) {
      min_request = sizeofobject + SIZEOF(mem_pool_hdr);
      if (prev_hdr_ptr == NULL)
         slop = first_pool_slop;
      else
         slop = extra_pool_slop;
      if (slop > (size_t) (MAX_ALLOC_CHUNK-min_request))
         slop = (size_t) (MAX_ALLOC_CHUNK-min_request);
      for (;;) {
         hdr_ptr = (mem_pool_hdr *) new byte[min_request + slop];
         if (hdr_ptr != NULL)
            break;
         slop /= 2;
         if (slop < MIN_SLOP)
            return NULL;
      }
      hdr_ptr->next = NULL;
      hdr_ptr->bytes_used = 0;
      hdr_ptr->bytes_left = sizeofobject + slop;
      if (prev_hdr_ptr == NULL)
         small_list = hdr_ptr;
      else
         prev_hdr_ptr->next = hdr_ptr;
   }
   data_ptr = (char *) (hdr_ptr + 1);
   data_ptr += hdr_ptr->bytes_used;
   hdr_ptr->bytes_used += sizeofobject;
   hdr_ptr->bytes_left -= sizeofobject;

   assert(!(dword(data_ptr)&3));
   return (void *) data_ptr;
}

/*
 * Creation of 2-D sample arrays.
 * The pointers are in near heap, the samples themselves in heap.
 *
 * To minimize allocation overhead and to allow I/O of large contiguous
 * blocks, we allocate the sample rows in groups of as many rows as possible
 * without exceeding MAX_ALLOC_CHUNK total bytes per allocation request.
 * NB: the virtual array control routines, later in this file, know about
 * this chunking of rows.  The rowsperchunk value is left in the mem manager
 * object so that it can be saved away if this sarray is the workspace for
 * a virtual array.
 */

static JSAMPARRAY AllocSarray(jpeg_decompress_struct* cinfo, JDIMENSION samplesperrow, JDIMENSION numrows){
   /* Allocate a 2-D sample array */
   my_memory_mgr* mem = (my_memory_mgr*)&cinfo->mem;
   JSAMPROW workspace;
   //JDIMENSION rowsperchunk;

   // Calculate max # of rows allowed in one allocation chunk
   /*
   long ltemp = (MAX_ALLOC_CHUNK-SIZEOF(mem_pool_hdr)) / ((long) samplesperrow * SIZEOF(JSAMPLE));
   if (ltemp <= 0)
      return NULL;//E RREXIT(cinfo, JERR_WIDTH_OVERFLOW);
   if (ltemp < (long) numrows)
      rowsperchunk = (JDIMENSION) ltemp;
   else
   */
   dword rowsperchunk = numrows;
   mem->last_rowsperchunk = rowsperchunk;

   /* Get space for row pointers (small object) */
   JSAMPARRAY result = (JSAMPARRAY)mem->AllocSmall((size_t) (numrows * SIZEOF(JSAMPROW)));
   if(!result)
      return NULL;

   /* Get the rows themselves (large objects) */
   dword currow = 0;
   while (currow < numrows) {
      rowsperchunk = Min(rowsperchunk, numrows - currow);
      workspace = (JSAMPROW)mem->AllocSmall((size_t) ((size_t) rowsperchunk * (size_t) samplesperrow*SIZEOF(JSAMPLE)));
      if(!workspace)
         return NULL;
      for(dword i = rowsperchunk; i > 0; i--) {
         result[currow++] = workspace;
         workspace += samplesperrow;
      }
   }
   return result;
}

//----------------------------
/*
 * Creation of 2-D coefficient-block arrays.
 * This is essentially the same as the code for sample arrays, above.
 */
static JBLOCKARRAY alloc_barray(jpeg_decompress_struct* cinfo, JDIMENSION blocksperrow, JDIMENSION numrows){
   /* Allocate a 2-D coefficient-block array */
   my_memory_mgr* mem = (my_memory_mgr*)&cinfo->mem;
   JBLOCKROW workspace;
   //JDIMENSION rowsperchunk;
   /*
                              //Calculate max # of rows allowed in one allocation chunk
   long ltemp = (MAX_ALLOC_CHUNK-SIZEOF(mem_pool_hdr)) / (long(blocksperrow) * SIZEOF(JBLOCK));
   if (ltemp <= 0)
      return NULL;//E RREXIT(cinfo, JERR_WIDTH_OVERFLOW);
   if(ltemp < (long) numrows)
      rowsperchunk = (JDIMENSION) ltemp;
   else
   */
   JDIMENSION rowsperchunk = numrows;
   mem->last_rowsperchunk = rowsperchunk;

   /* Get space for row pointers (small object) */
   JBLOCKARRAY result = (JBLOCKARRAY)mem->AllocSmall((size_t) (numrows * SIZEOF(JBLOCKROW)));
   if(!result)
      return NULL;

   /* Get the rows themselves (large objects) */
   JDIMENSION currow = 0;
   while (currow < numrows) {
      rowsperchunk = Min(rowsperchunk, numrows - currow);
      workspace = (JBLOCKROW)mem->AllocSmall((size_t) ((size_t) rowsperchunk * (size_t) blocksperrow*SIZEOF(JBLOCK)));
      if(!workspace)
         return NULL;
      for(dword i = rowsperchunk; i > 0; i--) {
         result[currow++] = workspace;
         workspace += blocksperrow;
      }
   }
   return result;
}


/*
 * About virtual array management:
 *
 * The above "normal" array routines are only used to allocate strip buffers
 * (as wide as the image, but just a few rows high).  Full-image-sized buffers
 * are handled as "virtual" arrays.  The array is still accessed a strip at a
 * time, but the memory manager must save the whole array for repeated
 * accesses.  The intended implementation is that there is a strip buffer in
 * memory (as high as is possible given the desired memory limit), plus a
 * backing file that holds the rest of the array.
 *
 * The request_virt_array routines are told the total size of the image and
 * the maximum number of rows that will be accessed at once.  The in-memory
 * buffer must be at least as large as the maxaccess value.
 *
 * The request routines create control blocks but not the in-memory buffers.
 * That is postponed until realize_virt_arrays is called.  At that time the
 * total amount of space needed is known (approximately, anyway), so free
 * memory can be divided up fairly.
 *
 * The access_virt_array routines are responsible for making a specific strip
 * area accessible (after reading or writing the backing file, if necessary).
 * Note that the access routines are told whether the caller intends to modify
 * the accessed strip; during a read-only pass this saves having to rewrite
 * data to disk.  The access routines are also responsible for pre-zeroing
 * any newly accessed rows, if pre-zeroing was requested.
 *
 * In current usage, the access requests are usually for nonoverlapping
 * strips; that is, successive access start_row numbers differ by exactly
 * num_rows = maxaccess.  This means we can get good performance with simple
 * buffer dump/reload logic, by making the in-memory buffer be a multiple
 * of the access height; then there will never be accesses across bufferload
 * boundaries.  The code will still work with overlapping access requests,
 * but it doesn't handle bufferload overlaps very efficiently.
 */

/* Request a virtual 2-D coefficient-block array */
static jvirt_barray_ptr request_virt_barray (jpeg_decompress_struct* cinfo, bool pre_zero, JDIMENSION blocksperrow, JDIMENSION numrows, JDIMENSION maxaccess){
   my_memory_mgr* mem = (my_memory_mgr*)&cinfo->mem;

   /* get control block */
   jvirt_barray_ptr result = (jvirt_barray_ptr) mem->AllocSmall(SIZEOF(struct jvirt_barray_control));
   if(!result)
      return NULL;

   result->_mem_buffer = NULL;  /* marks array not yet realized */
   result->rows_in_array = numrows;
   result->blocksperrow = blocksperrow;
   result->maxaccess = maxaccess;
   result->pre_zero = pre_zero;
   //result->b_s_open = false;   /* no associated backing-store object */
   result->next = mem->virt_barray_list; /* add to list of virtual arrays */
   mem->virt_barray_list = result;

   return result;
}

//----------------------------

/* Allocate the in-memory buffers for any unrealized virtual arrays */
static bool realize_virt_arrays (jpeg_decompress_struct* cinfo){
   my_memory_mgr* mem = (my_memory_mgr*)&cinfo->mem;
   long space_per_minheight, maximum_space;//, avail_mem;
   long minheights, max_minheights;
   jvirt_sarray_ptr sptr;
   jvirt_barray_ptr bptr;
   
   /* Compute the minimum space needed (maxaccess rows in each buffer)
   * and the maximum space needed (full image height in each buffer).
   * These may be of use to the system-dependent jpeg_mem_available routine.
   */
   space_per_minheight = 0;
   maximum_space = 0;
   for(sptr = mem->virt_sarray_list; sptr != NULL; sptr = (jvirt_sarray_ptr)sptr->next) {
      if (sptr->mem_buffer == NULL) { /* if not realized yet */
         space_per_minheight += (long) sptr->maxaccess * (long) sptr->samplesperrow * SIZEOF(JSAMPLE);
         maximum_space += (long) sptr->rows_in_array * (long) sptr->samplesperrow * SIZEOF(JSAMPLE);
      }
   }
   for(bptr = mem->virt_barray_list; bptr != NULL; bptr = (jvirt_barray_ptr)bptr->next) {
      if (bptr->_mem_buffer == NULL) { /* if not realized yet */
         space_per_minheight += (long) bptr->maxaccess * (long) bptr->blocksperrow * SIZEOF(JBLOCK);
         maximum_space += (long) bptr->rows_in_array * (long) bptr->blocksperrow * SIZEOF(JBLOCK);
      }
   }
   
   if (space_per_minheight <= 0)
      return true;       /* no unrealized arrays, no work */
   
   /* Determine amount of memory to actually use; this is system-dependent. */
   //avail_mem = jpeg_mem_available(cinfo, space_per_minheight, maximum_space, mem.total_space_allocated);
   
   /* If the maximum space needed is available, make all the buffers full
   * height; otherwise parcel it out with the same number of minheights
   * in each buffer.
   */
   //if (avail_mem >= maximum_space)
   max_minheights = 1000000000L;
   /*
   else {
   max_minheights = avail_mem / space_per_minheight;
   //If there doesn't seem to be enough space, try to get the minimum
   //anyway.  This allows a "stub" implementation of jpeg_mem_available().
   if (max_minheights <= 0)
   max_minheights = 1;
   }
   */
   
   /* Allocate the in-memory buffers and initialize backing store as needed. */
   
   for (sptr = mem->virt_sarray_list; sptr != NULL; sptr = (jvirt_sarray_ptr)sptr->next) {
      if (sptr->mem_buffer == NULL) { /* if not realized yet */
         minheights = ((long) sptr->rows_in_array - 1L) / sptr->maxaccess + 1L;
         //if (minheights <= max_minheights) {
            /* This buffer fits in memory */
            sptr->rows_in_mem = sptr->rows_in_array;
            /*
         } else {
            sptr->rows_in_mem = (JDIMENSION) (max_minheights * sptr->maxaccess);
            jpeg_open_backing_store(cinfo, & sptr->b_s_info, (long) sptr->rows_in_array * (long) sptr->samplesperrow * (long) SIZEOF(JSAMPLE));
            sptr->b_s_open = true;
         }
         */
         sptr->mem_buffer = AllocSarray(cinfo, sptr->samplesperrow, sptr->rows_in_mem);
         if(!sptr->mem_buffer)
            return false;
         sptr->rowsperchunk = mem->last_rowsperchunk;
         sptr->cur_start_row = 0;
         sptr->first_undef_row = 0;
         sptr->dirty = false;
      }
   }
   
   for (bptr = mem->virt_barray_list; bptr != NULL; bptr = (jvirt_barray_ptr)bptr->next) {
      if (bptr->_mem_buffer == NULL) { /* if not realized yet */
         minheights = ((long) bptr->rows_in_array - 1L) / bptr->maxaccess + 1L;
         //if (minheights <= max_minheights) {
            /* This buffer fits in memory */
            bptr->rows_in_mem = bptr->rows_in_array;
            /*
         } else {
            bptr->rows_in_mem = (JDIMENSION) (max_minheights * bptr->maxaccess);
            jpeg_open_backing_store(cinfo, & bptr->b_s_info,
               (long) bptr->rows_in_array *
               (long) bptr->blocksperrow *
               (long) SIZEOF(JBLOCK));
            bptr->b_s_open = true;
         }
         */
         bptr->_mem_buffer = alloc_barray(cinfo, bptr->blocksperrow, bptr->rows_in_mem);
         if(!bptr->_mem_buffer)
            return false;
         bptr->rowsperchunk = mem->last_rowsperchunk;
         bptr->cur_start_row = 0;
         bptr->first_undef_row = 0;
         bptr->dirty = false;
      }
   }
   return true;
}


static void
do_sarray_io (jpeg_decompress_struct* cinfo, jvirt_sarray_ptr ptr, bool writing)
/* Do backing store read or write of a virtual sample array */
{
   assert(0);

  long bytesperrow, file_offset, byte_count, rows, thisrow, i;

  bytesperrow = (long) ptr->samplesperrow * SIZEOF(JSAMPLE);
  file_offset = ptr->cur_start_row * bytesperrow;
  /* Loop to read or write each allocation chunk in mem_buffer */
  for (i = 0; i < (long) ptr->rows_in_mem; i += ptr->rowsperchunk) {
    /* One chunk, but check for short chunk at end of buffer */
    rows = Min((long) ptr->rowsperchunk, (long) ptr->rows_in_mem - i);
    /* Transfer no more than is currently defined */
    thisrow = (long) ptr->cur_start_row + i;
    rows = Min(rows, (long) ptr->first_undef_row - thisrow);
    /* Transfer no more than fits in file */
    rows = Min(rows, (long) ptr->rows_in_array - thisrow);
    if (rows <= 0)      /* this chunk might be past end of file! */
      break;
    byte_count = rows * bytesperrow;
    /*
    if (writing)
      (*ptr->b_s_info.write_backing_store) (cinfo, & ptr->b_s_info, (void *) ptr->mem_buffer[i], file_offset, byte_count);
    else
      (*ptr->b_s_info.read_backing_store) (cinfo, & ptr->b_s_info, (void *) ptr->mem_buffer[i], file_offset, byte_count);
      */
    file_offset += byte_count;
  }
}

//----------------------------

static void
do_barray_io (jpeg_decompress_struct* cinfo, jvirt_barray_ptr ptr, bool writing)
/* Do backing store read or write of a virtual coefficient-block array */
{
  long bytesperrow, file_offset, byte_count, rows, thisrow, i;

  assert(0);

  bytesperrow = (long) ptr->blocksperrow * SIZEOF(JBLOCK);
  file_offset = ptr->cur_start_row * bytesperrow;
  /* Loop to read or write each allocation chunk in mem_buffer */
  for (i = 0; i < (long) ptr->rows_in_mem; i += ptr->rowsperchunk) {
    /* One chunk, but check for short chunk at end of buffer */
    rows = Min((long) ptr->rowsperchunk, (long) ptr->rows_in_mem - i);
    /* Transfer no more than is currently defined */
    thisrow = (long) ptr->cur_start_row + i;
    rows = Min(rows, (long) ptr->first_undef_row - thisrow);
    /* Transfer no more than fits in file */
    rows = Min(rows, (long) ptr->rows_in_array - thisrow);
    if (rows <= 0)      /* this chunk might be past end of file! */
      break;
    byte_count = rows * bytesperrow;
    /*
    if (writing)
      (*ptr->b_s_info.write_backing_store) (cinfo, & ptr->b_s_info,
                   (void *) ptr->mem_buffer[i],
                   file_offset, byte_count);
    else
      (*ptr->b_s_info.read_backing_store) (cinfo, & ptr->b_s_info,
                  (void *) ptr->mem_buffer[i],
                  file_offset, byte_count);
                  */
    file_offset += byte_count;
  }
}

//----------------------------

static JSAMPARRAY access_virt_sarray (jpeg_decompress_struct* cinfo, jvirt_sarray_ptr ptr, JDIMENSION start_row, JDIMENSION num_rows, bool writable)
/* Access the part of a virtual sample array starting at start_row */
/* and extending for num_rows rows.  writable is true if  */
/* caller intends to modify the accessed area. */
{
   JDIMENSION end_row = start_row + num_rows;
   JDIMENSION undef_row;

   /* debugging check */
   if (end_row > ptr->rows_in_array || num_rows > ptr->maxaccess ||
      ptr->mem_buffer == NULL)
      return NULL;//E RREXIT(cinfo, JERR_BAD_VIRTUAL_ACCESS);

   /* Make the desired part of the virtual array accessible */
   if (start_row < ptr->cur_start_row ||
      end_row > ptr->cur_start_row+ptr->rows_in_mem) {
         //if (! ptr->b_s_open)
         //E RREXIT(cinfo, JERR_VIRTUAL_BUG);
         /* Flush old buffer contents if necessary */
         if (ptr->dirty) {
            do_sarray_io(cinfo, ptr, true);
            ptr->dirty = false;
         }
         /* Decide what part of virtual array to access.
         * Algorithm: if target address > current window, assume forward scan,
         * load starting at target address.  If target address < current window,
         * assume backward scan, load so that target area is top of window.
         * Note that when switching from forward write to forward read, will have
         * start_row = 0, so the limiting case applies and we load from 0 anyway.
         */
         if (start_row > ptr->cur_start_row) {
            ptr->cur_start_row = start_row;
         } else {
            /* use long arithmetic here to avoid overflow & unsigned problems */
            long ltemp;

            ltemp = (long) end_row - (long) ptr->rows_in_mem;
            if (ltemp < 0)
               ltemp = 0;     /* don't fall off front end of file */
            ptr->cur_start_row = (JDIMENSION) ltemp;
         }
         /* Read in the selected part of the array.
         * During the initial write pass, we will do no actual read
         * because the selected part is all undefined.
         */
         do_sarray_io(cinfo, ptr, false);
   }
   /* Ensure the accessed part of the array is defined; prezero if needed.
   * To improve locality of access, we only prezero the part of the array
   * that the caller is about to access, not the entire in-memory array.
   */
   if (ptr->first_undef_row < end_row) {
      if (ptr->first_undef_row < start_row) {
         if (writable)     /* writer skipped over a section of array */
            return NULL;//E RREXIT(cinfo, JERR_BAD_VIRTUAL_ACCESS);
         undef_row = start_row;  /* but reader is allowed to read ahead */
      } else {
         undef_row = ptr->first_undef_row;
      }
      if (writable)
         ptr->first_undef_row = end_row;
      if (ptr->pre_zero) {
         size_t bytesperrow = (size_t) ptr->samplesperrow * SIZEOF(JSAMPLE);
         undef_row -= ptr->cur_start_row; /* make indexes relative to buffer */
         end_row -= ptr->cur_start_row;
         while (undef_row < end_row) {
            MemSet(ptr->mem_buffer[undef_row], 0, bytesperrow);
            undef_row++;
         }
      } else {
         if (! writable)      /* reader looking at undefined data */
            return NULL;//E RREXIT(cinfo, JERR_BAD_VIRTUAL_ACCESS);
      }
   }
   /* Flag the buffer dirty if caller will write in it */
   if (writable)
      ptr->dirty = true;
   /* Return address of proper part of the buffer */
   return ptr->mem_buffer + (start_row - ptr->cur_start_row);
}

//----------------------------
/* Access the part of a virtual block array starting at start_row */
/* and extending for num_rows rows.  writable is true if  */
/* caller intends to modify the accessed area. */
static JBLOCKARRAY access_virt_barray (jpeg_decompress_struct* cinfo, jvirt_barray_ptr ptr, JDIMENSION start_row, JDIMENSION num_rows, bool writable){
   JDIMENSION end_row = start_row + num_rows;
   JDIMENSION undef_row;
   
   /* debugging check */
   if (end_row > ptr->rows_in_array || num_rows > ptr->maxaccess ||
      ptr->_mem_buffer == NULL)
      return NULL;//E RREXIT(cinfo, JERR_BAD_VIRTUAL_ACCESS);
   
   /* Make the desired part of the virtual array accessible */
   if (start_row < ptr->cur_start_row || end_row > ptr->cur_start_row+ptr->rows_in_mem) {
      //if (! ptr->b_s_open)
         //E RREXIT(cinfo, JERR_VIRTUAL_BUG);
      /* Flush old buffer contents if necessary */
      if (ptr->dirty) {
         do_barray_io(cinfo, ptr, true);
         ptr->dirty = false;
      }
      /* Decide what part of virtual array to access.
      * Algorithm: if target address > current window, assume forward scan,
      * load starting at target address.  If target address < current window,
      * assume backward scan, load so that target area is top of window.
      * Note that when switching from forward write to forward read, will have
      * start_row = 0, so the limiting case applies and we load from 0 anyway.
      */
      if (start_row > ptr->cur_start_row) {
         ptr->cur_start_row = start_row;
      } else {
         /* use long arithmetic here to avoid overflow & unsigned problems */
         long ltemp;
         
         ltemp = (long) end_row - (long) ptr->rows_in_mem;
         if (ltemp < 0)
            ltemp = 0;     /* don't fall off front end of file */
         ptr->cur_start_row = (JDIMENSION) ltemp;
      }
      /* Read in the selected part of the array.
      * During the initial write pass, we will do no actual read
      * because the selected part is all undefined.
      */
      do_barray_io(cinfo, ptr, false);
   }
   /* Ensure the accessed part of the array is defined; prezero if needed.
   * To improve locality of access, we only prezero the part of the array
   * that the caller is about to access, not the entire in-memory array.
   */
   if (ptr->first_undef_row < end_row) {
      if (ptr->first_undef_row < start_row) {
         if (writable)     /* writer skipped over a section of array */
            return NULL; //E RREXIT(cinfo, JERR_BAD_VIRTUAL_ACCESS);
         undef_row = start_row;  /* but reader is allowed to read ahead */
      } else {
         undef_row = ptr->first_undef_row;
      }
      if (writable)
         ptr->first_undef_row = end_row;
      if (ptr->pre_zero) {
         size_t bytesperrow = (size_t) ptr->blocksperrow * SIZEOF(JBLOCK);
         undef_row -= ptr->cur_start_row; /* make indexes relative to buffer */
         end_row -= ptr->cur_start_row;
         while (undef_row < end_row) {
            if(!ptr->_mem_buffer[undef_row])
               return NULL;
            MemSet(ptr->_mem_buffer[undef_row], 0, bytesperrow);
            undef_row++;
         }
      } else {
         if (! writable)      /* reader looking at undefined data */
            return NULL; //E RREXIT(cinfo, JERR_BAD_VIRTUAL_ACCESS);
      }
   }
   /* Flag the buffer dirty if caller will write in it */
   if (writable)
      ptr->dirty = true;
   /* Return address of proper part of the buffer */
   return ptr->_mem_buffer + (start_row - ptr->cur_start_row);
}

//----------------------------

void jpeg_decompress_struct::jinit_memory_mgr(){

   /* OK, fill in the method pointers */
   mem.AllocSarray = AllocSarray;
   mem.alloc_barray = alloc_barray;
   mem.request_virt_barray = request_virt_barray;
   mem.realize_virt_arrays = realize_virt_arrays;
   mem.access_virt_sarray = access_virt_sarray;
   mem.access_virt_barray = access_virt_barray;


   mem.small_list = NULL;
   mem.virt_sarray_list = NULL;
   mem.virt_barray_list = NULL;
}

//----------------------------

#ifndef SEEK_SET     /* pre-ANSI systems may not define this; */
#define SEEK_SET  0     /* if not, assume 0 is correct */
#endif

/*
 * Arithmetic utilities
 */

static long jdiv_round_up (long a, long b)
/* Compute a/b rounded up to next integer, ie, ceil(a/b) */
/* Assumes a >= 0, b > 0 */
{
  return (a + b - 1L) / b;
}

//----------------------------

static long jround_up (long a, long b)
/* Compute a rounded up to next multiple of b, ie, ceil(a/b)*b */
/* Assumes a >= 0, b > 0 */
{
  a += b - 1L;
  return a - (a % b);
}

//----------------------------

static void jcopy_sample_rows (JSAMPARRAY input_array, int source_row, JSAMPARRAY output_array, int dest_row, int num_rows, JDIMENSION num_cols)
/* Copy some rows of samples from one place to another.
 * num_rows rows are copied from input_array[source_row++]
 * to output_array[dest_row++]; these areas may overlap for duplication.
 * The source and destination arrays must be at least as wide as num_cols.
 */
{
  JSAMPROW inptr, outptr;
#ifdef MemCpy
  size_t count = (size_t) (num_cols * SIZEOF(JSAMPLE));
#else
  JDIMENSION count;
#endif
  int row;

  input_array += source_row;
  output_array += dest_row;

  for (row = num_rows; row > 0; row--) {
    inptr = *input_array++;
    outptr = *output_array++;
#ifdef MemCpy
    MemCpy(outptr, inptr, count);
#else
    for (count = num_cols; count > 0; count--)
      *outptr++ = *inptr++;   /* needn't bother with GETJSAMPLE() here */
#endif
  }
}
//----------------------------

}

class C_loader_jpg: public C_image_loader{

   jpglib::S_file_struct file_info;
   jpglib::jpeg_decompress_struct cinfo;
   bool in_decompress;

//----------------------------

   virtual bool GetImageInfo(dword header, int limit_size_x, int limit_size_y, C_fixed limit_ratio){

      if((header&0x00ffffff) != 0x00ffd8ff)
         return false;

      if(!jpglib::jpeg_stdio_src(&cinfo, (int)&file_info))
         return false;
      int ret = jpglib::jpeg_read_header(&cinfo);
      if(ret!=JPEG_HEADER_OK)
         return false;
#if 1
                              //compute idct scaling
      if(limit_size_x || limit_size_y){
         int sx = cinfo.image_width;
         int sy = cinfo.image_height;
         while((limit_size_x && sx>=limit_size_x*2) &&
            (limit_size_y && sy>=limit_size_y*2)){

            cinfo.scale_denom <<= 1;
            sx >>= 1;
            sy >>= 1;
         }
      }
      if(limit_ratio!=C_fixed::One()){
         dword denom = 1;
         while(limit_ratio<=C_fixed::Half()){
            denom <<= 1;
            limit_ratio <<= 1;
         }
         cinfo.scale_denom = Max(cinfo.scale_denom, denom);
      }
#endif
      jpglib::jpeg_calc_output_dimensions(&cinfo);

      size_original_x = cinfo.image_width;
      size_original_y = cinfo.image_height;
      size_x = cinfo.output_width;
      size_y = cinfo.output_height;
      bpp = cinfo.num_components;

      return true;
   }

//----------------------------

   virtual bool InitDecoder(){

                              //start decompressor
      if(!cinfo.master_selection())
         return false;

      if(!jpglib::jpegStartDecompress(&cinfo))
         return false;
      in_decompress = true;
      return true;
   }

//----------------------------

   virtual bool ReadOneOrMoreLines(C_vector<t_horizontal_line*> &lines){

      t_horizontal_line *line = new(true) t_horizontal_line;
      line->Resize(size_x);
      lines.push_back(line);
                              //read file header, set default decompression parameters
      //jpeg_read_header(&cinfo, true);
                              //process data
      S_rgb *dst = line->Begin();
      byte *bp = (byte*)dst;

      PROF_S(PROF_SCANLINES);
      if(jpglib::jpeg_read_scanlines(&cinfo, &bp, 1)!=1)
         return false;
      PROF_E(PROF_SCANLINES);

                              //convert grayscale to RGB
      if(bpp==1){
         bp += size_x;
         dst += size_x;
         for(int i=size_x; i--; ){
            --dst;
            byte c = *--bp;
            dst->r = c;
            dst->g = c;
            dst->b = c;
         }
      }
      return true;
   }
public:

   C_loader_jpg(C_file *ck):
      C_image_loader(ck),
      file_info(ck),
      in_decompress(false)
   {
      PROF_S(PROF_ALL);
   }
   ~C_loader_jpg(){
      if(in_decompress)
         jpglib::jpeg_abort(&cinfo);
      jpglib::jpeg_destroy_decompress(&cinfo);
      PROF_E(PROF_ALL);
   }
};

C_image_loader *CreateImgLoaderJPG(C_file *ck){
   return new(true) C_loader_jpg(ck);
}

//----------------------------
//----------------------------
