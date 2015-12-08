//----------------------------
// Multi-platform mobile library
// (c) Lonely Cat Games
//----------------------------

#include <Util.h>

//----------------------------

typedef void (t_SwapMem)(void*, void*, dword width, void *context);
typedef int (t_Comp)(const void*, const void*, void *context);

//----------------------------

static void SwapMem(void *a, void *b, dword width, void *context){

   if(a == b)
      return;
   byte *ap = (byte*)a;
   byte *bp = (byte*)b;
   while(width--){
      byte tmp = *ap;
      *ap++ = *bp;
      *bp++ = tmp;
   }
}

//----------------------------
// Insertion sort for sorting short arrays.
//    assumes that lo < hi
static void ShortSort(byte *lo, byte *hi, dword width, t_SwapMem *p_SwapMem, t_Comp *p_Comp, void *context){

                              //note: in assertions below, i and j are alway inside original bound of array to sort
   while(hi > lo){
                              //A[i] <= A[j] for i <= j, j > hi
      byte *max = lo;
      for(byte *p = lo+width; p <= hi; p += width){
                              //A[i] <= A[max] for lo <= i < p
         if((*p_Comp)(p, max, context) > 0)
            max = p;
                              //A[i] <= A[max] for lo <= i <= p
      }
                              //A[i] <= A[max] for lo <= i <= hi
      (*p_SwapMem)(max, hi, width, context);

                              //A[i] <= A[hi] for i <= hi, so A[i] <= A[j] for i <= j, j >= hi
      hi -= width;
                              //A[i] <= A[j] for i <= j, j > hi, loop top condition established
   }
                              //A[i] <= A[j] for i <= j, j > lo, which implies A[i] <= A[j] for i < j, so array is sorted
}

//----------------------------

                              //this parameter defines the cutoff between using quick sort and
                              // insertion sort for arrays; arrays with lengths shorter or equal to the
                              // below value use insertion sort
const dword CUTOFF = 8;         //testing shows that this is good value

//----------------------------
//    base = pointer to base of array
//    num  = number of elements in the array
//    width = width in bytes of each array element
//    p_Comp = pointer to function returning analog of strcmp for
//               strings, but supplied by user for comparing the array elements.
//               it accepts 2 pointers to elements and returns neg if 1<2, 0 if
//               1=2, pos if 1>2.
void QuickSort(void *base, dword num, dword width, t_Comp *p_Comp, void *context, t_SwapMem *p_SwapMem1){

                              //note: the number of stack entries required is no more than
                              // 1 + log2(size), so 30 is sufficient for any array
   byte *lostk[30], *histk[30];

   if(num < 2 || !width)
      return;                 //nothing to do

                              //stack for saving sub-array to be processed
   int stkptr = 0;

                              //ends of sub-array currently sorting
                              // initialize limits
   byte *lo = (byte*)base;
   byte *hi = (byte*)base + width * (num-1);

   t_SwapMem *p_SwapMem = p_SwapMem1 ? p_SwapMem1 : &SwapMem;

                              //this entry point is for pseudo-recursion calling: setting
                              // lo and hi and jumping to here is like recursion, but stkptr is
                              // preserved, locals aren't, so we preserve stuff on the stack
recurse:
                              //number of elements to sort
   dword size = (hi - lo) / width + 1;

                              //below a certain size, it is faster to use a O(n^2) sorting method
   if(size <= CUTOFF){
      ShortSort(lo, hi, width, p_SwapMem, p_Comp, context);
   }else{
                              //first we pick a partititioning element
                              //the efficiency of the algorithm demands that we find one that is approximately the
                              // median of the values, but also that we select one fast
                              //using the first one produces bad performace if the array is already
                              // sorted, so we use the middle one, which would require a very
                              // wierdly arranged array for worst case performance
                              //testing shows that a median-of-three algorithm does not, in general, increase performance

                              //find middle element
      byte *mid = lo + (size / 2) * width;
                              //swap it to beginning of array
      (*p_SwapMem)(mid, lo, width, context);

                              //we now wish to partition the array into three pieces, one
                              // consisiting of elements <= partition element, one of elements
                              // equal to the parition element, and one of element >= to it
                              //this is done below; comments indicate conditions established at every step
      byte *loguy = lo;
      byte *higuy = hi + width;

                              //note that higuy decreases and loguy increases on every iteration, so loop must terminate
      while(true){
                              //lo <= loguy < hi, lo < higuy <= hi + 1, A[i] <= A[lo] for lo <= i <= loguy,
                              // A[i] >= A[lo] for higuy <= i <= hi
         do{
            loguy += width;
         }while(loguy <= hi && p_Comp(loguy, lo, context) <= 0);

                              //lo < loguy <= hi+1, A[i] <= A[lo] for lo <= i < loguy, either loguy > hi or A[loguy] > A[lo]
         do{
            higuy -= width;
         }while(higuy > lo && p_Comp(higuy, lo, context) >= 0);

                              //lo-1 <= higuy <= hi, A[i] >= A[lo] for higuy < i <= hi, either higuy <= lo or A[higuy] < A[lo]
         if(higuy < loguy)
            break;

                              //if loguy > hi or higuy <= lo, then we would have exited, so
                              // A[loguy] > A[lo], A[higuy] < A[lo], loguy < hi, highy > lo
         (*p_SwapMem)(loguy, higuy, width, context);

                              //A[loguy] < A[lo], A[higuy] > A[lo]; so condition at top of loop is re-established
      }
                              //A[i] >= A[lo] for higuy < i <= hi, A[i] <= A[lo] for lo <= i < loguy,
                              // higuy < loguy, lo <= higuy <= hi
                              //implying:
                              //A[i] >= A[lo] for loguy <= i <= hi, A[i] <= A[lo] for lo <= i <= higuy, A[i] = A[lo] for higuy < i < loguy

                              //put partition element in place
      (*p_SwapMem)(lo, higuy, width, context);

                              //OK, now we have the following:
                              // A[i] >= A[higuy] for loguy <= i <= hi,
                              //  A[i] <= A[higuy] for lo <= i < higuy
                              // A[i] = A[lo] for higuy <= i < loguy

                              //we've finished the partition, now we want to sort the subarrays [lo, higuy-1] and [loguy, hi]
                              //we do the smaller one first to minimize stack usage
                              //we only sort arrays of length 2 or more
      if(higuy - 1 - lo >= hi - loguy){
         if(lo + width < higuy){
                              //save big recursion for later
            lostk[stkptr] = lo;
            histk[stkptr] = higuy - width;
            ++stkptr;
         }
         if(loguy < hi){
                              //do small recursion
            lo = loguy;
            goto recurse;
         }
      }else{
         if(loguy < hi){
                              //save big recursion for later
            lostk[stkptr] = loguy;
            histk[stkptr] = hi;
            ++stkptr;
         }

         if(lo + width < higuy){
                              //do small recursion
            hi = higuy - width;
            goto recurse;
         }
      }
   }
                              //we have sorted the array, except for any pending sorts on the stack.
                              // check if there are any, and do them
   --stkptr;
   if(stkptr >= 0){
                              //pop subarray from stack
      lo = lostk[stkptr];
      hi = histk[stkptr];
      goto recurse;
   }
                              //all subarrays done
}

//----------------------------
