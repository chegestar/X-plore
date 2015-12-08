#ifdef __SYMBIAN32__
#include <fbs.h>
#include <s32file.h>
#include <eikenv.h>
#endif

#include "..\Main.h"
#include "Main_Explorer.h"

#ifdef _MSC_VER
#pragma warning(disable: 4146 4244 4018 4389)
#endif

//#define SUPPORT_VER_15

//----------------------------

class C_EncodeFileName{
private:
   void AddFlags(int Value);

   //byte *EncName;
   byte Flags;
   int FlagBits;
   int FlagsPos;
   int DestSize;
public:
   C_EncodeFileName(){
      Flags=0;
      FlagBits=0;
      FlagsPos=0;
      DestSize=0;
   }
   //int Encode(char *Name,wchar *NameW,byte *EncName);
   void Decode(char *Name,byte *_EncName,int EncSize,wchar *NameW,int MaxDecSize);
};

//----------------------------

void C_EncodeFileName::Decode(char *Name, byte *_EncName, int EncSize, wchar *NameW, int MaxDecSize){

   int EncPos = 0, DecPos = 0;
   byte HighByte = _EncName[EncPos++];
   while(EncPos<EncSize && DecPos<MaxDecSize){
      if(FlagBits==0){
         Flags = _EncName[EncPos++];
         FlagBits = 8;
      }
      switch(Flags>>6){
      case 0:
         NameW[DecPos++] = _EncName[EncPos++];
         break;
      case 1:
         NameW[DecPos++] = _EncName[EncPos++] + (HighByte<<8);
         break;
      case 2:
         NameW[DecPos++] = _EncName[EncPos] + (_EncName[EncPos+1]<<8);
         EncPos+=2;
         break;
      case 3:
         {
            int Length = _EncName[EncPos++];
            if(Length & 0x80){
               byte Correction = _EncName[EncPos++];
               for(Length=(Length&0x7f)+2; Length>0 && DecPos<MaxDecSize; Length--,DecPos++)
                  NameW[DecPos] = ((Name[DecPos]+Correction)&0xff)+(HighByte<<8);
            }else{
               for(Length+=2; Length>0 && DecPos<MaxDecSize; Length--,DecPos++)
                  NameW[DecPos] = Name[DecPos];
            }
         }
         break;
      }
      Flags <<= 2;
      FlagBits -= 2;
   }
   NameW[DecPos<MaxDecSize ? DecPos : MaxDecSize-1] = 0;
}

//----------------------------

class C_rar_package: public C_zip_package{
   virtual E_TYPE GetArchiveType() const{
      return TYPE_RAR;
   }
   virtual const wchar *GetFileName() const{ return filename_full; }
public:

   struct S_file_entry{
      dword local_offset;     //offset info local header
      dword size_uncompressed;
      dword size_compressed;
      byte version;
      bool stored;
      word flags;
      wchar name[1];           //variable-length name (1st char is length)

      inline dword GetStructSize() const{
         return (((byte*)name - (byte*)this) + (((name[0]+2)*sizeof(wchar))) + 3) & -4;
      }
      inline bool CheckName(const wchar *in_name, int len) const{
         if(len!=name[0])
            return false;
         const wchar *n = name+1;
         while(len--){
            if(ToLower(*n++)!=*in_name++)
               return false;
         }
         return true;
      }
   };
   void *file_info;           //pointer to array of variable-length S_file_entry structures
   

   Cstr_w filename_full;      //archive filename
   C_file *arch_file;
   bool take_arch_ownership;

   dword num_files;

   C_rar_package():
      arch_file(NULL),
      file_info(NULL),
      num_files(0)
   {}
   C_rar_package(C_file *fp, bool take_ownership):
      arch_file(fp),
      take_arch_ownership(take_ownership),
      file_info(NULL),
      num_files(0)
   {}
   ~C_rar_package(){
      delete[] (byte*)file_info;
      if(arch_file && take_arch_ownership)
         delete arch_file;
   }

   bool Construct(const wchar *fname, C_file &ck);

//----------------------------

   virtual dword NumFiles() const{
      return num_files;
   }

//----------------------------

   virtual void *GetFirstEntry(const wchar *&name, dword &length) const{
      S_file_entry *fe = (S_file_entry*)file_info;
      if(fe){
         name = fe->name+1;
         length = fe->size_uncompressed;
      }
      return fe;
   }

//----------------------------

   virtual void *GetNextEntry(void *handle, const wchar *&name, dword &length) const{
      S_file_entry *fe = (S_file_entry*)handle;
      fe = (S_file_entry*)((byte*)fe + fe->GetStructSize());
      if(!fe->local_offset)
         return NULL;
      name = fe->name+1;
      length = fe->size_uncompressed;
      return fe;
   }

//----------------------------

   virtual bool GetFileWriteTime(const wchar *filename, S_date_time &td) const{ return false; }

   virtual C_file *OpenFile(const wchar *filename) const{
      C_file_rar *fp = new(true) C_file_rar;
      if(fp->Open(filename, this))
         return fp;
      delete fp;
      return NULL;
   }
};

//----------------------------

struct S_file_header{
   word header_crc;

                        //HEAD_TYPE=0x72          marker block
                        //HEAD_TYPE=0x73          archive header
                        //HEAD_TYPE=0x74          file header
                        //HEAD_TYPE=0x75          comment header
                        //HEAD_TYPE=0x76          extra information
                        //HEAD_TYPE=0x77          subblock
                        //HEAD_TYPE=0x78          recovery record
   byte block_type;

                        //0x01 - file continued from previous volume
                        //0x02 - file continued in next volume
                        //0x04 - file encrypted with password
                        //0x08 - file comment present
                        //0x10 - information from previous files is used (solid flag)
                        // (for RAR 2.0 and later)
   enum{
      LHD_SPLIT_BEFORE = 1,
      LHD_SPLIT_AFTER = 2,
      LHD_PASSWORD = 4,
      LHD_COMMENT = 8,
      LHD_SOLID = 0x10,
      LHD_LARGE = 0x0100,
      LHD_UNICODE = 0x0200,
      LHD_SALT = 0x0400,
      LHD_VERSION = 0x0800,
      LHD_EXTTIME = 0x1000,
      LHD_EXTFLAGS = 0x2000,
   };
   word block_flags;

                        //file header full size including file name and comments
   word header_size;
   dword file_len_packed;
   dword file_len_unpacked;

                        //Operating system used for archiving
                        //0 - MS DOS
                        //1 - OS/2
                        //2 - Win32
                        //3 - Unix
   byte host_os;

   dword file_crc;

                        //file date and time in MS-DOS format
   dword file_time;        

                        //10 * Major version + minor version
   byte rar_version;

                        //Packing method encoding:
                        //0x30 - storing
                        //0x31 - fastest compression
                        //0x32 - fast compression
                        //0x33 - normal compression
                        //0x34 - good compression
                        //0x35 - best compression
   byte pack_method;

   word fname_len;

   dword file_attributes;

   bool Read(C_file &ck){
      if(!ck.Read(&header_crc, 2)) return false;
      if(!ck.Read(&block_type, 1)) return false;
      if(!ck.Read(&block_flags, 2)) return false;
      if(!ck.Read(&header_size, 2)) return false;
      if(!ck.Read(&file_len_packed, 4)) return false;
      if(!ck.Read(&file_len_unpacked, 4)) return false;
      if(!ck.Read(&host_os, 1)) return false;
      if(!ck.Read(&file_crc, 4)) return false;
      if(!ck.Read(&file_time, 4)) return false;
      if(!ck.Read(&rar_version, 1)) return false;
      if(!ck.Read(&pack_method, 1)) return false;
      if(!ck.Read(&fname_len, 2)) return false;
      if(!ck.Read(&file_attributes, 4)) return false;
      return true;
   }
};

//----------------------------

bool C_rar_package::Construct(const wchar *fname, C_file &ck){

                              //read file header
   dword id;
   if(!ck.Read(&id, sizeof(id)))
      return false;
   if(id != 0x21726152)
      return false;

   C_vector<byte> buf;
   ck.Seek(20);
   while(true){
      dword fpos = ck.Tell();
      S_file_header hdr;
      if(!hdr.Read(ck))
         break;
      if(hdr.rar_version>36)
         break;
      char filename[256];
      if(hdr.fname_len < sizeof(filename)-1){
         if(hdr.file_len_packed){
            if(!ck.Read(&filename, hdr.fname_len))
               break;
            filename[hdr.fname_len] = 0;

            dword alloc_len = OffsetOf(S_file_entry, name);
            Cstr_w fn_w;
            if(hdr.block_flags&hdr.LHD_UNICODE){
               int len = StrLen(filename);
               if(len==hdr.fname_len){
                  assert(0);
                  ck.Seek(fpos + hdr.header_size + hdr.file_len_packed);
                  continue;
               }else{
                  C_EncodeFileName dec;
                  ++len;
                  C_buffer<wchar> buf1;
                  buf1.Resize(256);
                  dec.Decode(filename, (byte*)filename+len, hdr.fname_len-len, buf1.Begin(), buf1.Size());
                  fn_w = buf1.Begin();
                  alloc_len += (fn_w.Length() + 2)*sizeof(wchar);
               }
            }else{
               alloc_len += (hdr.fname_len + 2)*sizeof(wchar);
            }
            alloc_len = (alloc_len+3) & -4;

            buf.resize(buf.size()+alloc_len);
            S_file_entry *fe = (S_file_entry*)((byte*)buf.end() - alloc_len);
            fe->local_offset = fpos + hdr.header_size;
            if(hdr.block_flags&hdr.LHD_UNICODE){
               fe->name[0] = fn_w.Length();
               StrCpy(fe->name+1, fn_w);
            }else{
               fe->name[0] = hdr.fname_len;
               for(int i=0; ; i++){
                  fe->name[i+1] = filename[i];
                  if(!filename[i])
                     break;
               }
            }
            fe->size_compressed = hdr.file_len_packed;
            fe->size_uncompressed = hdr.file_len_unpacked;
            fe->stored = (hdr.pack_method==0x30);
            fe->version = hdr.rar_version;
            fe->flags = hdr.block_flags;
            ++num_files;
         }
         ck.Seek(fpos + hdr.header_size + hdr.file_len_packed);
      }
   }
                              //no files?
   if(!buf.size())
      return false;
   for(int i=0; i<4; i++)
      buf.push_back(0);

   file_info = new(true) byte[buf.size()];
   MemCpy(file_info, buf.begin(), buf.size());

   if(fname)
      C_file::GetFullPath(fname, filename_full);
   return true;
}

//----------------------------
//----------------------------
#define UNIT_SIZE Max(int(sizeof(C_unpack::C_model_ppm::S_ppm_context)), int(sizeof(S_rar_mem_blk)))

//----------------------------

class C_BitInput{
public:
   enum BufferSize{ MAX_SIZE=0x800 };
protected:
   int InAddr,InBit;
public:
   byte InBuf[MAX_SIZE];

   void InitBitInput(){ InAddr=InBit=0; }
   void addbits(int Bits){
      Bits+=InBit;
      InAddr+=Bits>>3;
      InBit=Bits&7;
   }
   dword getbits(){
      dword BitField = (dword)InBuf[InAddr] << 16;
      BitField|=(dword)InBuf[InAddr+1] << 8;
      BitField|=(dword)InBuf[InAddr+2];
      BitField >>= (8-InBit);
      return(BitField & 0xffff);
   }
   inline void faddbits(int Bits){
      addbits(Bits);
   }
   inline dword fgetbits(){
      return(getbits());
   }
   bool Overflow(int IncPtr) {return(InAddr+IncPtr>=MAX_SIZE);}
};

//----------------------------

class C_unpack: public C_BitInput{
   enum{
      MAXWINSIZE = 0x100000,
      MAXWINMASK = (MAXWINSIZE-1),

      LOW_DIST_REP_COUNT = 16,
      NC = 299,
      DC = 60,
      LDC = 17,
      RC = 28,
      HUFF_TABLE_SIZE = (NC+DC+RC+LDC),
      BC = 20,

      NC20 =298,
      DC20 =48,
      RC20 =28,
      BC20 =19,
      MC20 =257,
   };

   enum BLOCK_TYPES{ BLOCK_LZ, BLOCK_PPM };

   struct S_decode{
      dword MaxNum;
      dword DecodeLen[16];
      dword DecodePos[16];
      dword DecodeNum[2];
   };
   struct S_lit_decode{
      dword MaxNum;
      dword DecodeLen[16];
      dword DecodePos[16];
      dword DecodeNum[NC];
   };

   struct S_DistDecode{
      dword MaxNum;
      dword DecodeLen[16];
      dword DecodePos[16];
      dword DecodeNum[DC];
   };

   struct S_LowDistDecode{
      dword MaxNum;
      dword DecodeLen[16];
      dword DecodePos[16];
      dword DecodeNum[LDC];
   };

   struct S_RepDecode{
      dword MaxNum;
      dword DecodeLen[16];
      dword DecodePos[16];
      dword DecodeNum[RC];
   };

   struct S_BitDecode{
      dword MaxNum;
      dword DecodeLen[16];
      dword DecodePos[16];
      dword DecodeNum[BC];
   };


   /***************************** Unpack v 2.0 *********************************/
   struct S_MultDecode{
      dword MaxNum;
      dword DecodeLen[16];
      dword DecodePos[16];
      dword DecodeNum[MC20];
   };

   /***************************** Unpack v 2.0 *********************************/
   struct S_AudioVariables{
      int K1,K2,K3,K4,K5;
      int D1,D2,D3,D4;
      int LastDelta;
      dword Dif[11];
      dword ByteCount;
      int LastChar;
   };

   int DDecode_29[DC];
   byte DBits_29[DC];
public:
   class C_model_ppm{
      enum{
         MAX_O=64                   /* maximum allowed model order */
      };
   public:
      struct S_ppm_context{
         enum{
            INT_BITS=7, PERIOD_BITS=7, TOT_BITS=INT_BITS+PERIOD_BITS,
            INTERVAL=1 << INT_BITS, BIN_SCALE=1 << TOT_BITS, MAX_FREQ=124
         };
         struct S_see2_context{ // SEE-contexts for PPM-contexts with masked symbols
            word Summ;
            byte Shift, Count;
            void init(int InitVal) {
               Summ=InitVal << (Shift=PERIOD_BITS-4);
               Count=4;
            }
            dword getMean(){
               dword RetVal = word(Summ) >> Shift;
               Summ -= RetVal;
               return RetVal+(RetVal == 0);
            }
            void update() {
               if (Shift < PERIOD_BITS && --Count == 0) {
                  Summ += Summ;
                  Count=3 << Shift++;
               }
            }
         };
         struct S_state{
            byte Symbol;
            byte Freq;
            S_ppm_context* Successor;
         };
         struct S_FreqData{
            word SummFreq;
            S_state * Stats;
         };
         dword NumStats;
         union{
            S_FreqData U;
            S_state OneState;
         } u;
         S_ppm_context *Suffix;
         inline void encodeBinSymbol(C_model_ppm *Model,int symbol);  // MaxOrder:
         inline void encodeSymbol1(C_model_ppm *Model,int symbol);    //  ABCD    context
         inline void encodeSymbol2(C_model_ppm *Model,int symbol);    //   BCD    suffix
         inline void decodeBinSymbol(C_model_ppm *Model);  //   BCDE   successor
         inline bool decodeSymbol1(C_model_ppm *Model);    // other orders:
         inline bool decodeSymbol2(C_model_ppm *Model);    //   BCD    context
         inline void update1(C_model_ppm *Model,S_state* p); //    CD    suffix
         inline void update2(C_model_ppm *Model,S_state* p); //   BCDE   successor
         void rescale(C_model_ppm *Model);
         inline S_ppm_context* createChild(C_model_ppm *Model,S_state* pStats,S_state& FirstState);
         inline S_see2_context* makeEscFreq2(C_model_ppm *Model,int Diff);
      };
      class C_range_coder{
      public:
         enum{ TOP=1 << 24, BOT=1 << 15 };

         void InitDecoder(C_unpack *_UnpackRead){
            C_range_coder::UnpackRead=_UnpackRead;

            low=code=0;
            range=dword(-1);
            for (int i=0;i < 4;i++)
               code=(code << 8) | GetChar();
         }
         inline int GetCurrentCount(){ return (code-low)/(range /= SubRange.scale); }
         inline dword GetCurrentShiftCount(dword SHIFT){ return (code-low)/(range >>= SHIFT); }
         inline void S_decode(){
            low += range*SubRange.LowCount;
            range *= SubRange.HighCount-SubRange.LowCount;
         }
         inline void PutChar(dword c);
         inline dword GetChar(){ return(UnpackRead->GetChar()); }

         dword low, code, range;
         struct SUBRANGE{
            dword LowCount, HighCount, scale;
         } SubRange;
         C_unpack *UnpackRead;
      };

   //----------------------------

      class C_sub_allocator{
         enum{
            N1=4, N2=4, N3=4, N4=(128+3-1*N1-2*N2-3*N3)/4,
            N_INDEXES=N1+N2+N3+N4,

            FIXED_UNIT_SIZE=12,
         };

         struct S_rar_mem_blk{
            word Stamp, NU;
            S_rar_mem_blk* next, * prev;
            void insertAt(S_rar_mem_blk* p) {
               next=(prev=p)->next;
               p->next=next->prev=this;
            }
            void remove() {
               prev->next=next;
               next->prev=prev;
            }
         };
         struct S_rar_node{
            S_rar_node* next;
         };

      private:
         void InsertNode(void* p,int indx){
            ((S_rar_node*) p)->next=FreeList[indx].next;
            FreeList[indx].next=(S_rar_node*) p;
         }
         void* RemoveNode(int indx){
            S_rar_node* RetVal=FreeList[indx].next;
            FreeList[indx].next=RetVal->next;
            return RetVal;
         }
         inline dword U2B(int NU){
           return //8*NU+4*NU*/
              UNIT_SIZE*NU;
         }
         void SplitBlock(void* pv,int OldIndx,int NewIndx){
            int i, UDiff=Indx2Units[OldIndx]-Indx2Units[NewIndx];
            byte* p=((byte*) pv)+U2B(Indx2Units[NewIndx]);
            if (Indx2Units[i=Units2Indx[UDiff-1]] != UDiff) 
            {
               InsertNode(p,--i);
               p += U2B(i=Indx2Units[i]);
               UDiff -= i;
            }
            InsertNode(p,Units2Indx[UDiff-1]);
         }
         dword GetUsedMemory();
         inline void GlueFreeBlocks(){
            S_rar_mem_blk s0, * p, * p1;
            int i, k, sz;
            if (LoUnit != HiUnit)
               *LoUnit=0;
            for (i=0, s0.next=s0.prev=&s0;i < N_INDEXES;i++)
               while ( FreeList[i].next )
               {
                  p=(S_rar_mem_blk*)RemoveNode(i);
                  p->insertAt(&s0);
                  p->Stamp=0xFFFF;
                  p->NU=Indx2Units[i];
               }
               for (p=s0.next;p != &s0;p=p->next)
                  while ((p1=MBPtr(p,p->NU))->Stamp == 0xFFFF && int(p->NU)+p1->NU < 0x10000)
                  {
                     p1->remove();
                     p->NU += p1->NU;
                  }
                  while ((p=s0.next) != &s0)
                  {
                     for (p->remove(), sz=p->NU;sz > 128;sz -= 128, p=MBPtr(p,128))
                        InsertNode(p,N_INDEXES-1);
                     if (Indx2Units[i=Units2Indx[sz-1]] != sz)
                     {
                        k=sz-Indx2Units[--i];
                        InsertNode(MBPtr(p,sz-k),k-1);
                     }
                     InsertNode(p,i);
                  }
         }

         void* AllocUnitsRare(int indx);
         inline S_rar_mem_blk* MBPtr(S_rar_mem_blk *BasePtr,int Items){
            return((S_rar_mem_blk*)( ((byte *)(BasePtr))+U2B(Items) ));
         }

         long SubAllocatorSize;
         byte Indx2Units[N_INDEXES], Units2Indx[128], GlueCount;
         byte *HeapStart,*LoUnit, *HiUnit;
         S_rar_node FreeList[N_INDEXES];
      public:
         C_sub_allocator(){
            Clean();
         }
         ~C_sub_allocator() {StopSubAllocator();}
         void Clean(){
            SubAllocatorSize = 0;
         }
         bool StartSubAllocator(int SASize);
         void StopSubAllocator(){
            if(SubAllocatorSize){
               SubAllocatorSize=0;
               delete[] HeapStart;
            }
         }
         void  InitSubAllocator();
         void *AllocContext();
         inline void* AllocUnits(int NU){
            int indx=Units2Indx[NU-1];
            if ( FreeList[indx].next )
               return RemoveNode(indx);
            void* RetVal=LoUnit;
            LoUnit += U2B(Indx2Units[indx]);
            if (LoUnit <= HiUnit)
               return RetVal;
            LoUnit -= U2B(Indx2Units[indx]);
            return AllocUnitsRare(indx);
         }
         inline void* ExpandUnits(void* OldPtr,int OldNU){
            int i0=Units2Indx[OldNU-1], i1=Units2Indx[OldNU-1+1];
            if (i0 == i1)
               return OldPtr;
            void* ptr=AllocUnits(OldNU+1);
            if(ptr){
               MemCpy(ptr,OldPtr,U2B(OldNU));
               InsertNode(OldPtr,i0);
            }
            return ptr;
         }
         inline void* ShrinkUnits(void* OldPtr,int OldNU,int NewNU){
            int i0=Units2Indx[OldNU-1], i1=Units2Indx[NewNU-1];
            if (i0 == i1)
               return OldPtr;
            if ( FreeList[i1].next )
            {
               void* ptr=RemoveNode(i1);
               MemCpy(ptr,OldPtr,U2B(NewNU));
               InsertNode(OldPtr,i0);
               return ptr;
            } 
            else 
            {
               SplitBlock(OldPtr,i0,i1);
               return OldPtr;
            }
         }
         inline void  FreeUnits(void* ptr,int OldNU){ InsertNode(ptr,Units2Indx[OldNU-1]); }
         long GetAllocatedMemory() {return(SubAllocatorSize);};

         byte *pText, *UnitsStart,*HeapEnd,*FakeUnitsStart;
      };
   public:
      S_ppm_context::S_see2_context SEE2Cont[25][16], DummySEE2Cont;

      S_ppm_context *MinContext, *MedContext, *MaxContext;
      int NumMasked, InitEsc, MaxOrder, RunLength, InitRL;
      byte CharMask[256], NS2Indx[256], NS2BSIndx[256], HB2Flag[256];
      byte EscCount, PrevSuccess, HiBitsFlag;
      word BinSumm[128][64];               // binary SEE-contexts

      C_range_coder Coder;

      void RestartModelRare();
      void StartModelRare(int MaxOrder);
      inline S_ppm_context* CreateSuccessors(bool Skip, S_ppm_context::S_state* p1);

      void UpdateModel();
      inline void ClearMask();
   public:
      S_ppm_context::S_state* FoundState;      // found next state transition
      C_sub_allocator SubAlloc;
      int OrderFall;

      C_model_ppm(){
         MinContext=NULL;
         MaxContext=NULL;
         MedContext=NULL;
      }
      bool DecodeInit(C_unpack *UnpackRead,int &EscChar);
      int DecodeChar();
   };

//----------------------------
   enum{
      VM_MEMSIZE = 0x40000,
      VM_GLOBALMEMADDR = 0x3C000,
      VM_GLOBALMEMSIZE = 0x2000,
      VM_FIXEDGLOBALSIZE = 64,
   };

   struct VM_PreparedOperand{
      enum VM_OpType{ VM_OPREG, VM_OPINT, VM_OPREGMEM, VM_OPNONE };
      VM_OpType Type;
      dword Data;
      dword Base;
      dword *Addr;
   };
   enum VM_Commands{
      VM_MOV,  VM_CMP,  VM_ADD,  VM_SUB,  VM_JZ,   VM_JNZ,  VM_INC,  VM_DEC,
      VM_JMP,  VM_XOR,  VM_AND,  VM_OR,   VM_TEST, VM_JS,   VM_JNS,  VM_JB,
      VM_JBE,  VM_JA,   VM_JAE,  VM_PUSH, VM_POP,  VM_CALL, VM_RET,  VM_NOT,
      VM_SHL,  VM_SHR,  VM_SAR,  VM_NEG,  VM_PUSHA,VM_POPA, VM_PUSHF,VM_POPF,
      VM_MOVZX,VM_MOVSX,VM_XCHG, VM_MUL,  VM_DIV,  VM_ADC,  VM_SBB,  VM_PRINT,

   #ifdef VM_OPTIMIZE
      VM_MOVB, VM_MOVD, VM_CMPB, VM_CMPD,

      VM_ADDB, VM_ADDD, VM_SUBB, VM_SUBD, VM_INCB, VM_INCD, VM_DECB, VM_DECD,
      VM_NEGB, VM_NEGD,
   #endif

      VM_STANDARD
   };
   struct VM_PreparedCommand{
      VM_Commands OpCode;
      bool ByteMode;
      VM_PreparedOperand Op1,Op2;
   };

//----------------------------

   struct VM_PreparedProgram{
      VM_PreparedProgram() {AltCmd=NULL;}

      C_vector<VM_PreparedCommand> Cmd;
      VM_PreparedCommand *AltCmd;
      int CmdCount;

      C_vector<byte> GlobalData;
      C_vector<byte> StaticData; // static data contained in DB operators
      dword InitR[7];

      byte *FilteredData;
      dword FilteredDataSize;
   };

//----------------------------

   class C_RarVM: private C_BitInput{
   private:
      dword GetValue(bool ByteMode,dword *Addr);
      void SetValue(bool ByteMode,dword *Addr,dword Value){
         if (ByteMode)
            *(byte *)Addr = Value;
         else
            *(dword*)Addr=Value;
      }
      inline dword* GetOperand(VM_PreparedOperand *CmdOp);
      void DecodeArg(VM_PreparedOperand &Op,bool ByteMode);
      bool ExecuteCode(VM_PreparedCommand *PreparedCode,int CodeSize);
#ifdef VM_STANDARDFILTERS
      VM_StandardFilters IsStandardFilter(byte *Code,int CodeSize);
      void ExecuteStandardFilter(VM_StandardFilters FilterType);
      dword FilterItanium_GetBits(byte *Data,int BitPos,int BitCount);
      void FilterItanium_SetBits(byte *Data,dword BitField,int BitPos, int BitCount);
#endif

      byte *Mem;
      dword R[8];
      dword Flags;
   public:
      C_RarVM():
         Mem(NULL)
      {}
      ~C_RarVM(){
         delete[] Mem;
      }
      void Init(){
         if (Mem==NULL)
            Mem=new(true) byte[VM_MEMSIZE+4];
      }
      void Prepare(byte *Code, int CodeSize, VM_PreparedProgram *Prg);
      void Execute(VM_PreparedProgram *Prg);
      void SetValue(dword *Addr,dword Value){ SetValue(false,Addr,Value); }
      void SetMemory(dword Pos,byte *Data,dword DataSize);
      static dword ReadData(C_BitInput &Inp);
   };

//----------------------------

   struct UnpackFilter{
      dword BlockStart;
      dword BlockLength;
      dword ExecCount;
      bool NextWindow;

      // position of parent filter in Filters array used as prototype for filter
      // in PrgStack array. Not defined for filters in Filters array.
      dword ParentFilter;

      VM_PreparedProgram Prg;
   };

//----------------------------

protected:
   void Unpack29();
   bool UnpReadBuf();
   //void UnpWriteBuf();
   //void ExecuteCode(VM_PreparedProgram *Prg);
   //void UnpWriteArea(dword StartPtr,dword EndPtr);
   //void UnpWriteData(byte *Data,int Size);
   bool ReadTables();
   void MakeDecodeTables(byte *LenTab, S_decode *Dec,int Size);
   int DecodeNumber(S_decode *Dec);
   void CopyString();
   inline void InsertOldDist(dword Distance);
   inline void InsertLastMatch(dword Length,dword Distance);
   void UnpInitData();
   void CopyString(dword Length,dword Distance);
   bool ReadEndOfBlock();
   bool ReadVMCode();
   bool ReadVMCodePPM();
   bool AddVMCode(dword FirstByte, byte *Code, int CodeSize);
   void InitFilters();

   dword num_to_read;
   C_file *src_file;

   C_model_ppm PPM;
   int PPMEscChar;
   bool PPMError;

   C_RarVM VM;

   //Filters code, one entry per filter
   C_vector<UnpackFilter*> Filters;

   //Filters stack, several entrances of same filter are possible
   C_vector<UnpackFilter*> PrgStack;

   //lengths of preceding blocks, one length per filter. Used to reduce size required to write block length if lengths are repeating
   C_vector<int> OldFilterLengths;

   int LastFilter;

   bool TablesRead;
   S_lit_decode LD;
   S_DistDecode DD;
   S_LowDistDecode LDD;
   S_RepDecode RD;
   S_BitDecode BD;

   dword OldDist[4],OldDistPtr;
   dword LastDist,LastLength;

   int ReadTop;
   int ReadBorder;

   byte UnpOldTable[HUFF_TABLE_SIZE];

   BLOCK_TYPES unp_block_type;

   byte Window[MAXWINSIZE];
   dword win_indx, wr_indx;

   struct S_dest_buf{
      byte *ptr;
      int size;
      int total_size;
      inline void Write(byte b){
         assert(size>0);
         *ptr++ = b;
         --size;
         ++total_size;
      }
      S_dest_buf():
         total_size(0)
      {}
   } dst;

   int num_write_bytes;

   //int WrittenFileSize;

   int PrevLowDist, LowDistRepCount;
   int method;
   bool solid;

   inline void FlushData(){
      dword di = win_indx&MAXWINMASK;
      while(wr_indx!=di && dst.size){
         dst.Write(Window[wr_indx]);
         ++wr_indx &= MAXWINMASK;
      }
   }
   /***************************** Unpack v 1.5 *********************************/
#ifdef SUPPORT_VER_15
   void Unpack15();
   void ShortLZ();
   void LongLZ();
   void HuffDecode();
   void GetFlagsBuf();
   void OldUnpInitData();
   void InitHuff();
   void CorrHuff(dword *CharSet,dword *NumToPlace);
   void OldCopyString(dword Distance,dword Length);
   dword DecodeNum(int Num,dword StartPos, const dword *DecTab, const dword *PosTab);

   dword ChSet[256],ChSetA[256],ChSetB[256],ChSetC[256];
   dword Place[256],PlaceA[256],PlaceB[256],PlaceC[256];
   dword NToPl[256],NToPlB[256],NToPlC[256];
   dword FlagBuf,AvrPlc,AvrPlcB,AvrLn1,AvrLn2,AvrLn3;
   int Buf60,NumHuf,StMode,LCount,FlagsCnt;
   dword Nhfb,Nlzb,MaxDist3;
   /***************************** Unpack v 1.5 *********************************/
#endif//SUPPORT_VER_15

   //void OldUnpWriteBuf();

   /***************************** Unpack v 2.0 *********************************/
   void Unpack20();
   S_MultDecode MD[4];
   byte UnpOldTable20[MC20*4];
   int UnpAudioBlock,UnpChannels,UnpCurChannel,UnpChannelDelta;
   void CopyString20(dword Length,dword Distance);
   bool ReadTables20();
   void UnpInitData20();
   void ReadLastTables();
   byte DecodeAudio(int Delta);
   S_AudioVariables AudV[4];
   /***************************** Unpack v 2.0 *********************************/

public:
   dword GetChar(){
      if(InAddr>MAX_SIZE-30)
         UnpReadBuf();
      return(InBuf[InAddr++]);
   }

public:
   C_unpack();
   ~C_unpack(){
      InitFilters();
   }
   bool Init(C_file *fl, dword src_len, dword dst_len, int Method, bool Solid);
   int DoUnpack(byte *dst, dword len);

};

//----------------------------

C_unpack::C_model_ppm::S_ppm_context* C_unpack::C_model_ppm::S_ppm_context::createChild(C_model_ppm *Model,S_state* pStats, S_state& FirstState){
   S_ppm_context* pc = (S_ppm_context*) Model->SubAlloc.AllocContext();
   if( pc ) {
      pc->NumStats=1;                     
      pc->u.OneState = FirstState;
      pc->Suffix=this;                    
      pStats->Successor=pc;
   }
   return pc;
}

//----------------------------

C_unpack::C_unpack(){
   UnpInitData();
   DDecode_29[1] = 0;
}

//----------------------------

static const byte LDecode[]={0,1,2,3,4,5,6,7,8,10,12,14,16,20,24,28,32,40,48,56,64,80,96,112,128,160,192,224};
static const byte LBits[]=  {0,0,0,0,0,0,0,0,1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4,  4,  5,  5,  5,  5};
static const byte SDDecode[]={0,4,8,16,32,64,128,192};
static const byte SDBits[]=  {2,2,3, 4, 5, 6,  6,  6};
static const int DBitLengthCounts[]= {4,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,14,0,12};

//----------------------------

bool C_unpack::Init(C_file *fl, dword src_len, dword dst_len, int ver, bool _solid){

   src_file = fl;

   method = ver;
   solid = _solid;

   num_to_read = src_len;
   num_write_bytes = dst_len;

   switch(method){
   case 15:
#ifdef SUPPORT_VER_15
      UnpInitData();
      OldUnpInitData();
      UnpReadBuf();
      if(!solid){
         InitHuff();
         win_indx = 0;
      }else
         win_indx = wr_indx;
      --num_write_bytes;
      if(num_write_bytes >= 0){
         GetFlagsBuf();
         FlagsCnt = 8;
      }
#else
      return false;
#endif
      break;
   case 20:
   case 26:
      UnpInitData();
      if(!UnpReadBuf())
         return false;
      if(!solid)
         if(!ReadTables20())
            return false;
      --num_write_bytes;
      break;
   case 29:
   case 36:
      if(DDecode_29[1]==0){
         int Dist=0,BitLength=0,Slot=0;
         for(int I=0;I<sizeof(DBitLengthCounts)/sizeof(DBitLengthCounts[0]);I++,BitLength++){
            for(int J=0;J<DBitLengthCounts[I];J++,Slot++,Dist+=(1<<BitLength)){
               DDecode_29[Slot]=Dist;
               DBits_29[Slot]=BitLength;
            }
         }
      }
      UnpInitData();
      if(!UnpReadBuf())
         return false;
      if((!solid || !TablesRead) && !ReadTables())
         return false;
      if(PPMError)
         return false;
      break;
   default:
      return false;
   }
   return true;
}

//----------------------------

int C_unpack::DoUnpack(byte *dst_buf, dword len){

   dst.ptr = dst_buf;
   dst.size = len;
   dst.total_size = 0;

   switch(method){
#ifdef SUPPORT_VER_15
   case 15: // rar 1.5 compression
      Unpack15();
      break;
#endif
   case 20: // rar 2.x compression
   case 26: // files larger than 2GB
      Unpack20();
      break;
   case 29: // rar 3.x compression
   case 36: // alternative hash
      Unpack29();
      break;
   }
   return dst.total_size;
}

//----------------------------

inline void C_unpack::InsertOldDist(dword Distance){
   OldDist[3]=OldDist[2];
   OldDist[2]=OldDist[1];
   OldDist[1]=OldDist[0];
   OldDist[0]=Distance;
}

//----------------------------

inline void C_unpack::InsertLastMatch(dword Length,dword Distance){
   LastDist=Distance;
   LastLength=Length;
}

//----------------------------

void C_unpack::CopyString(dword Length,dword Distance){

   dword DestPtr = win_indx - Distance;
   if(DestPtr<MAXWINSIZE-260 && win_indx<MAXWINSIZE-260){
      Window[win_indx++] = Window[DestPtr++];
      while(--Length>0)
         Window[win_indx++] = Window[DestPtr++];
   }else
   while (Length--){
      Window[win_indx] = Window[DestPtr++ & MAXWINMASK];
      win_indx = (win_indx+1) & MAXWINMASK;
   }
}

//----------------------------

int C_unpack::DecodeNumber(S_decode *Dec){

   dword Bits;
   dword BitField=getbits() & 0xfffe;
   if (BitField<Dec->DecodeLen[8]){
      if (BitField<Dec->DecodeLen[4]){
         if (BitField<Dec->DecodeLen[2]){
            if (BitField<Dec->DecodeLen[1])
               Bits=1;
            else
               Bits=2;
         }else{
            if (BitField<Dec->DecodeLen[3])
               Bits=3;
            else
               Bits=4;
         }
      }else{
         if (BitField<Dec->DecodeLen[6]){
            if (BitField<Dec->DecodeLen[5])
               Bits=5;
            else
               Bits=6;
         }else{
            if (BitField<Dec->DecodeLen[7])
               Bits=7;
            else
               Bits=8;
         }
      }
   }else{
      if (BitField<Dec->DecodeLen[12]){
         if (BitField<Dec->DecodeLen[10]){
            if (BitField<Dec->DecodeLen[9])
               Bits=9;
            else
               Bits=10;
         }else{
            if (BitField<Dec->DecodeLen[11])
               Bits=11;
            else
               Bits=12;
         }
      }else{
         if (BitField<Dec->DecodeLen[14]){
            if (BitField<Dec->DecodeLen[13])
               Bits=13;
            else
               Bits=14;
         }else
            Bits=15;
      }
   }
   addbits(Bits);
   dword N=Dec->DecodePos[Bits]+((BitField-Dec->DecodeLen[Bits-1])>>(16-Bits));
   if (N>=Dec->MaxNum)
      N=0;
   return(Dec->DecodeNum[N]);
}

//----------------------------

void C_unpack::Unpack29(){

   while(true){
   //while(num_write_bytes>=0){
      FlushData();
      if(!dst.size)
         break;

      win_indx &= MAXWINMASK;
      if(InAddr>ReadBorder){
         if(!UnpReadBuf())
            break;
      }
      //if(((wr_indx-win_indx) & MAXWINMASK)<260 && wr_indx!=win_indx)
         //UnpWriteBuf();
      if(unp_block_type==BLOCK_PPM){
         int Ch=PPM.DecodeChar();
         if(Ch==-1){
            PPMError = true;
            break;
         }
         if(Ch==PPMEscChar){
            int NextCh=PPM.DecodeChar();
            if(NextCh==0){
               if (!ReadTables())
                  break;
               continue;
            }
            if(NextCh==2 || NextCh==-1)
               break;
            if (NextCh==3) {
               if (!ReadVMCodePPM())
                  break;
               continue;
            }
            if (NextCh==4) {
               dword Distance=0, Length = 0;
               bool Failed=false;
               for (int I=0;I<4 && !Failed;I++) {
                  int Ch1=PPM.DecodeChar();
                  if (Ch1==-1)
                     Failed=true;
                  else
                  if (I==3)
                     Length=(byte)Ch1;
                  else
                     Distance=(Distance<<8)+(byte)Ch1;
               }
               if (Failed)
                  break;
               CopyString(Length+32,Distance+2);
               continue;
            }
            if (NextCh==5) {
               int Length=PPM.DecodeChar();
               if (Length==-1)
                  break;
               CopyString(Length+4,1);
               continue;
            }
         }
         Window[win_indx++] = Ch;
         continue;
      }
      int Number=DecodeNumber((S_decode *)&LD);
      if(Number<256){
         Window[win_indx++] = (byte)Number;
         continue;
      }
      if(Number>=271){
         int Length=LDecode[Number-=271]+3;
         dword Bits=LBits[Number];
         if(Bits>0){
            Length+=getbits()>>(16-Bits);
            addbits(Bits);
         }
         int DistNumber=DecodeNumber((S_decode *)&DD);
         dword Distance = DDecode_29[DistNumber]+1;
         if((Bits=DBits_29[DistNumber])>0){
            if(DistNumber>9){
               if(Bits>4){
                  Distance+=((getbits()>>(20-Bits))<<4);
                  addbits(Bits-4);
               }
               if(LowDistRepCount>0){
                  LowDistRepCount--;
                  Distance += PrevLowDist;
               }else{
                  int LowDist=DecodeNumber((S_decode *)&LDD);
                  if(LowDist==16){
                     LowDistRepCount=LOW_DIST_REP_COUNT-1;
                     Distance += PrevLowDist;
                  }else{
                     Distance+=LowDist;
                     PrevLowDist = LowDist;
                  }
               }
            }else{
               Distance+=getbits()>>(16-Bits);
               addbits(Bits);
            }
         }
         if(Distance>=0x2000){
            Length++;
            if(Distance>=0x40000L)
               Length++;
         }

         InsertOldDist(Distance);
         InsertLastMatch(Length,Distance);
         CopyString(Length,Distance);
         continue;
      }
      if(Number==256){
         if(!ReadEndOfBlock())
            break;
         continue;
      }
      if(Number==257){
         if(!ReadVMCode())
            break;
         continue;
      }
      if(Number==258){
         if(LastLength!=0)
            CopyString(LastLength, LastDist);
         continue;
      }
      if(Number<263){
         int DistNum=Number-259;
         dword Distance=OldDist[DistNum];
         for (int I=DistNum;I>0;I--)
            OldDist[I]=OldDist[I-1];
         OldDist[0]=Distance;

         int LengthNumber=DecodeNumber((S_decode *)&RD);
         int Length=LDecode[LengthNumber]+2;
         dword Bits=LBits[LengthNumber];
         if(Bits>0){
            Length+=getbits()>>(16-Bits);
            addbits(Bits);
         }
         InsertLastMatch(Length,Distance);
         CopyString(Length,Distance);
         continue;
      }
      if(Number<272){
         dword Distance=SDDecode[Number-=263]+1;
         dword Bits=SDBits[Number];
         if(Bits>0){
            Distance+=getbits()>>(16-Bits);
            addbits(Bits);
         }
         InsertOldDist(Distance);
         InsertLastMatch(2,Distance);
         CopyString(2,Distance);
         continue;
      }
   }
   FlushData();
}

//----------------------------

bool C_unpack::ReadEndOfBlock(){
   dword BitField=getbits();
   bool NewTable,NewFile=false;
   if (BitField & 0x8000) {
      NewTable=true;
      addbits(1);
   } else {
      NewFile=true;
      NewTable=(BitField & 0x4000);
      addbits(2);
   }
   TablesRead = !NewTable;
   return !(NewFile || NewTable && !ReadTables());
}

//----------------------------

void C_unpack::InitFilters(){

   OldFilterLengths.clear();
   LastFilter=0;
   for (int I=0;I<Filters.size();I++)
      delete Filters[I];
   Filters.clear();
   for (int I=0;I<PrgStack.size();I++)
      delete PrgStack[I];
   PrgStack.clear();
}

//----------------------------

bool C_unpack::ReadVMCode(){
   dword FirstByte=getbits()>>8;
   addbits(8);
   int Length=(FirstByte & 7)+1;
   if(Length==7){
      Length=(getbits()>>8)+7;
      addbits(8);
   }else
   if(Length==8){
      Length=getbits();
      addbits(16);
   }
   C_buffer<byte> VMCode;
   VMCode.Resize(Length);
   for(int I=0;I<Length;I++){
      if(InAddr>=ReadTop-1 && !UnpReadBuf() && I<Length-1)
         return (false);
      VMCode[I]=getbits()>>8;
      addbits(8);
   }
   return (AddVMCode(FirstByte,&VMCode[0],Length));
}

//----------------------------

bool C_unpack::ReadVMCodePPM(){

   dword FirstByte=PPM.DecodeChar();
   if ((int)FirstByte==-1)
      return(false);
   int Length=(FirstByte & 7)+1;
   if(Length==7) {
      int B1=PPM.DecodeChar();
      if (B1==-1)
         return(false);
      Length=B1+7;
   } else
   if (Length==8) {
      int B1=PPM.DecodeChar();
      if (B1==-1)
         return(false);
      int B2=PPM.DecodeChar();
      if (B2==-1)
         return(false);
      Length=B1*256+B2;
   }
   C_vector<byte> VMCode;
   VMCode.resize(Length);
   for (int I=0;I<Length;I++) {
      int Ch=PPM.DecodeChar();
      if (Ch==-1)
         return(false);
      VMCode[I]=Ch;
   }
   return(AddVMCode(FirstByte,&VMCode[0],Length));
}

//----------------------------

bool C_unpack::AddVMCode(dword FirstByte, byte *Code, int CodeSize){

   C_BitInput Inp;
   Inp.InitBitInput();
   MemCpy(Inp.InBuf,Code, Min(int(C_BitInput::MAX_SIZE), CodeSize));
   VM.Init();

   dword FiltPos;
   if(FirstByte & 0x80){
      FiltPos=C_RarVM::ReadData(Inp);
      if(FiltPos==0)
         InitFilters();
      else
         FiltPos--;
   }else
      FiltPos=LastFilter; // use the same filter as last time

   if(FiltPos>Filters.size() || FiltPos>OldFilterLengths.size())
      return(false);
   LastFilter=FiltPos;
   bool NewFilter=(FiltPos==Filters.size());

   UnpackFilter *StackFilter=new(true) UnpackFilter; // new filter for PrgStack

   UnpackFilter *Filter;
   if(NewFilter){ // new filter code, never used before since VM reset
      // too many different filters, corrupt archive
      if(FiltPos>1024)
         return(false);

      Filter=new(true) UnpackFilter;
      Filters.push_back(Filter);
      StackFilter->ParentFilter = Filters.size()-1;
      OldFilterLengths.push_back(0);
      Filter->ExecCount=0;
   }else{  // filter was used in the past
      Filter=Filters[FiltPos];
      StackFilter->ParentFilter=FiltPos;
      Filter->ExecCount++;
   }

   int EmptyCount=0;
   for(int I=0;I<PrgStack.size();I++){
      PrgStack[I-EmptyCount]=PrgStack[I];
      if (PrgStack[I]==NULL)
         EmptyCount++;
      if (EmptyCount>0)
         PrgStack[I]=NULL;
   }
   if(EmptyCount==0){
      PrgStack.push_back(0);
      EmptyCount=1;
   }
   int StackPos=PrgStack.size()-EmptyCount;
   PrgStack[StackPos]=StackFilter;
   StackFilter->ExecCount=Filter->ExecCount;

   dword BlockStart=C_RarVM::ReadData(Inp);
   if (FirstByte & 0x40)
      BlockStart+=258;
   StackFilter->BlockStart=(BlockStart+win_indx)&MAXWINMASK;
   if (FirstByte & 0x20)
      StackFilter->BlockLength=C_RarVM::ReadData(Inp);
   else
      StackFilter->BlockLength=FiltPos<OldFilterLengths.size() ? OldFilterLengths[FiltPos]:0;
   StackFilter->NextWindow=wr_indx!=win_indx && ((wr_indx-win_indx)&MAXWINMASK)<=BlockStart;

   //  DebugLog("\nNextWindow: win_indx=%08x wr_indx=%08x BlockStart=%08x",win_indx,wr_indx,BlockStart);

   OldFilterLengths[FiltPos]=StackFilter->BlockLength;

   MemSet(StackFilter->Prg.InitR,0,sizeof(StackFilter->Prg.InitR));
   StackFilter->Prg.InitR[3]=VM_GLOBALMEMADDR;
   StackFilter->Prg.InitR[4]=StackFilter->BlockLength;
   StackFilter->Prg.InitR[5]=StackFilter->ExecCount;

   if(FirstByte & 0x10){   // set registers to optional parameters if any
      dword InitMask=Inp.fgetbits()>>9;
      Inp.faddbits(7);
      for (int I=0;I<7;I++)
         if (InitMask & (1<<I))
            StackFilter->Prg.InitR[I]=C_RarVM::ReadData(Inp);
   }
   if(NewFilter){
      dword VMCodeSize=C_RarVM::ReadData(Inp);
      if (VMCodeSize>=0x10000 || VMCodeSize==0)
         return(false);
      C_vector<byte> VMCode;
      VMCode.resize(VMCodeSize);
      for (int I=0;I<VMCodeSize;I++) {
         if (Inp.Overflow(3))
            return(false);
         VMCode[I]=Inp.fgetbits()>>8;
         Inp.faddbits(8);
      }
      VM.Prepare(&VMCode[0],VMCodeSize,&Filter->Prg);
   }
   StackFilter->Prg.AltCmd=&Filter->Prg.Cmd[0];
   StackFilter->Prg.CmdCount=Filter->Prg.CmdCount;

   int StaticDataSize=Filter->Prg.StaticData.size();
   if (StaticDataSize>0 && StaticDataSize<VM_GLOBALMEMSIZE) {
      // read statically defined data contained in DB commands
      //StackFilter->Prg.StaticData.Add(StaticDataSize);
      StackFilter->Prg.StaticData.resize(StackFilter->Prg.StaticData.size()+StaticDataSize);
      MemCpy(&StackFilter->Prg.StaticData[0],&Filter->Prg.StaticData[0],StaticDataSize);
   }

   if (StackFilter->Prg.GlobalData.size()<VM_FIXEDGLOBALSIZE) {
      StackFilter->Prg.GlobalData.clear();
      StackFilter->Prg.GlobalData.resize(VM_FIXEDGLOBALSIZE);
   }
   byte *GlobalData=&StackFilter->Prg.GlobalData[0];
   for (int I=0;I<7;I++)
      VM.SetValue((dword *)&GlobalData[I*4],StackFilter->Prg.InitR[I]);
   VM.SetValue((dword *)&GlobalData[0x1c],StackFilter->BlockLength);
   VM.SetValue((dword *)&GlobalData[0x20],0);
   VM.SetValue((dword *)&GlobalData[0x2c],StackFilter->ExecCount);
   MemSet(&GlobalData[0x30],0,16);

   if (FirstByte & 8){ // put data block passed as parameter if any
      if (Inp.Overflow(3))
         return(false);
      dword DataSize=C_RarVM::ReadData(Inp);
      if (DataSize>VM_GLOBALMEMSIZE-VM_FIXEDGLOBALSIZE)
         return(false);
      dword CurSize=StackFilter->Prg.GlobalData.size();
      if (CurSize<DataSize+VM_FIXEDGLOBALSIZE)
         StackFilter->Prg.GlobalData.resize(CurSize+DataSize+VM_FIXEDGLOBALSIZE-CurSize);
      byte *GlobalData1 = &StackFilter->Prg.GlobalData[VM_FIXEDGLOBALSIZE];
      for (int I=0;I<DataSize;I++) {
         if (Inp.Overflow(3))
            return(false);
         GlobalData1[I]=Inp.fgetbits()>>8;
         Inp.faddbits(8);
      }
   }
   return(true);
}

//----------------------------

dword C_unpack::C_RarVM::ReadData(C_BitInput &Inp){
   dword Data=Inp.fgetbits();
   switch(Data&0xc000) {
   case 0:
      Inp.faddbits(6);
      return((Data>>10)&0xf);
   case 0x4000:
      if ((Data&0x3c00)==0) {
         Data=0xffffff00|((Data>>2)&0xff);
         Inp.faddbits(14);
      } else {
         Data=(Data>>6)&0xff;
         Inp.faddbits(10);
      }
      return(Data);
   case 0x8000:
      Inp.faddbits(2);
      Data=Inp.fgetbits();
      Inp.faddbits(16);
      return(Data);
   default:
      Inp.faddbits(2);
      Data=(Inp.fgetbits()<<16);
      Inp.faddbits(16);
      Data|=Inp.fgetbits();
      Inp.faddbits(16);
      return(Data);
   }
}

//----------------------------

void C_unpack::C_RarVM::DecodeArg(VM_PreparedOperand &Op,bool ByteMode){
   dword Data=fgetbits();
   if (Data & 0x8000) {
      Op.Type=VM_PreparedOperand::VM_OPREG;
      Op.Data=(Data>>12)&7;
      Op.Addr=&R[Op.Data];
      faddbits(4);
   } else
   if ((Data & 0xc000)==0) {
      Op.Type=VM_PreparedOperand::VM_OPINT;
      if (ByteMode) {
         Op.Data=(Data>>6) & 0xff;
         faddbits(10);
      } else {
         faddbits(2);
         Op.Data=ReadData(*this);
      }
   } else {
      Op.Type=VM_PreparedOperand::VM_OPREGMEM;
      if ((Data & 0x2000)==0) {
         Op.Data=(Data>>10)&7;
         Op.Addr=&R[Op.Data];
         Op.Base=0;
         faddbits(6);
      } else {
         if ((Data & 0x1000)==0) {
            Op.Data=(Data>>9)&7;
            Op.Addr=&R[Op.Data];
            faddbits(7);
         } else {
            Op.Data=0;
            faddbits(4);
         }
         Op.Base=ReadData(*this);
      }
   }
}

//----------------------------
enum{
   VMCF_OP0 = 0,
   VMCF_OP1 = 1,
   VMCF_OP2 = 2,
   VMCF_OPMASK = 3,
   VMCF_BYTEMODE = 4,
   VMCF_JUMP = 8,
   VMCF_PROC = 16,
   VMCF_USEFLAGS = 32,
   VMCF_CHFLAGS = 64,
};

static const byte VM_CmdFlags[]={
  /* VM_MOV   */ VMCF_OP2 | VMCF_BYTEMODE                                ,
  /* VM_CMP   */ VMCF_OP2 | VMCF_BYTEMODE | VMCF_CHFLAGS                 ,
  /* VM_ADD   */ VMCF_OP2 | VMCF_BYTEMODE | VMCF_CHFLAGS                 ,
  /* VM_SUB   */ VMCF_OP2 | VMCF_BYTEMODE | VMCF_CHFLAGS                 ,
  /* VM_JZ    */ VMCF_OP1 | VMCF_JUMP | VMCF_USEFLAGS                    ,
  /* VM_JNZ   */ VMCF_OP1 | VMCF_JUMP | VMCF_USEFLAGS                    ,
  /* VM_INC   */ VMCF_OP1 | VMCF_BYTEMODE | VMCF_CHFLAGS                 ,
  /* VM_DEC   */ VMCF_OP1 | VMCF_BYTEMODE | VMCF_CHFLAGS                 ,
  /* VM_JMP   */ VMCF_OP1 | VMCF_JUMP                                    ,
  /* VM_XOR   */ VMCF_OP2 | VMCF_BYTEMODE | VMCF_CHFLAGS                 ,
  /* VM_AND   */ VMCF_OP2 | VMCF_BYTEMODE | VMCF_CHFLAGS                 ,
  /* VM_OR    */ VMCF_OP2 | VMCF_BYTEMODE | VMCF_CHFLAGS                 ,
  /* VM_TEST  */ VMCF_OP2 | VMCF_BYTEMODE | VMCF_CHFLAGS                 ,
  /* VM_JS    */ VMCF_OP1 | VMCF_JUMP | VMCF_USEFLAGS                    ,
  /* VM_JNS   */ VMCF_OP1 | VMCF_JUMP | VMCF_USEFLAGS                    ,
  /* VM_JB    */ VMCF_OP1 | VMCF_JUMP | VMCF_USEFLAGS                    ,
  /* VM_JBE   */ VMCF_OP1 | VMCF_JUMP | VMCF_USEFLAGS                    ,
  /* VM_JA    */ VMCF_OP1 | VMCF_JUMP | VMCF_USEFLAGS                    ,
  /* VM_JAE   */ VMCF_OP1 | VMCF_JUMP | VMCF_USEFLAGS                    ,
  /* VM_PUSH  */ VMCF_OP1                                                ,
  /* VM_POP   */ VMCF_OP1                                                ,
  /* VM_CALL  */ VMCF_OP1 | VMCF_PROC                                    ,
  /* VM_RET   */ VMCF_OP0 | VMCF_PROC                                    ,
  /* VM_NOT   */ VMCF_OP1 | VMCF_BYTEMODE                                ,
  /* VM_SHL   */ VMCF_OP2 | VMCF_BYTEMODE | VMCF_CHFLAGS                 ,
  /* VM_SHR   */ VMCF_OP2 | VMCF_BYTEMODE | VMCF_CHFLAGS                 ,
  /* VM_SAR   */ VMCF_OP2 | VMCF_BYTEMODE | VMCF_CHFLAGS                 ,
  /* VM_NEG   */ VMCF_OP1 | VMCF_BYTEMODE | VMCF_CHFLAGS                 ,
  /* VM_PUSHA */ VMCF_OP0                                                ,
  /* VM_POPA  */ VMCF_OP0                                                ,
  /* VM_PUSHF */ VMCF_OP0 | VMCF_USEFLAGS                                ,
  /* VM_POPF  */ VMCF_OP0 | VMCF_CHFLAGS                                 ,
  /* VM_MOVZX */ VMCF_OP2                                                ,
  /* VM_MOVSX */ VMCF_OP2                                                ,
  /* VM_XCHG  */ VMCF_OP2 | VMCF_BYTEMODE                                ,
  /* VM_MUL   */ VMCF_OP2 | VMCF_BYTEMODE                                ,
  /* VM_DIV   */ VMCF_OP2 | VMCF_BYTEMODE                                ,
  /* VM_ADC   */ VMCF_OP2 | VMCF_BYTEMODE | VMCF_USEFLAGS | VMCF_CHFLAGS ,
  /* VM_SBB   */ VMCF_OP2 | VMCF_BYTEMODE | VMCF_USEFLAGS | VMCF_CHFLAGS ,
  /* VM_PRINT */ VMCF_OP0
};

//----------------------------

void C_unpack::C_RarVM::Prepare(byte *Code,int CodeSize,VM_PreparedProgram *Prg){
   InitBitInput();
   MemCpy(InBuf,Code, Min(CodeSize, int(C_BitInput::MAX_SIZE)));

   byte XorSum=0;
   for (int I=1;I<CodeSize;I++)
      XorSum^=Code[I];

   faddbits(8);

   Prg->CmdCount=0;
   if (XorSum==Code[0])
   {
#ifdef VM_STANDARDFILTERS
      VM_StandardFilters FilterType=IsStandardFilter(Code,CodeSize);
      if (FilterType!=VMSF_NONE) {
         Prg->Cmd.Add(1);
         VM_PreparedCommand *CurCmd=&Prg->Cmd[Prg->CmdCount++];
         CurCmd->OpCode=VM_STANDARD;
         CurCmd->Op1.Data=FilterType;
         CurCmd->Op1.Addr=&CurCmd->Op1.Data;
         CurCmd->Op2.Addr=&CurCmd->Op2.Data;
         CurCmd->Op1.Type=CurCmd->Op2.Type=VM_OPNONE;
         CodeSize=0;
      }
#endif  
      dword DataFlag=fgetbits();
      faddbits(1);

      /* Read static data contained in DB operators. This data cannot be changed,
      it is a part of VM code, not a filter parameter.
      */
      if (DataFlag&0x8000) {
         int DataSize=ReadData(*this)+1;
         for (int I=0;InAddr<CodeSize && I<DataSize;I++) {
            Prg->StaticData.push_back(0);
            Prg->StaticData[I]=fgetbits()>>8;
            faddbits(8);
         }
      }
      while (InAddr<CodeSize) {
         Prg->Cmd.push_back(VM_PreparedCommand());
         VM_PreparedCommand *CurCmd=&Prg->Cmd[Prg->CmdCount];
         dword Data=fgetbits();
         if ((Data&0x8000)==0) {
            CurCmd->OpCode=(VM_Commands)(Data>>12);
            faddbits(4);
         } else {
            CurCmd->OpCode=(VM_Commands)((Data>>10)-24);
            faddbits(6);
         }
         if (VM_CmdFlags[CurCmd->OpCode] & VMCF_BYTEMODE) {
            CurCmd->ByteMode=fgetbits()>>15;
            faddbits(1);
         } else
            CurCmd->ByteMode=0;
         CurCmd->Op1.Type=CurCmd->Op2.Type=VM_PreparedOperand::VM_OPNONE;
         int OpNum=(VM_CmdFlags[CurCmd->OpCode] & VMCF_OPMASK);
         CurCmd->Op1.Addr=CurCmd->Op2.Addr=NULL;
         if (OpNum>0) {
            DecodeArg(CurCmd->Op1,CurCmd->ByteMode);
            if (OpNum==2)
               DecodeArg(CurCmd->Op2,CurCmd->ByteMode);
            else {
               if (CurCmd->Op1.Type==VM_PreparedOperand::VM_OPINT && (VM_CmdFlags[CurCmd->OpCode]&(VMCF_JUMP|VMCF_PROC))) {
                  int Distance=CurCmd->Op1.Data;
                  if (Distance>=256)
                     Distance-=256;
                  else {
                     if (Distance>=136)
                        Distance-=264;
                     else
                     if (Distance>=16)
                        Distance-=8;
                     else
                     if (Distance>=8)
                        Distance-=16;
                     Distance+=Prg->CmdCount;
                  }
                  CurCmd->Op1.Data=Distance;
               }
            }
         }
         Prg->CmdCount++;
      }
   }
   Prg->Cmd.push_back(VM_PreparedCommand());
   VM_PreparedCommand *CurCmd=&Prg->Cmd[Prg->CmdCount++];
   CurCmd->OpCode=VM_RET;
   CurCmd->Op1.Addr=&CurCmd->Op1.Data;
   CurCmd->Op2.Addr=&CurCmd->Op2.Data;
   CurCmd->Op1.Type=CurCmd->Op2.Type=VM_PreparedOperand::VM_OPNONE;

   for (int I=0;I<Prg->CmdCount;I++)
   {
      VM_PreparedCommand *Cmd=&Prg->Cmd[I];
      if (Cmd->Op1.Addr==NULL)
         Cmd->Op1.Addr=&Cmd->Op1.Data;
      if (Cmd->Op2.Addr==NULL)
         Cmd->Op2.Addr=&Cmd->Op2.Data;
   }

#ifdef VM_OPTIMIZE
   if (CodeSize!=0)
      Optimize(Prg);
#endif
}

//----------------------------

bool C_unpack::UnpReadBuf(){

   int DataSize=ReadTop-InAddr;
   if (DataSize<0)
      return(false);
   if(InAddr>MAX_SIZE/2){
      if(DataSize>0)
         //memmove(InBuf,InBuf+InAddr,DataSize);
         MemCpy(InBuf,InBuf+InAddr,DataSize);
      InAddr=0;
      ReadTop=DataSize;
   } else
      DataSize=ReadTop;

   dword cnt = Min(num_to_read, dword((MAX_SIZE-DataSize)&~0xf));
   if(!src_file->Read(InBuf+DataSize, cnt))
      return false;
   num_to_read -= cnt;
   ReadTop+=cnt;
   ReadBorder=ReadTop-30;
   return true;//(ReadCode!=-1);
}

//----------------------------

  /*
void C_unpack::UnpWriteBuf(){
   dword WriteSize = (win_indx-wr_indx)&MAXWINMASK;
   dword WrittenBorder = wr_indx;
  for (int I=0;I<PrgStack.Size();I++) {
    UnpackFilter *flt=PrgStack[I];
    if (flt==NULL)
      continue;
    if (flt->NextWindow)
    {
      flt->NextWindow=false;
      continue;
    }
    dword BlockStart=flt->BlockStart;
    dword BlockLength=flt->BlockLength;
    if (((BlockStart-WrittenBorder)&MAXWINMASK)<WriteSize)
    {
      if (WrittenBorder!=BlockStart)
      {
        UnpWriteArea(WrittenBorder,BlockStart);
        WrittenBorder=BlockStart;
        WriteSize=(win_indx-WrittenBorder)&MAXWINMASK;
      }
      if (BlockLength<=WriteSize)
      {
         assert(0);
        dword BlockEnd=(BlockStart+BlockLength)&MAXWINMASK;
        if (BlockStart<BlockEnd || BlockEnd==0){
          VM.SetMemory(0,Window+BlockStart,BlockLength);
        }else
        {
          dword FirstPartLength=MAXWINSIZE-BlockStart;
          VM.SetMemory(0,Window+BlockStart,FirstPartLength);
          VM.SetMemory(FirstPartLength,Window,BlockEnd);
        }

        VM_PreparedProgram *ParentPrg=&Filters[flt->ParentFilter]->Prg;
        VM_PreparedProgram *Prg=&flt->Prg;

        if (ParentPrg->GlobalData.Size()>VM_FIXEDGLOBALSIZE)
        {
          // copy global data from previous script execution if any
          Prg->GlobalData.Alloc(ParentPrg->GlobalData.Size());
          MemCpy(&Prg->GlobalData[VM_FIXEDGLOBALSIZE],&ParentPrg->GlobalData[VM_FIXEDGLOBALSIZE],ParentPrg->GlobalData.Size()-VM_FIXEDGLOBALSIZE);
        }

        ExecuteCode(Prg);

        if (Prg->GlobalData.Size()>VM_FIXEDGLOBALSIZE)
        {
          // save global data for next script execution
          if (ParentPrg->GlobalData.Size()<Prg->GlobalData.Size())
            ParentPrg->GlobalData.Alloc(Prg->GlobalData.Size());
          MemCpy(&ParentPrg->GlobalData[VM_FIXEDGLOBALSIZE],&Prg->GlobalData[VM_FIXEDGLOBALSIZE],Prg->GlobalData.Size()-VM_FIXEDGLOBALSIZE);
        }
        else
          ParentPrg->GlobalData.Reset();

        byte *FilteredData=Prg->FilteredData;
        dword FilteredDataSize=Prg->FilteredDataSize;

        delete PrgStack[I];
        PrgStack[I]=NULL;
        while (I+1<PrgStack.Size())
        {
          UnpackFilter *NextFilter=PrgStack[I+1];
          if (NextFilter==NULL || NextFilter->BlockStart!=BlockStart ||
              NextFilter->BlockLength!=FilteredDataSize || NextFilter->NextWindow)
            break;

          // apply several filters to same data block

          VM.SetMemory(0,FilteredData,FilteredDataSize);

          VM_PreparedProgram *ParentPrg=&Filters[NextFilter->ParentFilter]->Prg;
          VM_PreparedProgram *NextPrg=&NextFilter->Prg;

          if (ParentPrg->GlobalData.Size()>VM_FIXEDGLOBALSIZE)
          {
            // copy global data from previous script execution if any
            NextPrg->GlobalData.Alloc(ParentPrg->GlobalData.Size());
            MemCpy(&NextPrg->GlobalData[VM_FIXEDGLOBALSIZE],&ParentPrg->GlobalData[VM_FIXEDGLOBALSIZE],ParentPrg->GlobalData.Size()-VM_FIXEDGLOBALSIZE);
          }

          ExecuteCode(NextPrg);

          if (NextPrg->GlobalData.Size()>VM_FIXEDGLOBALSIZE)
          {
            // save global data for next script execution
            if (ParentPrg->GlobalData.Size()<NextPrg->GlobalData.Size())
              ParentPrg->GlobalData.Alloc(NextPrg->GlobalData.Size());
            MemCpy(&ParentPrg->GlobalData[VM_FIXEDGLOBALSIZE],&NextPrg->GlobalData[VM_FIXEDGLOBALSIZE],NextPrg->GlobalData.Size()-VM_FIXEDGLOBALSIZE);
          }
          else
            ParentPrg->GlobalData.Reset();

          FilteredData=NextPrg->FilteredData;
          FilteredDataSize=NextPrg->FilteredDataSize;
          I++;
          delete PrgStack[I];
          PrgStack[I]=NULL;
        }
        UnpIO->UnpWrite(FilteredData,FilteredDataSize);
        WrittenFileSize+=FilteredDataSize;
        WrittenBorder=BlockEnd;
        WriteSize=(win_indx-WrittenBorder)&MAXWINMASK;
      }
      else
      {
        for (int J=I;J<PrgStack.Size();J++)
        {
          UnpackFilter *flt=PrgStack[J];
          if (flt!=NULL && flt->NextWindow)
            flt->NextWindow=false;
        }
        wr_indx = WrittenBorder;
        return;
      }
    }
  }
   UnpWriteArea(wr_indx, win_indx);
   wr_indx = win_indx;
}

//----------------------------

void C_unpack::UnpWriteArea(dword StartPtr,dword EndPtr){
   if (EndPtr<StartPtr) {
      UnpWriteData(&Window[StartPtr],-StartPtr & MAXWINMASK);
      UnpWriteData(Window,EndPtr);
   }else
      UnpWriteData(&Window[StartPtr],EndPtr-StartPtr);
}

//----------------------------

void C_unpack::UnpWriteData(byte *Data,int Size){

   //if(WrittenFileSize >= DestUnpSize) return;
   int WriteSize=Size;
   //int LeftToWrite = DestUnpSize-WrittenFileSize;
   //if(WriteSize>LeftToWrite)
      //WriteSize = LeftToWrite;
   dst_file.Write(Data, WriteSize);
   //WrittenFileSize+=Size;
}
*/

//----------------------------

bool C_unpack::ReadTables(){

   byte BitLength[BC];
   byte Table[HUFF_TABLE_SIZE];
   if(InAddr>ReadTop-25)
      if(!UnpReadBuf())
         return(false);
   faddbits((8-InBit)&7);
   dword BitField=fgetbits();
   if(BitField & 0x8000){
      unp_block_type = BLOCK_PPM;
      return PPM.DecodeInit(this,PPMEscChar);
   }
   unp_block_type = BLOCK_LZ;

   PrevLowDist = 0;
   LowDistRepCount=0;

   if (!(BitField & 0x4000))
      MemSet(UnpOldTable, 0, sizeof(UnpOldTable));
   faddbits(2);
   for (int I=0;I<BC;I++) {
      int Length=(byte)(fgetbits() >> 12);
      faddbits(4);
      if (Length==15) {
         int ZeroCount=(byte)(fgetbits() >> 12);
         faddbits(4);
         if (ZeroCount==0)
            BitLength[I]=15;
         else {
            ZeroCount+=2;
            while (ZeroCount-- > 0 && I<sizeof(BitLength)/sizeof(BitLength[0]))
               BitLength[I++]=0;
            I--;
         }
      } else
         BitLength[I]=Length;
   }
   MakeDecodeTables(BitLength,(S_decode *)&BD,BC);

   const int TableSize=HUFF_TABLE_SIZE;
   for (int I=0;I<TableSize;) {
      if (InAddr>ReadTop-5)
         if (!UnpReadBuf())
            return(false);
      int Number=DecodeNumber((S_decode *)&BD);
      if (Number<16) {
         Table[I] = (Number+UnpOldTable[I]) & 0xf;
         I++;
      } else
      if (Number<18) {
         int N;
         if (Number==16) {
            N=(fgetbits() >> 13)+3;
            faddbits(3);
         } else {
            N=(fgetbits() >> 9)+11;
            faddbits(7);
         }
         while (N-- > 0 && I<TableSize) {
            Table[I]=Table[I-1];
            I++;
         }
      } else {
         int N;
         if (Number==18) {
            N=(fgetbits() >> 13)+3;
            faddbits(3);
         } else {
            N=(fgetbits() >> 9)+11;
            faddbits(7);
         }
         while (N-- > 0 && I<TableSize)
            Table[I++]=0;
      }
   }
   TablesRead = true;
   if (InAddr>ReadTop)
      return(false);
   MakeDecodeTables(&Table[0],(S_decode *)&LD,NC);
   MakeDecodeTables(&Table[NC],(S_decode *)&DD,DC);
   MakeDecodeTables(&Table[NC+DC],(S_decode *)&LDD,LDC);
   MakeDecodeTables(&Table[NC+DC+LDC],(S_decode *)&RD,RC);
   MemCpy(UnpOldTable, Table, sizeof(UnpOldTable));
   return(true);
}

//----------------------------

void C_unpack::UnpInitData(){
   if(!solid){
      TablesRead=false;
      MemSet(OldDist, 0, sizeof(OldDist));
      OldDistPtr = 0;
      LastDist = LastLength = 0;
      //    MemSet(Window,0,MAXWINSIZE);
      MemSet(UnpOldTable, 0, sizeof(UnpOldTable));
      win_indx = wr_indx = 0;
      PPMEscChar=2;
      InitFilters();
   }
   InitBitInput();
   PPMError = false;
   ReadTop=0;
   ReadBorder=0;

   UnpInitData20();
}

//----------------------------

void C_unpack::MakeDecodeTables(byte *LenTab, S_decode *Dec,int Size){

   int LenCount[16],TmpPos[16],I;
   long M,N;
   MemSet(LenCount,0,sizeof(LenCount));
   MemSet(Dec->DecodeNum,0,Size*sizeof(*Dec->DecodeNum));
   for (I=0;I<Size;I++)
      LenCount[LenTab[I] & 0xF]++;

   LenCount[0]=0;
   for (TmpPos[0]=Dec->DecodePos[0]=Dec->DecodeLen[0]=0,N=0,I=1;I<16;I++) {
      N=2*(N+LenCount[I]);
      M=N<<(15-I);
      if (M>0xFFFF)
         M=0xFFFF;
      Dec->DecodeLen[I]=(dword)M;
      TmpPos[I]=Dec->DecodePos[I]=Dec->DecodePos[I-1]+LenCount[I-1];
   }
   for (I=0;I<Size;I++)
      if (LenTab[I]!=0)
         Dec->DecodeNum[TmpPos[LenTab[I] & 0xF]++]=I;
   Dec->MaxNum=Size;
}

//----------------------------
#ifdef SUPPORT_VER_15
//----------------------------//Unpack 15
//----------------------------

#define STARTL1  2
static const dword DecL1[]={0x8000,0xa000,0xc000,0xd000,0xe000,0xea00, 0xee00,0xf000,0xf200,0xf200,0xffff};
static const dword PosL1[]={0,0,0,2,3,5,7,11,16,20,24,32,32};

#define STARTL2  3
static const dword DecL2[]={0xa000,0xc000,0xd000,0xe000,0xea00,0xee00, 0xf000,0xf200,0xf240,0xffff};
static const dword PosL2[]={0,0,0,0,5,7,9,13,18,22,26,34,36};

#define STARTHF0  4
static const dword DecHf0[]={0x8000,0xc000,0xe000,0xf200,0xf200,0xf200, 0xf200,0xf200,0xffff};
static const dword PosHf0[]={0,0,0,0,0,8,16,24,33,33,33,33,33};

#define STARTHF1  5
static const dword DecHf1[]={0x2000,0xc000,0xe000,0xf000,0xf200,0xf200, 0xf7e0,0xffff};
static const dword PosHf1[]={0,0,0,0,0,0,4,44,60,76,80,80,127};

#define STARTHF2  5
static const dword DecHf2[]={0x1000,0x2400,0x8000,0xc000,0xfa00,0xffff, 0xffff,0xffff};
static const dword PosHf2[]={0,0,0,0,0,0,2,7,53,117,233,0,0};

#define STARTHF3  6
static const dword DecHf3[]={0x800,0x2400,0xee00,0xfe80,0xffff,0xffff, 0xffff};
static const dword PosHf3[]={0,0,0,0,0,0,0,2,16,218,251,0,0};

#define STARTHF4  8
static const dword DecHf4[]={0xff00,0xffff,0xffff,0xffff,0xffff,0xffff};
static const dword PosHf4[]={0,0,0,0,0,0,0,0,0,255,0,0,0};

//----------------------------

void C_unpack::Unpack15(){

   while(num_write_bytes>=0){
      win_indx &= MAXWINMASK;

      if(InAddr>ReadTop-30 && !UnpReadBuf())
         break;
      //if(((wr_indx-win_indx) & MAXWINMASK)<270 && wr_indx!=win_indx)
         //OldUnpWriteBuf();
      if(StMode){
         HuffDecode();
         continue;
      }
      if(--FlagsCnt < 0){
         GetFlagsBuf();
         FlagsCnt=7;
      }
      if(FlagBuf & 0x80){
         FlagBuf<<=1;
         if(Nlzb > Nhfb)
            LongLZ();
         else
            HuffDecode();
      }else{
         FlagBuf<<=1;
         if(--FlagsCnt < 0){
            GetFlagsBuf();
            FlagsCnt=7;
         }
         if(FlagBuf & 0x80){
            FlagBuf<<=1;
            if(Nlzb > Nhfb)
               HuffDecode();
            else
               LongLZ();
         }else{
            FlagBuf<<=1;
            ShortLZ();
         }
      }
   }
}

//----------------------------

#define GetShortLen1(pos) ((pos)==1 ? Buf60+3:ShortLen1[pos])
#define GetShortLen2(pos) ((pos)==3 ? Buf60+3:ShortLen2[pos])

void C_unpack::ShortLZ(){
   static const dword ShortLen1[]={1,3,4,4,5,6,7,8,8,4,4,5,6,6,4,0};
   static const dword ShortXor1[]={0,0xa0,0xd0,0xe0,0xf0,0xf8,0xfc,0xfe, 0xff,0xc0,0x80,0x90,0x98,0x9c,0xb0};
   static const dword ShortLen2[]={2,3,3,3,4,4,5,6,6,4,4,5,6,6,4,0};
   static const dword ShortXor2[]={0,0x40,0x60,0xa0,0xd0,0xe0,0xf0,0xf8, 0xfc,0xc0,0x80,0x90,0x98,0x9c,0xb0};

   dword Length,SaveLength;
   dword LastDistance;
   dword Distance;
   int DistancePlace;
   NumHuf=0;

   dword BitField=fgetbits();
   if (LCount==2) {
      faddbits(1);
      if (BitField >= 0x8000) {
         OldCopyString((dword)LastDist,LastLength);
         return;
      }
      BitField <<= 1;
      LCount=0;
   }
   BitField>>=8;
   //  not thread safe, replaced by GetShortLen1 and GetShortLen2 macro
   //  ShortLen1[1]=ShortLen2[3]=Buf60+3;
   if (AvrLn1<37) {
      for (Length=0;;Length++){
         if (((BitField^ShortXor1[Length]) & (~(0xff>>GetShortLen1(Length))))==0)
            break;
      }
      faddbits(GetShortLen1(Length));
   } else {
      for (Length=0;;Length++){
         if (((BitField^ShortXor2[Length]) & (~(0xff>>GetShortLen2(Length))))==0)
            break;
      }
      faddbits(GetShortLen2(Length));
   }
   if (Length >= 9) {
      if (Length == 9) {
         LCount++;
         OldCopyString((dword)LastDist,LastLength);
         return;
      }
      if (Length == 14) {
         LCount=0;
         Length=DecodeNum(fgetbits(),STARTL2,DecL2,PosL2)+5;
         Distance=(fgetbits()>>1) | 0x8000;
         faddbits(15);
         LastLength=Length;
         LastDist=Distance;
         OldCopyString(Distance,Length);
         return;
      }
      LCount=0;
      SaveLength=Length;
      Distance=OldDist[(OldDistPtr-(Length-9)) & 3];
      Length=DecodeNum(fgetbits(),STARTL1,DecL1,PosL1)+2;
      if (Length==0x101 && SaveLength==10) {
         Buf60 ^= 1;
         return;
      }
      if (Distance > 256)
         Length++;
      if (Distance >= MaxDist3)
         Length++;

      OldDist[OldDistPtr++]=Distance;
      OldDistPtr = OldDistPtr & 3;
      LastLength=Length;
      LastDist=Distance;
      OldCopyString(Distance,Length);
      return;
   }

   LCount=0;
   AvrLn1 += Length;
   AvrLn1 -= AvrLn1 >> 4;

   DistancePlace=DecodeNum(fgetbits(),STARTHF2,DecHf2,PosHf2) & 0xff;
   Distance=ChSetA[DistancePlace];
   if (--DistancePlace != -1) {
      PlaceA[Distance]--;
      LastDistance=ChSetA[DistancePlace];
      PlaceA[LastDistance]++;
      ChSetA[DistancePlace+1]=LastDistance;
      ChSetA[DistancePlace]=Distance;
   }
   Length+=2;
   OldDist[OldDistPtr++] = ++Distance;
   OldDistPtr = OldDistPtr & 3;
   LastLength=Length;
   LastDist=Distance;
   OldCopyString(Distance,Length);
}

//----------------------------

void C_unpack::LongLZ(){
   dword Length;
   dword Distance;
   dword DistancePlace,NewDistancePlace;
   dword OldAvr2,OldAvr3;

   NumHuf=0;
   Nlzb+=16;
   if (Nlzb > 0xff) {
      Nlzb=0x90;
      Nhfb >>= 1;
   }
   OldAvr2=AvrLn2;

   dword BitField=fgetbits();
   if (AvrLn2 >= 122)
      Length=DecodeNum(BitField,STARTL2,DecL2,PosL2);
   else
   if (AvrLn2 >= 64)
      Length=DecodeNum(BitField,STARTL1,DecL1,PosL1);
   else
   if (BitField < 0x100) {
      Length=BitField;
      faddbits(16);
   } else {
      for (Length=0;((BitField<<Length)&0x8000)==0;Length++) ;
      faddbits(Length+1);
   }

   AvrLn2 += Length;
   AvrLn2 -= AvrLn2 >> 5;

   BitField=fgetbits();
   if (AvrPlcB > 0x28ff)
      DistancePlace=DecodeNum(BitField,STARTHF2,DecHf2,PosHf2);
   else
   if (AvrPlcB > 0x6ff)
      DistancePlace=DecodeNum(BitField,STARTHF1,DecHf1,PosHf1);
   else
      DistancePlace=DecodeNum(BitField,STARTHF0,DecHf0,PosHf0);

   AvrPlcB += DistancePlace;
   AvrPlcB -= AvrPlcB >> 8;
   while (1) {
      Distance = ChSetB[DistancePlace & 0xff];
      NewDistancePlace = NToPlB[Distance++ & 0xff]++;
      if (!(Distance & 0xff))
         CorrHuff(ChSetB,NToPlB);
      else
         break;
   }

   ChSetB[DistancePlace]=ChSetB[NewDistancePlace];
   ChSetB[NewDistancePlace]=Distance;

   Distance=((Distance & 0xff00) | (fgetbits() >> 8)) >> 1;
   faddbits(7);

   OldAvr3=AvrLn3;
   if (Length!=1 && Length!=4)
   if (Length==0 && Distance <= MaxDist3) {
      AvrLn3++;
      AvrLn3 -= AvrLn3 >> 8;
   } else
   if (AvrLn3 > 0)
      AvrLn3--;
   Length+=3;
   if (Distance >= MaxDist3)
      Length++;
   if (Distance <= 256)
      Length+=8;
   if (OldAvr3 > 0xb0 || AvrPlc >= 0x2a00 && OldAvr2 < 0x40)
      MaxDist3=0x7f00;
   else
      MaxDist3=0x2001;
   OldDist[OldDistPtr++]=Distance;
   OldDistPtr = OldDistPtr & 3;
   LastLength=Length;
   LastDist=Distance;
   OldCopyString(Distance,Length);
}

//----------------------------

void C_unpack::HuffDecode(){
   dword CurByte,NewBytePlace;
   dword Length;
   dword Distance;
   int BytePlace;

   dword BitField=fgetbits();

   if(AvrPlc > 0x75ff)
      BytePlace=DecodeNum(BitField,STARTHF4,DecHf4,PosHf4);
   else
   if(AvrPlc > 0x5dff)
      BytePlace=DecodeNum(BitField,STARTHF3,DecHf3,PosHf3);
   else
   if (AvrPlc > 0x35ff)
      BytePlace=DecodeNum(BitField,STARTHF2,DecHf2,PosHf2);
   else
   if (AvrPlc > 0x0dff)
      BytePlace=DecodeNum(BitField,STARTHF1,DecHf1,PosHf1);
   else
      BytePlace=DecodeNum(BitField,STARTHF0,DecHf0,PosHf0);
   BytePlace&=0xff;
   if(StMode){
      if(BytePlace==0 && BitField > 0xfff)
         BytePlace=0x100;
      if(--BytePlace==-1){
         BitField=fgetbits();
         faddbits(1);
         if(BitField & 0x8000){
            NumHuf=StMode=0;
            return;
         }else{
            Length = (BitField & 0x4000) ? 4 : 3;
            faddbits(1);
            Distance=DecodeNum(fgetbits(),STARTHF2,DecHf2,PosHf2);
            Distance = (Distance << 5) | (fgetbits() >> 11);
            faddbits(5);
            OldCopyString(Distance,Length);
            return;
         }
      }
   }else
   if (NumHuf++ >= 16 && FlagsCnt==0)
      StMode=1;
   AvrPlc += BytePlace;
   AvrPlc -= AvrPlc >> 8;
   Nhfb+=16;
   if(Nhfb > 0xff){
      Nhfb=0x90;
      Nlzb >>= 1;
   }
   Window[win_indx++] = (byte)(ChSet[BytePlace]>>8);
   --num_write_bytes;
   while(1){
      CurByte=ChSet[BytePlace];
      NewBytePlace=NToPl[CurByte++ & 0xff]++;
      if ((CurByte & 0xff) > 0xa1)
         CorrHuff(ChSet,NToPl);
      else
         break;
   }
   ChSet[BytePlace]=ChSet[NewBytePlace];
   ChSet[NewBytePlace]=CurByte;
}

//----------------------------

void C_unpack::GetFlagsBuf(){
   dword Flags,NewFlagsPlace;
   dword FlagsPlace=DecodeNum(fgetbits(),STARTHF2,DecHf2,PosHf2);
   while(1){
      Flags=ChSetC[FlagsPlace];
      FlagBuf=Flags>>8;
      NewFlagsPlace=NToPlC[Flags++ & 0xff]++;
      if ((Flags & 0xff) != 0)
         break;
      CorrHuff(ChSetC,NToPlC);
   }

   ChSetC[FlagsPlace]=ChSetC[NewFlagsPlace];
   ChSetC[NewFlagsPlace]=Flags;
}

//----------------------------

#ifdef SUPPORT_VER_15
void C_unpack::OldUnpInitData(){

  if(!solid){
    AvrPlcB=AvrLn1=AvrLn2=AvrLn3=NumHuf=Buf60=0;
    AvrPlc=0x3500;
    MaxDist3=0x2001;
    Nhfb=Nlzb=0x80;
  }
  FlagsCnt=0;
  FlagBuf=0;
  StMode=0;
  LCount=0;
  ReadTop=0;
}
#endif//SUPPORT_VER_15

//----------------------------

void C_unpack::InitHuff(){
  for(dword I=0;I<256;I++){
    Place[I]=PlaceA[I]=PlaceB[I]=I;
    PlaceC[I]=(~I+1) & 0xff;
    ChSet[I]=ChSetB[I]=I<<8;
    ChSetA[I]=I;
    ChSetC[I]=((~I+1) & 0xff)<<8;
  }
  MemSet(NToPl,0,sizeof(NToPl));
  MemSet(NToPlB,0,sizeof(NToPlB));
  MemSet(NToPlC,0,sizeof(NToPlC));
  CorrHuff(ChSetB,NToPlB);
}

//----------------------------

void C_unpack::CorrHuff(dword *CharSet,dword *NumToPlace){
  int I,J;
  for (I=7;I>=0;I--)
    for (J=0;J<32;J++,CharSet++)
      *CharSet=(*CharSet & ~0xff) | I;
  MemSet(NumToPlace,0,sizeof(NToPl));
  for (I=6;I>=0;I--)
    NumToPlace[I]=(7-I)*32;
}

//----------------------------

void C_unpack::OldCopyString(dword Distance,dword Length){
   num_write_bytes -= Length;
   while(Length--){
      Window[win_indx] = Window[(win_indx-Distance) & MAXWINMASK];
      win_indx = (win_indx+1) & MAXWINMASK;
   }
}

//----------------------------

dword C_unpack::DecodeNum(int Num,dword StartPos, const dword *DecTab, const dword *PosTab){
  int I;
  for (Num&=0xfff0,I=0;DecTab[I]<=Num;I++)
    StartPos++;
  faddbits(StartPos);
  return(((Num-(I ? DecTab[I-1]:0))>>(16-StartPos))+PosTab[StartPos]);
}

#endif//SUPPORT_VER_15
//----------------------------

/*
void C_unpack::OldUnpWriteBuf(){
   if(win_indx<wr_indx){
      dst_file.Write(&Window[wr_indx], -wr_indx&MAXWINMASK);
      dst_file.Write(Window, win_indx);
   }else
      dst_file.Write(&Window[wr_indx], win_indx-wr_indx);
   wr_indx = win_indx;
}
*/

//----------------------------
//----------------------------// Unpack 2.0
//----------------------------
void C_unpack::CopyString20(dword Length,dword Distance){
   LastDist=OldDist[OldDistPtr++ & 3]=Distance;
   LastLength=Length;
   num_write_bytes -= Length;

   dword DestPtr = win_indx - Distance;
   if(DestPtr<MAXWINSIZE-300 && win_indx<MAXWINSIZE-300){
      Window[win_indx++] = Window[DestPtr++];
      Window[win_indx++] = Window[DestPtr++];
      while(Length>2){
         Length--;
         Window[win_indx++] = Window[DestPtr++];
      }
   }else{
      while(Length--){
         Window[win_indx] = Window[DestPtr++ & MAXWINMASK];
         win_indx = (win_indx+1) & MAXWINMASK;
      }
   }
}

//----------------------------
static const int DDecode[]={0,1,2,3,4,6,8,12,16,24,32,48,64,96,128,192,256,384,512,768,1024,1536,2048,3072,4096,6144,8192,12288,16384,24576,32768U,49152U,65536,98304,131072,196608,262144,327680,393216,458752,524288,589824,655360,720896,786432,851968,917504,983040};
static const byte DBits[]=  {0,0,0,0,1,1,2, 2, 3, 3, 4, 4, 5, 5,  6,  6,  7,  7,  8,  8,   9,   9,  10,  10,  11,  11,  12,   12,   13,   13,    14,    14,   15,   15,    16,    16,    16,    16,    16,    16,    16,    16,    16,    16,    16,    16,    16,    16};

void C_unpack::Unpack20(){

   while(num_write_bytes>=0){
      FlushData();
      if(!dst.size)
         break;
      win_indx &= MAXWINMASK;

      if(InAddr>ReadTop-30)
         if(!UnpReadBuf())
            break;
      //if(((wr_indx-win_indx) & MAXWINMASK)<270 && wr_indx!=win_indx)
         //OldUnpWriteBuf();
      if(UnpAudioBlock){
         int AudioNumber=DecodeNumber((S_decode *)&MD[UnpCurChannel]);
         if(AudioNumber==256){
            if (!ReadTables20())
               break;
            continue;
         }
         Window[win_indx++] = DecodeAudio(AudioNumber);
         if(++UnpCurChannel==UnpChannels)
            UnpCurChannel=0;
         --num_write_bytes;
         continue;
      }
      int Number=DecodeNumber((S_decode *)&LD);
      if(Number<256){
         Window[win_indx++] = (byte)Number;
         --num_write_bytes;
         continue;
      }
      if(Number>269){
         int Length=LDecode[Number-=270]+3;
         dword Bits = LBits[Number];
         if(Bits>0){
            Length+=getbits()>>(16-Bits);
            addbits(Bits);
         }
         int DistNumber=DecodeNumber((S_decode *)&DD);
         dword Distance=DDecode[DistNumber]+1;
         if((Bits=DBits[DistNumber])>0){
            Distance+=getbits()>>(16-Bits);
            addbits(Bits);
         }
         if(Distance>=0x2000){
            Length++;
            if(Distance>=0x40000L)
               Length++;
         }
         CopyString20(Length, Distance);
         continue;
      }
      if(Number==269){
         if(!ReadTables20())
            break;
         continue;
      }
      if(Number==256){
         CopyString20(LastLength,LastDist);
         continue;
      }
      if(Number<261){
         dword Distance=OldDist[(OldDistPtr-(Number-256)) & 3];
         int LengthNumber=DecodeNumber((S_decode *)&RD);
         int Length=LDecode[LengthNumber]+2;
         dword Bits=LBits[LengthNumber];
         if(Bits>0){
            Length+=getbits()>>(16-Bits);
            addbits(Bits);
         }
         if(Distance>=0x101){
            Length++;
            if(Distance>=0x2000){
               Length++;
               if (Distance>=0x40000)
                  Length++;
            }
         }
         CopyString20(Length,Distance);
         continue;
      }
      if(Number<270){
         dword Distance=SDDecode[Number-=261]+1;
         dword Bits=SDBits[Number];
         if(Bits>0){
            Distance+=getbits()>>(16-Bits);
            addbits(Bits);
         }
         CopyString20(2,Distance);
         continue;
      }
   }
   if(!num_write_bytes)
      ReadLastTables();
   FlushData();
}

//----------------------------

bool C_unpack::ReadTables20(){

   byte BitLength[BC20];
   byte Table[MC20*4];
   int TableSize,N,I;
   if(InAddr>ReadTop-25)
      if(!UnpReadBuf())
         return(false);
   dword BitField=getbits();
   UnpAudioBlock=(BitField & 0x8000);

   if(!(BitField & 0x4000))
      MemSet(UnpOldTable20,0,sizeof(UnpOldTable20));
   addbits(2);

   if(UnpAudioBlock){
      UnpChannels=((BitField>>12) & 3)+1;
      if(UnpCurChannel>=UnpChannels)
         UnpCurChannel=0;
      addbits(2);
      TableSize=MC20*UnpChannels;
   }else
      TableSize=NC20+DC20+RC20;

   for(I=0;I<BC20;I++){
      BitLength[I]=(byte)(getbits() >> 12);
      addbits(4);
   }
   MakeDecodeTables(BitLength,(S_decode *)&BD,BC20);
   I=0;
   while(I<TableSize){
      if(InAddr>ReadTop-5)
         if(!UnpReadBuf())
            return(false);
      int Number=DecodeNumber((S_decode *)&BD);
      if(Number<16){
         Table[I]=(Number+UnpOldTable20[I]) & 0xf;
         I++;
      }else
      if(Number==16){
         N=(getbits() >> 14)+3;
         addbits(2);
         while(N-- > 0 && I<TableSize){
            Table[I]=Table[I-1];
            I++;
         }
      }else{
         if(Number==17){
            N=(getbits() >> 13)+3;
            addbits(3);
         }else{
            N=(getbits() >> 9)+11;
            addbits(7);
         }
         while(N-- > 0 && I<TableSize)
            Table[I++]=0;
      }
   }
   if(InAddr>ReadTop)
      return(true);
   if(UnpAudioBlock){
      for (I=0;I<UnpChannels;I++)
         MakeDecodeTables(&Table[I*MC20],(S_decode *)&MD[I],MC20);
   }else{
      MakeDecodeTables(&Table[0],(S_decode *)&LD,NC20);
      MakeDecodeTables(&Table[NC20],(S_decode *)&DD,DC20);
      MakeDecodeTables(&Table[NC20+DC20],(S_decode *)&RD,RC20);
   }
   MemCpy(UnpOldTable20,Table,sizeof(UnpOldTable20));
   return(true);
}

//----------------------------

void C_unpack::ReadLastTables(){
   if(ReadTop>=InAddr+5){
      if(UnpAudioBlock){
         if (DecodeNumber((S_decode *)&MD[UnpCurChannel])==256)
            ReadTables20();
      }else
      if(DecodeNumber((S_decode *)&LD)==269)
         ReadTables20();
   }
}

//----------------------------

void C_unpack::UnpInitData20(){
   if(!solid){
      UnpChannelDelta=UnpCurChannel=0;
      UnpChannels=1;
      MemSet(AudV,0,sizeof(AudV));
      MemSet(UnpOldTable20,0,sizeof(UnpOldTable20));
   }
}

//----------------------------

byte C_unpack::DecodeAudio(int Delta){
   S_AudioVariables *V=&AudV[UnpCurChannel];
   V->ByteCount++;
   V->D4=V->D3;
   V->D3=V->D2;
   V->D2=V->LastDelta-V->D1;
   V->D1=V->LastDelta;
   int PCh=8*V->LastChar+V->K1*V->D1+V->K2*V->D2+V->K3*V->D3+V->K4*V->D4+V->K5*UnpChannelDelta;
   PCh=(PCh>>3) & 0xFF;

   dword Ch=PCh-Delta;

   int D=((signed char)Delta)<<3;

   V->Dif[0]+=Abs(D);
   V->Dif[1]+=Abs(D-V->D1);
   V->Dif[2]+=Abs(D+V->D1);
   V->Dif[3]+=Abs(D-V->D2);
   V->Dif[4]+=Abs(D+V->D2);
   V->Dif[5]+=Abs(D-V->D3);
   V->Dif[6]+=Abs(D+V->D3);
   V->Dif[7]+=Abs(D-V->D4);
   V->Dif[8]+=Abs(D+V->D4);
   V->Dif[9]+=Abs(D-UnpChannelDelta);
   V->Dif[10]+=Abs(D+UnpChannelDelta);

   UnpChannelDelta=V->LastDelta=(signed char)(Ch-V->LastChar);
   V->LastChar=Ch;

   if ((V->ByteCount & 0x1F)==0)
   {
      dword MinDif=V->Dif[0],NumMinDif=0;
      V->Dif[0]=0;
      for (int I=1;I<sizeof(V->Dif)/sizeof(V->Dif[0]);I++)
      {
         if (V->Dif[I]<MinDif)
         {
            MinDif=V->Dif[I];
            NumMinDif=I;
         }
         V->Dif[I]=0;
      }
      switch(NumMinDif)
      {
      case 1:
         if (V->K1>=-16)
            V->K1--;
         break;
      case 2:
         if (V->K1<16)
            V->K1++;
         break;
      case 3:
         if (V->K2>=-16)
            V->K2--;
         break;
      case 4:
         if (V->K2<16)
            V->K2++;
         break;
      case 5:
         if (V->K3>=-16)
            V->K3--;
         break;
      case 6:
         if (V->K3<16)
            V->K3++;
         break;
      case 7:
         if (V->K4>=-16)
            V->K4--;
         break;
      case 8:
         if (V->K4<16)
            V->K4++;
         break;
      case 9:
         if (V->K5>=-16)
            V->K5--;
         break;
      case 10:
         if (V->K5<16)
            V->K5++;
         break;
      }
   }
   return((byte)Ch);
}


//----------------------------
//----------------------------
//----------------------------

#define ARI_DEC_NORMALIZE(code,low,range,read) { \
   while ((low^(low+range))<C_range_coder::TOP || range<C_range_coder::BOT && ((range=-low&(C_range_coder::BOT-1)),1)){ \
      code=(code << 8) | read->GetChar(); \
      range <<= 8; \
      low <<= 8; \
   } } \

//----------------------------

void C_unpack::C_model_ppm::RestartModelRare(){

   int i, k, m;
   MemSet(CharMask, 0, sizeof(CharMask));
   SubAlloc.InitSubAllocator();
   InitRL = -(MaxOrder < 12 ? MaxOrder:12)-1;
   MinContext = MaxContext = (S_ppm_context*)SubAlloc.AllocContext();
   //Fatal("!", dword(MinContext));
   MinContext->Suffix=NULL;
   OrderFall=MaxOrder;
   MinContext->u.U.SummFreq=(MinContext->NumStats=256)+1;
   FoundState=MinContext->u.U.Stats=(S_ppm_context::S_state*)SubAlloc.AllocUnits(256/2);
   for(RunLength=InitRL, PrevSuccess=i=0;i < 256;i++){
      MinContext->u.U.Stats[i].Symbol=i;      
      MinContext->u.U.Stats[i].Freq=1;
      MinContext->u.U.Stats[i].Successor=NULL;
   }

   static const word InitBinEsc[]={
      0x3CDD,0x1F3F,0x59BF,0x48F3,0x64A1,0x5ABC,0x6632,0x6051
   };

   for (i=0;i < 128;i++)
      for (k=0;k < 8;k++)
         for (m=0;m < 64;m += 8)
            BinSumm[i][k+m] = S_ppm_context::BIN_SCALE-InitBinEsc[k]/(i+2);
   for (i=0;i < 25;i++)
      for (k=0;k < 16;k++)            
         SEE2Cont[i][k].init(5*i+10);
}

//----------------------------

void C_unpack::C_model_ppm::StartModelRare(int _MaxOrder){
   int i, k, m ,Step;
   EscCount=1;
   /*
   if (_MaxOrder < 2) 
   {
   MemSet(CharMask,0,sizeof(CharMask));
   OrderFall=C_model_ppm::_MaxOrder;
   MinContext=MaxContext;
   while (MinContext->Suffix != NULL)
   {
   MinContext=MinContext->Suffix;
   OrderFall--;
   }
   FoundState=MinContext->U.Stats;
   MinContext=MaxContext;
   } 
   else 
   */
   {
      C_model_ppm::MaxOrder = _MaxOrder;
      RestartModelRare();
      NS2BSIndx[0]=2*0;
      NS2BSIndx[1]=2*1;
      MemSet(NS2BSIndx+2,2*2,9);
      MemSet(NS2BSIndx+11,2*3,256-11);
      for(i=0;i < 3;i++)
         NS2Indx[i]=i;
      for(m=i, k=Step=1;i < 256;i++){
         NS2Indx[i]=m;
         if(!--k){ 
            k = ++Step;
            m++; 
         }
      }
      MemSet(HB2Flag,0,0x40);
      MemSet(HB2Flag+0x40,0x08,0x100-0x40);
      DummySEE2Cont.Shift = S_ppm_context::PERIOD_BITS;
   }
}

//----------------------------

void C_unpack::C_model_ppm::S_ppm_context::rescale(C_model_ppm *Model){
   int OldNS=NumStats, i=NumStats-1, Adder, EscFreq;
   S_state* p1, * p;
   for (p=Model->FoundState;p != u.U.Stats;p--)
      Swap(p[0],p[-1]);
   u.U.Stats->Freq += 4;
   u.U.SummFreq += 4;
   EscFreq = u.U.SummFreq-p->Freq;
   Adder=(Model->OrderFall != 0);
   u.U.SummFreq = (p->Freq=(p->Freq+Adder) >> 1);
   do{
      EscFreq -= (++p)->Freq;
      u.U.SummFreq += (p->Freq=(p->Freq+Adder) >> 1);
      if (p[0].Freq > p[-1].Freq) {
         S_state tmp=*(p1=p);
         do { 
            p1[0]=p1[-1]; 
         } while (--p1 != u.U.Stats && tmp.Freq > p1[-1].Freq);
         *p1=tmp;
      }
   } while ( --i );
   if (p->Freq == 0) {
      do { 
         i++; 
      } while ((--p)->Freq == 0);
      EscFreq += i;
      if((NumStats -= i) == 1) {
         S_state tmp = *u.U.Stats;
         do { 
            tmp.Freq-=(tmp.Freq >> 1); 
            EscFreq>>=1; 
         } while (EscFreq > 1);
         Model->SubAlloc.FreeUnits(u.U.Stats,(OldNS+1) >> 1);
         *(Model->FoundState=&u.OneState)=tmp;  return;
      }
   }
   u.U.SummFreq += (EscFreq -= (EscFreq >> 1));
   int n0=(OldNS+1) >> 1, n1=(NumStats+1) >> 1;
   if (n0 != n1)
      u.U.Stats = (S_state*) Model->SubAlloc.ShrinkUnits(u.U.Stats,n0,n1);
   Model->FoundState = u.U.Stats;
}

//----------------------------

inline C_unpack::C_model_ppm::S_ppm_context* C_unpack::C_model_ppm::CreateSuccessors(bool Skip, S_ppm_context::S_state* p1){
#ifdef __ICL
  static
#endif
     S_ppm_context::S_state UpState;
  S_ppm_context* pc=MinContext, * UpBranch=FoundState->Successor;
  S_ppm_context::S_state * p, * ps[MAX_O], ** pps=ps;
  if ( !Skip ) 
  {
    *pps++ = FoundState;
    if ( !pc->Suffix )
      goto NO_LOOP;
  }
  if ( p1 ) 
  {
    p=p1;
    pc=pc->Suffix;
    goto LOOP_ENTRY;
  }
  do 
  {
    pc=pc->Suffix;
    if(pc->NumStats != 1) {
      if ((p=pc->u.U.Stats)->Symbol != FoundState->Symbol)
        do {
          p++; 
        } while (p->Symbol != FoundState->Symbol);
    } else
      p=&(pc->u.OneState);
LOOP_ENTRY:
    if (p->Successor != UpBranch) 
    {
      pc=p->Successor;
      break;
    }
    *pps++ = p;
  } while ( pc->Suffix );
NO_LOOP:
  if (pps == ps)
    return pc;
  UpState.Symbol=*(byte*) UpBranch;
  UpState.Successor=(S_ppm_context*) (((byte*) UpBranch)+1);
  if (pc->NumStats != 1) 
  {
    if ((byte*) pc <= SubAlloc.pText)
      return(NULL);
    if ((p=pc->u.U.Stats)->Symbol != UpState.Symbol)
    do { 
      p++; 
    } while (p->Symbol != UpState.Symbol);
    dword cf=p->Freq-1;
    dword s0=pc->u.U.SummFreq-pc->NumStats-cf;
    UpState.Freq=1+((2*cf <= s0)?(5*cf > s0):((2*cf+3*s0-1)/(2*s0)));
  } 
  else
    UpState.Freq=pc->u.OneState.Freq;
  do 
  {
    pc = pc->createChild(this,*--pps,UpState);
    if ( !pc )
      return NULL;
  } while (pps != ps);
  return pc;
}

//----------------------------

void C_unpack::C_model_ppm::UpdateModel(){
   S_ppm_context::S_state fs = *FoundState, *p = NULL;
   S_ppm_context *pc, *Successor;
   dword ns1, ns, cf, sf, s0;
   if (fs.Freq < S_ppm_context::MAX_FREQ/4 && (pc=MinContext->Suffix) != NULL) {
      if (pc->NumStats != 1) {
         if ((p=pc->u.U.Stats)->Symbol != fs.Symbol) {
            do 
            { 
               p++; 
            } while (p->Symbol != fs.Symbol);
            if (p[0].Freq >= p[-1].Freq) 
            {
               Swap(p[0],p[-1]); 
               p--;
            }
         }
         if (p->Freq < S_ppm_context::MAX_FREQ-9) {
            p->Freq += 2;               
            pc->u.U.SummFreq += 2;
         }
      } else {
         p=&(pc->u.OneState);
         p->Freq += (p->Freq < 32);
      }
   }
   if ( !OrderFall ) 
   {
      MinContext=MaxContext=FoundState->Successor=CreateSuccessors(true,p);
      if ( !MinContext )
         goto RESTART_MODEL;
      return;
   }
   *SubAlloc.pText++ = fs.Symbol;                   
   Successor = (S_ppm_context*) SubAlloc.pText;
   if (SubAlloc.pText >= SubAlloc.FakeUnitsStart)                
      goto RESTART_MODEL;
   if ( fs.Successor ) 
   {
      if ((byte*) fs.Successor <= SubAlloc.pText &&
         (fs.Successor=CreateSuccessors(false,p)) == NULL)
         goto RESTART_MODEL;
      if ( !--OrderFall ) 
      {
         Successor=fs.Successor;
         SubAlloc.pText -= (MaxContext != MinContext);
      }
   } 
   else 
   {
      FoundState->Successor=Successor;
      fs.Successor=MinContext;
   }
   s0=MinContext->u.U.SummFreq-(ns=MinContext->NumStats)-(fs.Freq-1);
   for (pc=MaxContext;pc != MinContext;pc=pc->Suffix) 
   {
      if ((ns1=pc->NumStats) != 1) 
      {
         if ((ns1 & 1) == 0) 
         {
            pc->u.U.Stats=(S_ppm_context::S_state*) SubAlloc.ExpandUnits(pc->u.U.Stats,ns1 >> 1);
            if ( !pc->u.U.Stats )           
               goto RESTART_MODEL;
         }
         pc->u.U.SummFreq += (2*ns1 < ns)+2*((4*ns1 <= ns) & (pc->u.U.SummFreq <= 8*ns1));
      } 
      else 
      {
         p=(S_ppm_context::S_state*) SubAlloc.AllocUnits(1);
         if ( !p )
            goto RESTART_MODEL;
         *p=pc->u.OneState;
         pc->u.U.Stats=p;
         if (p->Freq < S_ppm_context::MAX_FREQ/4-1)
            p->Freq += p->Freq;
         else
            p->Freq  = S_ppm_context::MAX_FREQ-4;
         pc->u.U.SummFreq=p->Freq+InitEsc+(ns > 3);
      }
      cf=2*fs.Freq*(pc->u.U.SummFreq+6);
      sf=s0+pc->u.U.SummFreq;
      if (cf < 6*sf) 
      {
         cf=1+(cf > sf)+(cf >= 4*sf);
         pc->u.U.SummFreq += 3;
      }
      else 
      {
         cf=4+(cf >= 9*sf)+(cf >= 12*sf)+(cf >= 15*sf);
         pc->u.U.SummFreq += cf;
      }
      p=pc->u.U.Stats+ns1;
      p->Successor=Successor;
      p->Symbol = fs.Symbol;
      p->Freq = cf;
      pc->NumStats=++ns1;
   }
   MaxContext=MinContext=fs.Successor;
   return;
RESTART_MODEL:
   RestartModelRare();
   EscCount=0;
}

//----------------------------

// Tabulated escapes for exponential symbol distribution
static const byte ExpEscape[16]={ 25,14, 9, 7, 5, 5, 4, 4, 4, 3, 3, 3, 2, 2, 2, 2 };
#define GET_MEAN(SUMM,SHIFT,ROUND) ((SUMM+(1 << (SHIFT-ROUND))) >> (SHIFT))

inline void C_unpack::C_model_ppm::S_ppm_context::decodeBinSymbol(C_model_ppm *Model){
   S_state& rs=u.OneState;
   Model->HiBitsFlag=Model->HB2Flag[Model->FoundState->Symbol];
   word& bs=Model->BinSumm[rs.Freq-1][Model->PrevSuccess+
      Model->NS2BSIndx[Suffix->NumStats-1]+
      Model->HiBitsFlag+2*Model->HB2Flag[rs.Symbol]+
      ((Model->RunLength >> 26) & 0x20)];
   if (Model->Coder.GetCurrentShiftCount(TOT_BITS) < bs) 
   {
      Model->FoundState=&rs;
      rs.Freq += (rs.Freq < 128);
      Model->Coder.SubRange.LowCount=0;
      Model->Coder.SubRange.HighCount=bs;
      bs = word(bs+INTERVAL-GET_MEAN(bs,PERIOD_BITS,2));
      Model->PrevSuccess=1;
      Model->RunLength++;
   } 
   else 
   {
      Model->Coder.SubRange.LowCount=bs;
      bs = word(bs-GET_MEAN(bs,PERIOD_BITS,2));
      Model->Coder.SubRange.HighCount=BIN_SCALE;
      Model->InitEsc=ExpEscape[bs >> 10];
      Model->NumMasked=1;
      Model->CharMask[rs.Symbol]=Model->EscCount;
      Model->PrevSuccess=0;
      Model->FoundState=NULL;
   }
}

//----------------------------

inline void C_unpack::C_model_ppm::S_ppm_context::update1(C_model_ppm *Model,S_state* p){
  (Model->FoundState=p)->Freq += 4;              
  u.U.SummFreq += 4;
  if (p[0].Freq > p[-1].Freq) 
  {
    Swap(p[0],p[-1]);                   
    Model->FoundState=--p;
    if (p->Freq > MAX_FREQ)             
      rescale(Model);
  }
}

//----------------------------

inline bool C_unpack::C_model_ppm::S_ppm_context::decodeSymbol1(C_model_ppm *Model){
  Model->Coder.SubRange.scale=u.U.SummFreq;
  S_state* p=u.U.Stats;
  int i, HiCnt;
  int count=Model->Coder.GetCurrentCount();
  if (count>=Model->Coder.SubRange.scale)
    return(false);
  if (count < (HiCnt=p->Freq)) 
  {
    Model->PrevSuccess=(2*(Model->Coder.SubRange.HighCount=HiCnt) > Model->Coder.SubRange.scale);
    Model->RunLength += Model->PrevSuccess;
    (Model->FoundState=p)->Freq=(HiCnt += 4);
    u.U.SummFreq += 4;
    if (HiCnt > MAX_FREQ)
      rescale(Model);
    Model->Coder.SubRange.LowCount=0;
    return(true);
  }
  else
    if (Model->FoundState==NULL)
      return(false);
  Model->PrevSuccess=0;
  i=NumStats-1;
  while ((HiCnt += (++p)->Freq) <= count)
    if (--i == 0) 
    {
      Model->HiBitsFlag=Model->HB2Flag[Model->FoundState->Symbol];
      Model->Coder.SubRange.LowCount=HiCnt;
      Model->CharMask[p->Symbol]=Model->EscCount;
      i=(Model->NumMasked=NumStats)-1;
      Model->FoundState=NULL;
      do 
      { 
        Model->CharMask[(--p)->Symbol]=Model->EscCount; 
      } while ( --i );
      Model->Coder.SubRange.HighCount=Model->Coder.SubRange.scale;
      return(true);
    }
  Model->Coder.SubRange.LowCount=(Model->Coder.SubRange.HighCount=HiCnt)-p->Freq;
  update1(Model,p);
  return(true);
}

//----------------------------

inline void C_unpack::C_model_ppm::S_ppm_context::update2(C_model_ppm *Model,S_state* p){
  (Model->FoundState=p)->Freq += 4;              
  u.U.SummFreq += 4;
  if (p->Freq > MAX_FREQ)                 
    rescale(Model);
  Model->EscCount++;
  Model->RunLength=Model->InitRL;
}

//----------------------------

inline C_unpack::C_model_ppm::S_ppm_context::S_see2_context* C_unpack::C_model_ppm::S_ppm_context::makeEscFreq2(C_model_ppm *Model,int Diff){
  S_see2_context* psee2c;
  if (NumStats != 256) 
  {
    psee2c=Model->SEE2Cont[Model->NS2Indx[Diff-1]]+
           (Diff < Suffix->NumStats-NumStats)+
           2*(u.U.SummFreq < 11*NumStats)+4*(Model->NumMasked > Diff)+
           Model->HiBitsFlag;
    Model->Coder.SubRange.scale=psee2c->getMean();
  }
  else 
  {
    psee2c=&Model->DummySEE2Cont;
    Model->Coder.SubRange.scale=1;
  }
  return psee2c;
}

//----------------------------

inline bool C_unpack::C_model_ppm::S_ppm_context::decodeSymbol2(C_model_ppm *Model){
  int count, HiCnt, i=NumStats-Model->NumMasked;
  S_see2_context* psee2c=makeEscFreq2(Model,i);
  S_state* ps[256], ** pps=ps, * p=u.U.Stats-1;
  HiCnt=0;
  do 
  {
    do 
    { 
      p++; 
    } while (Model->CharMask[p->Symbol] == Model->EscCount);
    HiCnt += p->Freq;
    *pps++ = p;
  } while ( --i );
  Model->Coder.SubRange.scale += HiCnt;
  count=Model->Coder.GetCurrentCount();
  if (count>=Model->Coder.SubRange.scale)
    return(false);
  p=*(pps=ps);
  if (count < HiCnt) 
  {
    HiCnt=0;
    while ((HiCnt += p->Freq) <= count) 
      p=*++pps;
    Model->Coder.SubRange.LowCount = (Model->Coder.SubRange.HighCount=HiCnt)-p->Freq;
    psee2c->update();
    update2(Model,p);
  }
  else
  {
    Model->Coder.SubRange.LowCount=HiCnt;
    Model->Coder.SubRange.HighCount=Model->Coder.SubRange.scale;
    i=NumStats-Model->NumMasked;
    pps--;
    do 
    { 
      Model->CharMask[(*++pps)->Symbol]=Model->EscCount; 
    } while ( --i );
    psee2c->Summ += Model->Coder.SubRange.scale;
    Model->NumMasked = NumStats;
  }
  return(true);
}

//----------------------------

inline void C_unpack::C_model_ppm::ClearMask(){
  EscCount=1;                             
  MemSet(CharMask,0,sizeof(CharMask));
}

//----------------------------

bool C_unpack::C_model_ppm::DecodeInit(C_unpack *UnpackRead,int &EscChar){

   int _MaxOrder = UnpackRead->GetChar();
   bool Reset=_MaxOrder & 0x20;

   int MaxMB = 0;
   if(Reset)
      MaxMB = UnpackRead->GetChar();
   else
   if(SubAlloc.GetAllocatedMemory()==0)
      return(false);
   if(_MaxOrder & 0x40)
      EscChar=UnpackRead->GetChar();
   Coder.InitDecoder(UnpackRead);
   if(Reset){
      _MaxOrder=(_MaxOrder & 0x1f)+1;
      if(_MaxOrder>16)
         _MaxOrder=16+(_MaxOrder-16)*3;
      if(_MaxOrder==1){
         SubAlloc.StopSubAllocator();
         return(false);
      }
      //if(!SubAlloc.StartSubAllocator(MaxMB+1))
      if(!SubAlloc.StartSubAllocator(2))
         return false;
      StartModelRare(_MaxOrder);
   }
   return (MinContext!=NULL);
}

//----------------------------

int C_unpack::C_model_ppm::DecodeChar(){
   if ((byte*)MinContext <= SubAlloc.pText || (byte*)MinContext>SubAlloc.HeapEnd)
      return(-1);
   if (MinContext->NumStats != 1)      
   {
      if ((byte*)MinContext->u.U.Stats <= SubAlloc.pText || (byte*)MinContext->u.U.Stats>SubAlloc.HeapEnd)
         return(-1);
      if (!MinContext->decodeSymbol1(this))
         return(-1);
   }
   else                                
      MinContext->decodeBinSymbol(this);
   Coder.S_decode();
   while ( !FoundState ) 
   {
      ARI_DEC_NORMALIZE(Coder.code,Coder.low,Coder.range,Coder.UnpackRead);
      do
      {
         OrderFall++;                
         MinContext=MinContext->Suffix;
         if ((byte*)MinContext <= SubAlloc.pText || (byte*)MinContext>SubAlloc.HeapEnd)
            return(-1);
      } while (MinContext->NumStats == NumMasked);
      if (!MinContext->decodeSymbol2(this))
         return(-1);
      Coder.S_decode();
   }
   int Symbol=FoundState->Symbol;
   if (!OrderFall && (byte*) FoundState->Successor > SubAlloc.pText)
      MinContext=MaxContext=FoundState->Successor;
   else
   {
      UpdateModel();
      if (EscCount == 0)
         ClearMask();
   }
   ARI_DEC_NORMALIZE(Coder.code,Coder.low,Coder.range,Coder.UnpackRead);
   return(Symbol);
}

//----------------------------
bool C_unpack::C_model_ppm::C_sub_allocator::StartSubAllocator(int SASize){
   dword t=SASize << 20;
   if(SubAllocatorSize == t)
      return true;
   StopSubAllocator();
   dword AllocSize=t/FIXED_UNIT_SIZE*UNIT_SIZE+UNIT_SIZE;
   //#ifdef STRICT_ALIGNMENT_REQUIRED
   AllocSize+=UNIT_SIZE;
   //#endif
   if((HeapStart=new byte[AllocSize]) == NULL){
      //ErrHandler.MemoryError();
      return false;
   }
   HeapEnd=HeapStart+AllocSize-UNIT_SIZE;
   SubAllocatorSize=t;
   return true;
}

//----------------------------

void C_unpack::C_model_ppm::C_sub_allocator::InitSubAllocator(){
   int i, k;
   MemSet(FreeList,0,sizeof(FreeList));
   pText = HeapStart;
   dword Size2 = FIXED_UNIT_SIZE*(SubAllocatorSize/8/FIXED_UNIT_SIZE*7);
   dword RealSize2=Size2/FIXED_UNIT_SIZE*UNIT_SIZE;
   dword Size1=SubAllocatorSize-Size2;
   dword RealSize1=Size1/FIXED_UNIT_SIZE*UNIT_SIZE+Size1%FIXED_UNIT_SIZE;
//#ifdef STRICT_ALIGNMENT_REQUIRED
   if (Size1%FIXED_UNIT_SIZE!=0)
      RealSize1+=UNIT_SIZE-Size1%FIXED_UNIT_SIZE;
//#endif
   HiUnit = HeapStart+SubAllocatorSize;
   LoUnit=UnitsStart=HeapStart+RealSize1;
   FakeUnitsStart=HeapStart+Size1;
   HiUnit=LoUnit+RealSize2;
   for (i=0,k=1;i < N1     ;i++,k += 1)
      Indx2Units[i]=k;
   for (k++;i < N1+N2      ;i++,k += 2)
      Indx2Units[i]=k;
   for (k++;i < N1+N2+N3   ;i++,k += 3)
      Indx2Units[i]=k;
   for (k++;i < N1+N2+N3+N4;i++,k += 4)
      Indx2Units[i]=k;
   for (GlueCount=k=i=0;k < 128;k++)
   {
      i += (Indx2Units[i] < k+1);
      Units2Indx[k]=i;
   }
}

//----------------------------

void* C_unpack::C_model_ppm::C_sub_allocator::AllocUnitsRare(int indx){
  if ( !GlueCount )
  {
    GlueCount = 255;
    GlueFreeBlocks();
    if ( FreeList[indx].next )
      return RemoveNode(indx);
  }
  int i=indx;
  do
  {
    if (++i == N_INDEXES)
    {
      GlueCount--;
      i=U2B(Indx2Units[indx]);
      int j=FIXED_UNIT_SIZE*Indx2Units[indx];
      if (FakeUnitsStart-pText > j)
      {
        FakeUnitsStart-=j;
        UnitsStart -= i;
        return(UnitsStart);
      }
      return(NULL);
    }
  } while ( !FreeList[i].next );
  void* RetVal=RemoveNode(i);
  SplitBlock(RetVal,i,indx);
  return RetVal;
}

//----------------------------

void *C_unpack::C_model_ppm::C_sub_allocator::AllocContext(){
   if(HiUnit != LoUnit){
      //Fatal("a");
      return (HiUnit -= UNIT_SIZE);
   }
   //Fatal("b");
   if(FreeList->next)
      return RemoveNode(0);
   return AllocUnitsRare(0);
}

//----------------------------
//----------------------------

C_zip_package *CreateRarPackage(const wchar *fname){

   C_file fl;
   if(!fl.Open(fname))
      return NULL;
   C_rar_package *rp = new C_rar_package;
   if(!rp->Construct(fname, fl)){
      rp->Release();
      rp = NULL;
   }
   return rp;
}

//----------------------------

C_zip_package *CreateRarPackage(C_file *fp, bool take_ownership){

   C_rar_package *rp = new C_rar_package(fp, take_ownership);
   if(!rp->Construct(NULL, *fp)){
      rp->Release();
      rp = NULL;
   }
   return rp;
}

//----------------------------

class C_file_read_rar: public C_file::C_file_base{
   enum{ CACHE_SIZE = 0x2000 };
   byte base[CACHE_SIZE];

   C_file fl;
   C_file *arch_file;
   byte *top, *curr;
   dword curr_pos;         //real file pos

//----------------------------

   const C_rar_package::S_file_entry *fe;

//----------------------------

   C_unpack rar_unpack;

//----------------------------

   dword ReadCache();

//----------------------------
                              //archive we're operating on
   C_smart_ptr<C_rar_package> arch;

public:
//----------------------------
   C_file_read_rar(C_rar_package *a):
      arch_file(NULL),
      curr_pos(0),
      arch(a)
   {
   }
//----------------------------
   ~C_file_read_rar(){
      if(!arch_file)
         fl.Close();
   }
//----------------------------

   virtual C_file::E_FILE_MODE GetMode() const{ return C_file::FILE_READ; }

//----------------------------

   bool Open(const wchar *in_name);

//----------------------------

   virtual bool Read(void *mem, dword len){
      dword read_len = 0;
      const dword wanted_len = len;
      dword rl = 0;
      while(curr+len>top){
         dword rl1 = top-curr;
         MemCpy(mem, curr, rl1);
         len -= rl1;
         mem = (byte*)mem + rl1;
         dword sz = ReadCache();
         curr_pos += sz;
         rl += rl1;
         if(!sz){
            read_len = rl;
            goto check_read_size;
         }
      }
      MemCpy(mem, curr, len);
      curr += len;
      read_len = rl + len;
check_read_size:
      return (read_len == wanted_len);
   }

//----------------------------

   virtual bool ReadByte(byte &v){
      if(curr==top){
         dword sz = ReadCache();
         if(sz < sizeof(byte))
            return false;
         curr_pos += sz;
      }
      v = *curr++;
      return true;
   }

//----------------------------

   virtual bool IsEof() const{
      return (curr==top && GetFileSize()==GetCurrPos());
   }

//----------------------------

   virtual dword GetCurrPos() const{ return curr_pos+curr-top; }

//----------------------------

   bool InitDecompress(){

      if(fe->stored){
         if(arch_file)
            arch_file->Seek(fe->local_offset);
         else
            fl.Seek(fe->local_offset);
         return true;
      }
      if(fe->flags&S_file_header::LHD_SOLID){
                              //analyze all previous files first
         const C_rar_package::S_file_entry *fe1 = (C_rar_package::S_file_entry*)arch->file_info;
         while(fe1!=fe){
            if(arch_file)
               arch_file->Seek(fe1->local_offset);
            else
               fl.Seek(fe1->local_offset);
            if(!rar_unpack.Init(arch_file ? arch_file : &fl, fe1->size_compressed, fe1->size_uncompressed, fe1->version, (fe1->flags&S_file_header::LHD_SOLID)))
               return false;
            int sz = fe1->size_uncompressed;
            while(sz)
               sz -= rar_unpack.DoUnpack(base, Min(int(CACHE_SIZE), sz));
            rar_unpack.DoUnpack(base, CACHE_SIZE);
            fe1 = (C_rar_package::S_file_entry*)(((byte*)fe1) + fe1->GetStructSize());
         }
      }
      if(arch_file)
         arch_file->Seek(fe->local_offset);
      else
         fl.Seek(fe->local_offset);
      return rar_unpack.Init(arch_file ? arch_file : &fl, fe->size_compressed, fe->size_uncompressed, fe->version, (fe->flags&S_file_header::LHD_SOLID));
   }

//----------------------------

   virtual bool SetPos(dword pos){
      if(pos>GetFileSize())
         return false;
      if(pos<GetCurrPos()){
         if(pos >= curr_pos-(top-base)){
            curr = top-(curr_pos-pos);
         }else{
                              //rewind and decompress
            InitDecompress();
            curr = top = base;
            curr_pos = 0;
            SetPos(pos);
         }
      }else
      if(pos > curr_pos){
                              //read from file until we've desired position in cache
         do{
            curr_pos += ReadCache();
         }while(pos > curr_pos);
         curr = base + (pos&(CACHE_SIZE-1));
      }else
         curr = top + pos - curr_pos;
      return true;
   }
//----------------------------
   virtual dword GetFileSize() const{ return fe->size_uncompressed; }
};

//----------------------------

bool C_file_read_rar::Open(const wchar *in_name){

                           //convert name to lowercase, and get length
   Cstr_w name = in_name;
   name.ToLower();
   int name_len = name.Length();
                           //locate the filename among zip entries
   int i;
   fe = (C_rar_package::S_file_entry*)arch->file_info;
   for(i=arch->num_files; i--; ){
      if(fe->CheckName(name, name_len))
         break;
      fe = (C_rar_package::S_file_entry*)(((byte*)fe) + fe->GetStructSize());
   }
   if(i==-1)
      return false;
                              //check supported version
   if(fe->version<13 || fe->version>36)
      return false;
   if(fe->flags&(S_file_header::LHD_PASSWORD|S_file_header::LHD_SPLIT_AFTER|S_file_header::LHD_SPLIT_BEFORE))
      return false;
   if(arch->arch_file)
      arch_file = arch->arch_file;
   else{
                           //open the archive file for reading
      if(!fl.Open(arch->filename_full))
         return false;
   }
   if(!InitDecompress())
      return false;
   curr = top = base;
   return true;
}

//----------------------------

dword C_file_read_rar::ReadCache(){

   curr = base;

   if(fe->stored){
                           //raw read, no compression
      dword sz = Min(int(CACHE_SIZE), int(GetFileSize()-curr_pos));
      if(arch_file)
         arch_file->Read(base, sz);
      else
         fl.Read(base, sz);
      top = base + sz;
      return sz;
   }
                              //read next data from cache
   dword num_read = rar_unpack.DoUnpack(base, Min(int(CACHE_SIZE), int(fe->size_uncompressed)));
   top = base + num_read;
   return num_read;
}

//----------------------------

bool C_file_rar::Open(const wchar *fname, const C_zip_package *arch){

   Close();
   if(!arch)
      return false;
   if(arch->GetArchiveType()!=C_zip_package::TYPE_RAR)
      return false;

   C_file_read_rar *cr = new(true) C_file_read_rar((C_rar_package*)arch);
   imp = cr;
   if(!cr->Open(fname)){
      Close();
      return false;
   }
   return true;
}

//----------------------------
