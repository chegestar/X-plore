//----------------------------
// Multi-platform mobile library
// (c) Lonely Cat Games
//----------------------------

#include <AppBase.h>


extern const int NUM_SYSTEM_FONTS = 28;

//----------------------------

void S_rect::SetIntersection(const S_rect &rc1, const S_rect &rc2){
   int x_ = Max(rc1.x, rc2.x);
   int y_ = Max(rc1.y, rc2.y);
   sx = Min(rc1.x+rc1.sx, rc2.x+rc2.sx) - x_;
   sy = Min(rc1.y+rc1.sy, rc2.y+rc2.sy) - y_;
   x = x_;
   y = y_;
}

//----------------------------

void S_rect::SetUnion(const S_rect &rc1, const S_rect &rc2){
   int x1 = Min(rc1.x, rc2.x);
   int y1 = Min(rc1.y, rc2.y);
   sx = Max(rc1.x+rc1.sx, rc2.x+rc2.sx) - x1;
   sy = Max(rc1.y+rc1.sy, rc2.y+rc2.sy) - y1;
   x = x1;
   y = y1;
}

//----------------------------

void S_rect::PlaceToCenter(int size_x, int size_y){
   x = (size_x-sx)/2;
   y = (size_y-sy)/2;
}

//----------------------------

S_rect S_rect::GetExpanded(int _x, int _y) const{
   return S_rect(x-_x, y-_y, sx+2*_x, sy+2*_y);
}

//----------------------------

S_rect S_rect::GetHorizontallyExpanded(int n) const{
   return S_rect(x-n, y, sx+2*n, sy);
}

//----------------------------

S_rect S_rect::GetVerticallyExpanded(int n) const{
   return S_rect(x, y-n, sx, sy+2*n);
}

//----------------------------

S_rect S_rect::GetMoved(int move_x, int move_y) const{
   return S_rect(x+move_x, y+move_y, sx, sy);
}

//----------------------------

bool S_rect::IsPointInRect(const S_point &p) const{
   return (p.x >= x && p.y >= y && p.x < Right() && p.y < Bottom());
}

//----------------------------

bool S_rect::IsRectOverlapped(const S_rect &rc) const{
   return (x<rc.Right() && rc.x<Right() &&
      y<rc.Bottom() && rc.y<Bottom());
}

//----------------------------

void S_pixelformat::Init(byte _bits_per_pixel){

   MemSet(this, 0, sizeof(*this));
   bits_per_pixel = _bits_per_pixel;

   switch(bits_per_pixel){
   case 8:
      bytes_per_pixel = 1;
      break;

#if defined _DEBUG && defined _WIN32_WCE
                              //just simulate lower bitdepth on 565 mode
   case 12:
      bytes_per_pixel = 2;
      r_mask = 0xf000;
      g_mask = 0x0780;
      b_mask = 0x001e;
      break;
   case 15:
      bytes_per_pixel = 2;
      r_mask = 0xf800;
      g_mask = 0x07c0;
      b_mask = 0x001f;
      break;
#else
#ifdef SUPPORT_12BIT_PIXEL
   case 12:
      bytes_per_pixel = 2;
      r_mask = 0x0f00;
      g_mask = 0x00f0;
      b_mask = 0x000f;
      break;
#endif
   case 15:
      bytes_per_pixel = 2;
      r_mask = 0x7c00;
      g_mask = 0x03e0;
      b_mask = 0x001f;
      break;
#endif
   case 16:
      bytes_per_pixel = 2;
      r_mask = 0xf800;
      g_mask = 0x07e0;
      b_mask = 0x001f;
      break;

   case 24:
      bytes_per_pixel = 3;
      r_mask = 0xff0000;
      g_mask = 0x00ff00;
      b_mask = 0x0000ff;
      break;

   case 32:
      bytes_per_pixel = 4;
      r_mask = 0xff0000;
      g_mask = 0x00ff00;
      b_mask = 0x0000ff;
      break;
   }
}

//----------------------------

C_application_base::C_application_base():
   is_focused(false),
   app_obs(NULL),
   igraph(NULL),
   embedded(false),
   last_click_time(0),
   ig_flags(0),
   _scrn_sx(0), _scrn_sy(0),
#ifndef USE_SYSTEM_FONT
   font_defs(internal_font.all_font_defs),
#endif
#ifdef UI_MENU_ON_RIGHT
#if (defined BADA || defined ANDROID)
   menu_on_right(false),
#else
   menu_on_right(true),
#endif
#endif
   use_system_ui(false),
   has_mouse(false),
   has_kb(false),
   has_full_kb(false)
{
   ResetDirtyRect();
#if defined _WINDOWS && defined UI_MENU_ON_RIGHT
   menu_on_right = false;
#endif
}

//----------------------------

const C_smart_ptr<C_zip_package> C_application_base::CreateDtaFile() const{
   const wchar *fn = GetDataFileName();
#ifdef BADA
   Cstr_w tmp; tmp<<L"\\Res\\" <<fn;
   fn = tmp;
#endif
#ifdef ANDROID
   Cstr_w tmp = L"<";
   if(fn){
      tmp <<'\\' <<fn;
   }
   fn = tmp;
#endif
   C_zip_package *dp = C_zip_package::Create(fn, true);
   const C_smart_ptr<C_zip_package> dta = dp;
   if(dp)
      dp->Release();
   else{
      Cstr_w s; s<<L"Can't open DTA file: " <<fn;
      Fatal(s);
   }
   return dta;
}

//----------------------------

void C_application_base::ProcessInputInternal(IG_KEY key, bool auto_repeat, dword key_bits, const S_point &touch_pos, dword pen_state, const S_point *secondary_pos){

   S_user_input ui;
   ui.Clear();
   ui.key = key;
   ui.key_is_autorepeat = auto_repeat;
   ui.key_bits = key_bits;

   if(HasMouse()){
      ui.mouse = touch_pos;
      if(secondary_pos)
         ui.mouse_2nd_touch = *secondary_pos;
      ui.mouse_buttons = pen_state;
      if(ui.mouse_buttons&MOUSE_BUTTON_1_DRAG){
         ui.mouse_rel.x = ui.mouse.x - last_mouse_pos.x;
         ui.mouse_rel.y = ui.mouse.y - last_mouse_pos.y;
         if(!ui.mouse_rel.x && !ui.mouse_rel.y && !secondary_pos)
            ui.mouse_buttons &= ~MOUSE_BUTTON_1_DRAG;
      }
                              //detect double-clicks
      ui.is_doubleclick = false;
      if(ui.mouse_buttons&MOUSE_BUTTON_1_DOWN){
         int t = GetTickTime();
         const int MAX_DBLCLICK_TIME = 500;
         if(t-last_click_time < MAX_DBLCLICK_TIME){
            const int MAX_DBLCLICK_DELTA = (ScrnSX()+ScrnSY())/100;
            if(Abs(last_mouse_pos.x-ui.mouse.x)<MAX_DBLCLICK_DELTA && Abs(last_mouse_pos.y-ui.mouse.y)<MAX_DBLCLICK_DELTA){
               ui.is_doubleclick = true;
               t = 0;
            }
         }
         last_click_time = t;
      }
      if(ui.mouse_buttons&(MOUSE_BUTTON_1_DOWN|MOUSE_BUTTON_1_DRAG))
         last_mouse_pos = ui.mouse;
   }
   ProcessInput(ui);
}

//----------------------------

dword C_application_base::GetPixel(dword rgb) const{

   switch(GetPixelFormat().bits_per_pixel){
   default: assert(0); return 0;
#ifdef SUPPORT_12BIT_PIXEL
   case 12: rgb = ((rgb&0xf00000)>>12) | ((rgb&0xf000)>>8) | ((rgb&0xf0)>>4); break;
#endif
   case 16: rgb = ((rgb&0xf80000)>>8) | ((rgb&0xfc00)>>5) | ((rgb&0xf8)>>3); break;
   case 32: rgb &= 0xffffff; break;
   }
   return rgb;
}

//----------------------------

void C_application_base::ClearScreen(dword color){
   FillRect(S_rect(0, 0, ScrnSX(), ScrnSY()), 0xff000000|color);
}

//----------------------------

void C_application_base::FillRect(const S_rect &rc, dword color, bool optimized){

   byte *dst = (byte*)GetBackBuffer();
   if(!dst)
      return;
   byte alpha = byte(color>>24);
   if(!alpha)
      return;
                              //clip
   int l = Max(clip_rect.x, rc.x);
   int t = Max(clip_rect.y, rc.y);
   int r = Min(clip_rect.Right(), rc.Right());
   int b = Min(clip_rect.Bottom(), rc.Bottom());
   int sx = r - l;
   int sy = b - t;
   if(sx<=0 || sy<=0)
      return;
   const dword bpp = pixel_format.bytes_per_pixel;
   const dword dst_pitch = GetScreenPitch();
   dst += t * dst_pitch + l * bpp;

   switch(alpha){
   case 0x40:
   case 0x80:
   case 0xc0:
   case 0xff:
      optimized = true;
      break;
   }
   if(!optimized){
      S_rgb_x c(color);
      if(pixel_format.bytes_per_pixel==2){
         for(; sy--; dst += dst_pitch){
            for(int i=0; i<sx; i++){
               word &dp = ((word*)dst)[i];
               S_rgb_x d;
               d.From16bit(dp);
               d.BlendWith(c, alpha);
               dp = d.To16bit();
            }
         }
      }else{
         for(; sy--; dst += dst_pitch){
            for(int i=0; i<sx; i++){
               dword &dp = ((dword*)dst)[i];
               S_rgb_x d;
               d.From32bit(dp);
               d.BlendWith(c, alpha);
               dp = d.To32bit();
            }
         }
      }
   }else{
      int alpha_index = (alpha+0x20)>>6;
      if(!alpha_index)
         return;
      color = GetPixel(color);
      dword s_rb = 0, s_g = 0;

      switch(pixel_format.bits_per_pixel){
#ifdef SUPPORT_12BIT_PIXEL
      case 12:
         switch(alpha_index){
         case 1: color = (color>>2)&0x333; break;
         case 2: s_rb = color&0xf0f, s_g = color&0x0f0; break;
         case 3: color = ((color>>2)&0x333) + ((color>>1)&0x777); break;
         }
         break;
#endif
      case 16:
         switch(alpha_index){
         case 1: color = (color>>2)&0x39e7; break;
         case 2: s_rb = color&0xf81f, s_g = color&0x07e0; break;
         case 3: color = ((color>>2)&0x39e7) + ((color>>1)&0x7bef); break;
         }
         break;
      case 32:
         switch(alpha_index){
         case 1: color = (color>>2)&0x3f3f3f; break;
         case 2: color = (color>>1)&0x7f7f7f; break;
         case 3: color = ((color>>2)&0x3f3f3f) + ((color>>1)&0x7f7f7f); break;
         }
         break;
      }

      for(; sy--; dst += dst_pitch){
         int i;
         switch(pixel_format.bits_per_pixel){
#ifdef SUPPORT_12BIT_PIXEL
         case 12:
            switch(alpha_index){
            case 1:        //25%
               for(i=0; i<sx; i++){
                  word &d = ((word*)dst)[i];
                  d = word(((d>>1)&0x777) + ((d>>2)&0x333) + color);
               }
               break;
            case 2:        //50%
               for(i=0; i<sx; i++){
                  word &d = ((word*)dst)[i];
                  //d = ((d>>1)&0x777) + color;
                  word d_rb = d&0xf0f, d_g = d&0x0f0;
                  d = word((((d_rb + s_rb + 0x101) >> 1) & 0xf0f) | (((d_g + s_g + 0x010) >> 1) & 0x0f0));
               }
               break;
            case 3:        //75%
               for(i=0; i<sx; i++){
                  word &d = ((word*)dst)[i];
                  d = word(((d>>2)&0x333) + color);
               }
               break;
            case 4:        //100%
               for(i=0; i<sx; i++)
                  ((word*)dst)[i] = word(color);
               break;
            }
            break;
#endif
         case 16:
            switch(alpha_index){
            case 1:        //25%
               for(i=0; i<sx; i++){
                  word &d = ((word*)dst)[i];
                  d = word(((d>>1)&0x7bef) + ((d>>2)&0x39e7) + color);
               }
               break;
            case 2:        //50%
               for(i=0; i<sx; i++){
                  word &d = ((word*)dst)[i];
                  //d = ((d>>1)&0x7bef) + color;
                  word d_rb = d&0xf81f, d_g = d&0x07e0;
                  d = word((((d_rb + s_rb + 0x0801) >> 1) & 0xf81f) | (((d_g + s_g + 0x020) >> 1) & 0x07e0));
               }
               break;
            case 3:        //75%
               for(i=0; i<sx; i++){
                  word &d = ((word*)dst)[i];
                  d = word(((d>>2)&0x39e7) + color);
               }
               break;
            case 4:        //100%
               for(i=0; i<sx; i++)
                  ((word*)dst)[i] = word(color);
               break;
            }
            break;

         default:
            switch(bpp){
            case 2:
                           //uknown 2-byte format
               if((color&0xff) == ((color>>8)&0xff))
                  MemSet(dst, byte(color), sx*bpp);
               else{
                  for(int j=0; j<sx; j++)
                     ((word*)dst)[j] = word(color);
               }
               break;
            case 4:
                           //true-color
               switch(alpha_index){
               case 1:     //25%
                  for(i=0; i<sx; i++){
                     dword &d = ((dword*)dst)[i];
                     d = ((d>>1)&0x7f7f7f) + ((d>>2)&0x3f3f3f) + color;
                  }
                  break;
               case 2:     //50%
                  for(i=0; i<sx; i++){
                     dword &d = ((dword*)dst)[i];
                     d = ((d>>1)&0x7f7f7f) + color;
                  }
                  break;
               case 3:     //75%
                  for(i=0; i<sx; i++){
                     dword &d = ((dword*)dst)[i];
                     d = ((d>>2)&0x3f3f3f) + color;
                  }
                  break;
               case 4:     //100%
                  for(i=0; i<sx; i++)
                     ((dword*)dst)[i] = color;
                  break;
               }
               break;
            }
         }
      }
   }
   AddDirtyRect(rc);
}

//----------------------------

void C_application_base::DrawHorizontalLine(int x, int y, int width, dword color){

   if(width<0){
      x += width;
      width = -width;
   }
   FillRect(S_rect(x, y, width, 1), color);
}

//----------------------------

void C_application_base::DrawVerticalLine(int x, int y, int height, dword color){

   if(height<0){
      y += height;
      height = -height;
   }
   FillRect(S_rect(x, y, 1, height), color);
}

//----------------------------

void C_application_base::DrawOutline(const S_rect &rc, dword col_lt, dword col_rb){

   if(rc.sx<0 || rc.sy<0)
      return;
                              //top
   DrawHorizontalLine(rc.x-1, rc.y-1, rc.sx+2, col_lt);
                              //bottom
   DrawHorizontalLine(rc.x-1, rc.Bottom(), rc.sx+2, col_rb);
                              //left
   DrawVerticalLine(rc.x-1, rc.y, rc.sy, col_lt);
                              //right
   DrawVerticalLine(rc.Right(), rc.y, rc.sy, col_rb);
}

//----------------------------

void C_application_base::DrawDashedOutline(const S_rect &rc, dword col0, dword col1){

   dword pix0 = GetPixel(col0);
   dword pix1 = GetPixel(col1);
   const S_pixelformat &pf = GetPixelFormat();
   dword scrn_pitch = GetScreenPitch()/pf.bytes_per_pixel;
   const S_rect &curr_clip_rect = GetClipRect();
   int clip_r = curr_clip_rect.Right();
   int clip_b = curr_clip_rect.Bottom();
   int x, y;
   int i;

   byte *dst = (byte*)GetBackBuffer();
   for(i=0; i<2; i++){
                              //horizontal lines
      y = rc.y + (!i ? -1 : rc.sy);
      if(y >= curr_clip_rect.y && y<clip_b){
         if(pf.bytes_per_pixel==2){
            word *d = ((word*)dst) + y*scrn_pitch + rc.x-1;
            for(x=0; x<rc.sx+2; x++, ++d){
               int xx = rc.x+x-1;
               if(xx>=curr_clip_rect.x && xx<clip_r)
                  *d = (((xx^y)&1) ? word(pix0&0xffff) : word(pix1&0xffff));
            }
         }else{
            dword *d = ((dword*)dst) + y*scrn_pitch + rc.x-1;
            for(x=0; x<rc.sx+2; x++, ++d){
               int xx = rc.x+x-1;
               if(xx>=curr_clip_rect.x && xx<clip_r)
                  *d = (((xx^y)&1) ? pix0 : pix1);
            }
         }
      }
                              //vertical lines
      x = rc.x + (!i ? -1 : rc.sx);
      if(x >= curr_clip_rect.x && x<clip_r){
         if(pf.bytes_per_pixel==2){
            word *d = ((word*)dst) + rc.y*scrn_pitch + x;
            for(y=0; y<rc.sy; y++, d+=scrn_pitch){
               int yy = rc.y+y;
               if(yy>=curr_clip_rect.y && yy<clip_b)
                  *d = (((x^yy)&1) ? word(pix0&0xffff) : word(pix1&0xffff));
            }
         }else{
            dword *d = ((dword*)dst) + rc.y*scrn_pitch + x;
            for(y=0; y<rc.sy; y++, d+=scrn_pitch){
               int yy = rc.y+y;
               if(yy>=curr_clip_rect.y && yy<clip_b)
                  *d = (((x^yy)&1) ? pix0 : pix1);
            }
         }
      }
   }
}

//----------------------------

static dword MakeGray(dword val){
   return 0xff000000 | (val<<16) | (val<<8) | val;
}

//----------------------------

dword C_application_base::DrawPlusMinus(int x, int y, int size, bool plus){

   size |= 1;
   S_rect rc(x, y, size, size);
   rc.Compact();
   const dword col = 0xff000000;
   DrawOutline(rc, col);
   //FillRect(rc, 0xffffffff);
                              //draw nice shady area
   dword val_fill = 0xdc;
   FillRect(rc, MakeGray(val_fill));

   dword val_dark = val_fill - size*4;
   dword val_bright = Min(255ul, val_fill + size*4);

   dword col_lt = MakeGray(val_bright);
   dword col_rb = MakeGray(val_dark);
   FillRect(S_rect(rc.x+1, rc.y, rc.sx-2, 1), col_lt);
   FillRect(S_rect(rc.x, rc.y, 1, rc.sy-1), col_lt);

   FillRect(S_rect(rc.x+1, rc.Bottom()-1, rc.sx-1, 1), col_rb);
   FillRect(S_rect(rc.Right()-1, rc.y+1, 1, rc.sy-2), col_rb);


   int border = Max(1, rc.sx*2/8);
   if(size<=5)
      border = 0;
   DrawHorizontalLine(rc.x+border, rc.CenterY(), rc.sx-border*2, col);
   if(plus)
      DrawVerticalLine(rc.CenterX(), rc.y+border, rc.sy-border*2, col);

   return size;
}

//----------------------------

void C_application_base::DrawSimpleShadow(const S_rect &rc, bool expand){

   int add_px = int(expand);
                           //simple line shadow
   S_rect rh(rc.x+1-add_px, rc.Bottom()+add_px, rc.sx+add_px*2-1, 1);
   S_rect rv(rc.Right()+add_px, rc.y+1-add_px, 1, rc.sy+add_px*2);
   for(int i=0; i<3; i++){
      dword color = (0x40 + 0x40*(2-i)) << 24;
      //color = 0xffff0000;
      FillRect(rh, color);
      FillRect(rv, color);
      rh.x++; rh.y++;
      rv.x++; rv.y++;
   }
}

//----------------------------

void C_application_base::DrawArrowHorizontal(int x, int y, int size, dword color, bool right){

   size |= 1;
   dword col1 = ((color>>1)&0xff000000) | (color&0xffffff);
   for(int i=0; i<size; i++){
      int l = size - Abs(i - int(size/2))*2-1;
      if(right){
         if(l)
            DrawHorizontalLine(x, y+i, l, color);
         DrawHorizontalLine(x+l, y+i, 1, col1);
      }else{
         if(l)
            DrawHorizontalLine(x+size, y+i, -l, color);
         DrawHorizontalLine(x+size-l-1, y+i, 1, col1);
      }
   }
}

//----------------------------

void C_application_base::DrawArrowVertical(int x, int y, int size, dword color, bool down){

   size |= 1;
   dword col1 = ((color>>1)&0xff000000) | (color&0xffffff);
   for(int i=0; i<size; i++){
      int l = size - Abs(i - int(size/2))*2-1;
      if(down){
         if(l)
            DrawVerticalLine(x+i, y, l, color);
         DrawVerticalLine(x+i, y+l, 1, col1);
      }else{
         if(l)
            DrawVerticalLine(x+i, y+size, -l, color);
         DrawVerticalLine(x+i, y+size-l-1, 1, col1);
      }
   }
}

//----------------------------

dword C_application_base::GetColor(E_COLOR col) const{

   switch(col){
   case COL_BLACK: return 0xff000000;
   case COL_DARK_GREY: return 0xff404040;
   case COL_GREY: return 0xff808080;
   case COL_LIGHT_GREY: return 0xffc0c0c0;
   case COL_WHITE: return 0xffffffff;
   case COL_SHADOW: return 0xff808080;
   case COL_HIGHLIGHT: return 0xffffffff;
   case COL_TEXT_INPUT: return 0xff000000;
   case COL_TEXT_POPUP:
   case COL_TEXT_HIGHLIGHTED:
   case COL_TEXT_SOFTKEY:
   case COL_TEXT_TITLE:
   case COL_TEXT_MENU:
   case COL_TEXT_MENU_SECONDARY:
   case COL_EDITING_STATE: return GetColor(COL_TEXT);
   case COL_TEXT: return 0xff000000;
   case COL_TEXT_SELECTED: return 0xffffffff;
   case COL_TEXT_SELECTED_BGND: return 0xff0000ff;
   case COL_TOUCH_MENU_LINE: return 0xff0070ff;
   case COL_TOUCH_MENU_FILL: return 0x300070ff;
   case COL_MENU: return 0xffa0a0a0;
   case COL_AREA: return 0xff808080;
   case COL_TITLE: return 0xffc0c0c0;
   case COL_SELECTION: return 0xffe0e0e0;
   case COL_SCROLLBAR: return 0xff606060;
   case COL_WASHOUT:
      return 0xc0000000 | (BlendColor(GetColor(COL_TEXT), 0x808080, 0xa000)&0xffffff);
   }
   return 0xff000000;
}

//----------------------------

void C_application_base::ResetDirtyRect(){
   rc_screen_dirty = S_rect(0x7fffffff, 0x7fffffff, -0x7fffffff, -0x7fffffff);
}

//----------------------------

void C_application_base::AddDirtyRect(const S_rect &rc){
   S_rect rc_i;
   rc_i.SetIntersection(rc, clip_rect);
   if(!rc_i.IsEmpty())
      rc_screen_dirty.SetUnion(rc_screen_dirty, rc_i);
}

//----------------------------

void C_application_base::SetScreenDirty(){
   rc_screen_dirty = S_rect(0, 0, ScrnSX(), ScrnSY());
}

//----------------------------

void C_application_base::UpdateScreen(){

   if(!rc_screen_dirty.IsEmpty()){
      igraph->_UpdateScreen(&rc_screen_dirty, 1);
      ResetDirtyRect();
   }
}

//----------------------------

void C_application_base::SetClipRect(const S_rect &rc){

   //assert(rc.x >= 0);
   //assert(rc.y >= 0);
   //assert(rc.x+rc.sx <= ScrnSX());
   //assert(rc.y+rc.sy <= ScrnSY());
   clip_rect = rc;
                              //clip to screen
   int r = clip_rect.Right(), b = clip_rect.Bottom();
   if(r>ScrnSX())
      clip_rect.sx -= r-ScrnSX();
   if(b>ScrnSY())
      clip_rect.sy -= b-ScrnSY();
   if(clip_rect.x<0){
      clip_rect.sx = Max(0, clip_rect.sx + clip_rect.x);
      clip_rect.x = 0;
   }
   if(clip_rect.y<0){
      clip_rect.sy = Max(0, clip_rect.sy + clip_rect.y);
      clip_rect.y = 0;
   }
   if(system_font.use)
      system_font.SetClipRect(clip_rect);
}

//----------------------------

void C_application_base::ResetClipRect(){
   SetClipRect(S_rect(0, 0, ScrnSX(), ScrnSY()));
}

//----------------------------

dword MulColor(dword col, dword mul_fp){

   dword r = (col>>16) & 0xff;
   dword g = (col>>8) & 0xff;
   dword b = (col>>0) & 0xff;
   r = (r * mul_fp + 0x8000) >> 16;
   if(r >= 256) r = 255;
   g = (g * mul_fp + 0x8000) >> 16;
   if(g >= 256) g = 255;
   b = (b * mul_fp + 0x8000) >> 16;
   if(b >= 256) b = 255;
   return (col&0xff000000) | (r<<16) | (g<<8) | b;
}

//----------------------------

dword MulAlpha(dword col, dword mul_fp){

   dword a = (col>>24) & 0xff;
   a = (a * mul_fp + 0x8000) >> 16;
   if(a >= 256) a = 255;
   return (a<<24) | (col&0xffffff);
}

//----------------------------

dword BlendColor(dword col0, dword col1, dword mul_fp){

   dword e00 = (col0>>16) & 0xff;
   dword e01 = (col0>>8) & 0xff;
   dword e02 = (col0>>0) & 0xff;
   dword e03 = (col0>>24) & 0xff;
   dword e10 = (col1>>16) & 0xff;
   dword e11 = (col1>>8) & 0xff;
   dword e12 = (col1>>0) & 0xff;
   dword e13 = (col1>>24) & 0xff;
   dword mul_inv = 0x10000 - mul_fp;
   dword e0 = (e00*mul_fp + e10*mul_inv) >> 16;
   dword e1 = (e01*mul_fp + e11*mul_inv) >> 16;
   dword e2 = (e02*mul_fp + e12*mul_inv) >> 16;
   dword e3 = (e03*mul_fp + e13*mul_inv) >> 16;
   if(e0 >= 256) e0 = 255;
   if(e1 >= 256) e1 = 255;
   if(e2 >= 256) e2 = 255;
   if(e3 >= 256) e3 = 255;
   return (e3<<24) | (e0<<16) | (e1<<8) | e2;
}

//----------------------------
#ifndef USE_SYSTEM_FONT

static const byte punkt_char_code[S_font::PK_LAST] = {
   0, 125, 123, 157, 126, 127, 155, 124, 156, '.'-' ', 158, 109, '\''-' ', 123
};

//----------------------------

const S_font C_application_base::S_internal_font::all_font_defs[] = {
   {
      6, 8,
      3,
      4,
      11,
      0,
      1, 2, 1,
      {
                                 //NO, HOOK, LINE, 2DOT
         0, 1, 0, 1,
                                 //HOOK_I, CIRCLE, 2LINES, LINE_I
         1, 1, 1, 1,
                              //TILDE, DOT, PK_LOW_HOOK, PK_HLINE
         2, 0, 1, 1,
                              //PK_SIDE_HOOK
         0
      }
   },
   {
      7, 10,
      3,
      5,
      13,
      0,
      1, 2, 1,
      {
                              //NO, HOOK, LINE, 2DOT
         0, 1, 0, 1,
                              //HOOK_I, CIRCLE, 2LINES, LINE_I
         1, 1, 1, 1,
                              //TILDE, DOT, PK_LOW_HOOK, PK_HLINE
         1, 0, 1, 1,
                              //PK_SIDE_HOOK
         0
      }
   },
   {
      9, 12,
      4,
      7,
      15,
      0,
      1, 2, 1,
      {
                              //NO, HOOK, LINE, 2DOT
         0, 1, 0, 1,
                              //HOOK_I, CIRCLE, 2LINES, LINE_I
         1, 1, 1, 1,
                              //TILDE, DOT, PK_LOW_HOOK, PK_HLINE
         1, 0, 1, 2,
                              //PK_SIDE_HOOK
         0
      }
   },
   {
      12, 16,
      5,
      8,
      20,
      0,
      1, 2, 2,
      {
                              //NO, HOOK, LINE, 2DOT
         0, 2, 1, 1,
                              //HOOK_I, CIRCLE, 2LINES, LINE_I
         2, 1, 1, 3,
                              //TILDE, DOT, PK_LOW_HOOK, PK_HLINE
         1, 0, 1, 2,
                              //PK_SIDE_HOOK
         0
      }
   },
   {
      16, 21,
      6,
      10,
      25,
      1,
      1, 4, 2,
      {
                              //NO, HOOK, LINE, 2DOT
         0, 2, 1, 2,
                              //HOOK_I, CIRCLE, 2LINES, LINE_I
         2, 2, 2, 4,
                              //TILDE, DOT, PK_LOW_HOOK, PK_HLINE
         2, 0, 2, 3,
                              //PK_SIDE_HOOK
         0
      }
   },
   {
      20, 26,
      8,
      13,
      31,
      1,
      1, 4, 2,
      {
                              //NO, HOOK, LINE, 2DOT
         0, 3, 0, 4,
                              //HOOK_I, CIRCLE, 2LINES, LINE_I
         4, 2, 4, 4,
                              //TILDE, DOT, PK_LOW_HOOK, PK_HLINE
         4, 0, 3, 3,
                              //PK_SIDE_HOOK
         0
      }
   },
};

//----------------------------

void C_application_base::S_internal_font::Close(){

   for(int i=0; i<NUM_FONTS; i++){
      font_data[i].width_empty_bits.Clear();
      for(int j=0; j<4; j++)
         font_data[i].compr_data[j].Clear();
   }
   curr_def_font_size = -1;
   curr_relative_size = 0;
}

//----------------------------

dword C_application_base::S_internal_font::GetCharMapping(dword c, S_font::E_PUNKTATION &punkt){

   punkt = S_font::PK_NO;

#define CMAP(orig, map, pkt) case (orig): punkt = (pkt); return (map)

   /*
   if(coding==COD_CYRILLIC){
                              //map cyrillic letters to roman letters if possible
      //switch(c){ }
      if(c>=0xc0 && c<=0xff)
         return c;
   }
   */
   switch(c){
   CMAP(0x85, 'a', S_font::PK_LINE_I);
   CMAP(0x88, 0xd5, S_font::PK_NO); //Euro
   CMAP(0x8c, 'S', S_font::PK_LINE);
   CMAP(0x8d, 'T', S_font::PK_HOOK);
   CMAP(0x8e, 'Z', S_font::PK_HOOK);

   CMAP(0x92, '\'', S_font::PK_NO);
   CMAP(0x93, '\"', S_font::PK_NO);
   CMAP(0x94, '\"', S_font::PK_NO);
   CMAP(0x95, 0xc0, S_font::PK_NO);
   CMAP(0x96, '-', S_font::PK_NO);
   CMAP(0x97, '-', S_font::PK_NO);
   CMAP(0x9a, 's', S_font::PK_HOOK);
   CMAP(0x9c, 's', S_font::PK_LINE);
   CMAP(0x9d, 't', S_font::PK_HOOK);
   CMAP(0x9e, 'z', S_font::PK_HOOK);

   //CMAP(0xa0, ' ', S_font::PK_NO);
   CMAP(0xa1, 0x90, S_font::PK_NO); //Spanish reversed !
   CMAP(0xa2, 'c', S_font::PK_LINE);   //cent
   CMAP(0xa3, 0xd0, S_font::PK_NO);
   CMAP(0xa4, 0xc0, S_font::PK_NO);   //¤
   CMAP(0xa5, 0xd1, S_font::PK_NO);
   CMAP(0xa6, '|', S_font::PK_NO);
   CMAP(0xa7, 0xd2, S_font::PK_NO);
   CMAP(0xa8, ' ', S_font::PK_2DOT);
   CMAP(0xa9, 'c', S_font::PK_NO);  //(c)
   CMAP(0xab, '\"', S_font::PK_NO);    //<<
   CMAP(0xac, '-', S_font::PK_NO);
   CMAP(0xad, '-', S_font::PK_NO);    //soft hyphen
   CMAP(0xae, 'R', S_font::PK_NO);    //(R)
   CMAP(0xaf, ' ', S_font::PK_HLINE);

   CMAP(0xb0, 0x9f, S_font::PK_NO);
   CMAP(0xb1, '+', S_font::PK_HLINE);
   CMAP(0xb2, '2', S_font::PK_NO);
   CMAP(0xb3, '3', S_font::PK_NO);
   CMAP(0xb4, '\'', S_font::PK_NO);
   CMAP(0xb5, 'u', S_font::PK_NO); //micro sign
   CMAP(0xb6, '?', S_font::PK_NO); //¶
   CMAP(0xb7, '.', S_font::PK_NO);
   CMAP(0xb8, 0xbe, S_font::PK_NO); //cedilla
   CMAP(0xb9, '1', S_font::PK_NO);
   CMAP(0xba, 0x9f, S_font::PK_NO);
   CMAP(0xbb, '\"', S_font::PK_NO);    //>>
   CMAP(0xbc, '?', S_font::PK_NO);    //¼
   CMAP(0xbd, '?', S_font::PK_NO);    //½
   CMAP(0xbe, '?', S_font::PK_NO);    //¾
   CMAP(0xbf, 0x91, S_font::PK_NO); //Spanish reversed ?

   CMAP(0xc0, 'A', S_font::PK_LINE_I);
   CMAP(0xc1, 'A', S_font::PK_LINE);
   CMAP(0xc2, 'A', S_font::PK_HOOK_I);
   CMAP(0xc3, 'A', S_font::PK_TILDE);
   CMAP(0xc4, 'A', S_font::PK_2DOT);
   CMAP(0xc5, 'A', S_font::PK_CIRCLE);
   CMAP(0xc6, 0x82, S_font::PK_NO); //AE
   CMAP(0xc7, 'C', S_font::PK_LOW_HOOK);
   CMAP(0xc8, 'E', S_font::PK_LINE_I);
   CMAP(0xc9, 'E', S_font::PK_LINE);
   CMAP(0xca, 'E', S_font::PK_HOOK_I);
   CMAP(0xcb, 'E', S_font::PK_2DOT);
   CMAP(0xcc, 'I', S_font::PK_LINE_I);
   CMAP(0xcd, 'I', S_font::PK_LINE);
   CMAP(0xce, 'I', S_font::PK_HOOK_I);
   CMAP(0xcf, 'I', S_font::PK_2DOT);

   CMAP(0xd0, 0x8a, S_font::PK_NO);    //iceland strike D
   CMAP(0xd1, 'N', S_font::PK_TILDE);
   CMAP(0xd2, 'O', S_font::PK_LINE_I);
   CMAP(0xd3, 'O', S_font::PK_LINE);
   CMAP(0xd4, 'O', S_font::PK_HOOK_I);
   CMAP(0xd5, 'O', S_font::PK_TILDE);
   CMAP(0xd6, 'O', S_font::PK_2DOT);
   CMAP(0xd7, 'x', S_font::PK_NO);
   CMAP(0xd8, 0x86, S_font::PK_NO);    //iceland strike O
   CMAP(0xd9, 'U', S_font::PK_LINE_I);
   CMAP(0xda, 'U', S_font::PK_LINE);
   CMAP(0xdb, 'U', S_font::PK_HOOK_I);
   CMAP(0xdc, 'U', S_font::PK_2DOT);
   CMAP(0xdd, 'Y', S_font::PK_LINE);
   CMAP(0xde, 0x88, S_font::PK_NO);
   CMAP(0xdf, 0x98, S_font::PK_NO); //german ss

   CMAP(0xe0, 'a', S_font::PK_LINE_I);
   CMAP(0xe1, 'a', S_font::PK_LINE);
   CMAP(0xe2, 'a', S_font::PK_HOOK_I);
   CMAP(0xe3, 'a', S_font::PK_TILDE);
   CMAP(0xe4, 'a', S_font::PK_2DOT);
   CMAP(0xe5, 'a', S_font::PK_CIRCLE);
   CMAP(0xe6, 0xa2, S_font::PK_NO);       //ae
   CMAP(0xe7, 'c', S_font::PK_LOW_HOOK);
   CMAP(0xe8, 'e', S_font::PK_LINE_I);
   CMAP(0xe9, 'e', S_font::PK_LINE);
   CMAP(0xea, 'e', S_font::PK_HOOK_I);
   CMAP(0xeb, 'e', S_font::PK_2DOT);
   CMAP(0xec, 'i', S_font::PK_LINE_I);
   CMAP(0xed, 'i', S_font::PK_LINE);
   CMAP(0xee, 'i', S_font::PK_HOOK_I);
   CMAP(0xef, 'i', S_font::PK_2DOT);

   CMAP(0xf0, 0xaa, S_font::PK_NO);
   CMAP(0xf1, 'n', S_font::PK_TILDE);
   CMAP(0xf2, 'o', S_font::PK_LINE_I);
   CMAP(0xf3, 'o', S_font::PK_LINE);
   CMAP(0xf4, 'o', S_font::PK_HOOK_I);
   CMAP(0xf5, 'o', S_font::PK_TILDE);
   CMAP(0xf6, 'o', S_font::PK_2DOT);
   CMAP(0xf7, ':', S_font::PK_NO);    //divide
   CMAP(0xf8, 0xa6, S_font::PK_NO);       //strike o
   CMAP(0xf9, 'u', S_font::PK_LINE_I);
   CMAP(0xfa, 'u', S_font::PK_LINE);
   CMAP(0xfb, 'u', S_font::PK_HOOK_I);
   CMAP(0xfc, 'u', S_font::PK_2DOT);
   CMAP(0xfd, 'y', S_font::PK_LINE);
   CMAP(0xfe, 0xa8, S_font::PK_NO);
   CMAP(0xff, 'y', S_font::PK_2DOT);

   CMAP(0x100, 'A', S_font::PK_HLINE);
   CMAP(0x101, 'a', S_font::PK_HLINE);
   CMAP(0x102, 'A', S_font::PK_HOOK);
   CMAP(0x103, 'a', S_font::PK_HOOK);
   CMAP(0x104, 'A', S_font::PK_LOW_HOOK);
   CMAP(0x105, 'a', S_font::PK_LOW_HOOK);
   CMAP(0x106, 'C', S_font::PK_LINE);
   CMAP(0x107, 'c', S_font::PK_LINE);
   CMAP(0x108, 'C', S_font::PK_HOOK_I);
   CMAP(0x109, 'c', S_font::PK_HOOK_I);
   CMAP(0x10a, 'C', S_font::PK_DOT);
   CMAP(0x10b, 'c', S_font::PK_DOT);
   CMAP(0x10c, 'C', S_font::PK_HOOK);
   CMAP(0x10d, 'c', S_font::PK_HOOK);
   CMAP(0x10e, 'D', S_font::PK_HOOK);
   CMAP(0x10f, 'd', S_font::PK_SIDE_HOOK);

   CMAP(0x110, 'D', S_font::PK_NO);
   CMAP(0x111, 'd', S_font::PK_NO);
   CMAP(0x112, 'E', S_font::PK_HLINE);
   CMAP(0x113, 'e', S_font::PK_HLINE);
   CMAP(0x114, 'E', S_font::PK_HOOK);
   CMAP(0x115, 'e', S_font::PK_HOOK);
   CMAP(0x116, 'E', S_font::PK_DOT);
   CMAP(0x117, 'e', S_font::PK_DOT);
   CMAP(0x118, 'E', S_font::PK_LOW_HOOK);
   CMAP(0x119, 'e', S_font::PK_LOW_HOOK);
   CMAP(0x11a, 'E', S_font::PK_HOOK);
   CMAP(0x11b, 'e', S_font::PK_HOOK);
   CMAP(0x11c, 'G', S_font::PK_HOOK_I);
   CMAP(0x11d, 'g', S_font::PK_HOOK_I);
   CMAP(0x11e, 'G', S_font::PK_HOOK);
   CMAP(0x11f, 'g', S_font::PK_HOOK);

   CMAP(0x120, 'G', S_font::PK_DOT);
   CMAP(0x121, 'g', S_font::PK_DOT);
   CMAP(0x122, 'G', S_font::PK_LOW_HOOK);
   CMAP(0x123, 'g', S_font::PK_DOT);
   //CMAP(0x124, 'H', S_font::PK_HOOK_I);
   //CMAP(0x125, 'h', S_font::PK_HOOK_I);
   CMAP(0x126, 'H', S_font::PK_NO);
   CMAP(0x127, 'h', S_font::PK_NO);
   CMAP(0x128, 'I', S_font::PK_TILDE);
   CMAP(0x129, 'i', S_font::PK_TILDE);
   CMAP(0x12a, 'I', S_font::PK_HLINE);
   CMAP(0x12b, 'i'+64, S_font::PK_HLINE);
   CMAP(0x12c, 'I', S_font::PK_HOOK);
   CMAP(0x12d, 'i'+64, S_font::PK_HOOK);
   CMAP(0x12e, 'I', S_font::PK_LOW_HOOK);
   CMAP(0x12f, 'i', S_font::PK_LOW_HOOK);

   CMAP(0x130, 'I', S_font::PK_DOT);
   CMAP(0x131, 'i'+64, S_font::PK_NO);
   //CMAP(0x134, 'J', S_font::PK_HOOK_I);
   //CMAP(0x135, 'j', S_font::PK_HOOK_I);
   CMAP(0x136, 'K', S_font::PK_LOW_HOOK);
   CMAP(0x137, 'k', S_font::PK_LOW_HOOK);
   CMAP(0x138, 'k', S_font::PK_NO);
   CMAP(0x139, 'L', S_font::PK_LINE);
   CMAP(0x13a, 'l', S_font::PK_SIDE_LINE);
   CMAP(0x13b, 'L', S_font::PK_LOW_HOOK);
   CMAP(0x13c, 'l', S_font::PK_LOW_HOOK);
   CMAP(0x13d, 'L', S_font::PK_HOOK);
   CMAP(0x13e, 'l', S_font::PK_SIDE_HOOK);
   CMAP(0x13f, 'L', S_font::PK_NO);

   CMAP(0x140, 'l', S_font::PK_NO);
   CMAP(0x141, 'L'+0x3f, S_font::PK_NO); //polish l, L
   CMAP(0x142, 'l'+0x3f, S_font::PK_NO);
   CMAP(0x143, 'N', S_font::PK_LINE);
   CMAP(0x144, 'n', S_font::PK_LINE);
   CMAP(0x145, 'N', S_font::PK_LOW_HOOK);
   CMAP(0x146, 'n', S_font::PK_LOW_HOOK);
   CMAP(0x147, 'N', S_font::PK_HOOK);
   CMAP(0x148, 'n', S_font::PK_HOOK);
   CMAP(0x14a, 'n', S_font::PK_NO);
   CMAP(0x14b, 'n', S_font::PK_NO);
   CMAP(0x14c, 'O', S_font::PK_HLINE);
   CMAP(0x14d, 'o', S_font::PK_HLINE);
   CMAP(0x14e, 'O', S_font::PK_HOOK);
   CMAP(0x14f, 'o', S_font::PK_HOOK);

   CMAP(0x150, 'O', S_font::PK_2LINES);
   CMAP(0x151, 'o', S_font::PK_2LINES);
   CMAP(0x154, 'R', S_font::PK_LINE);
   CMAP(0x155, 'r', S_font::PK_LINE);
   CMAP(0x156, 'R', S_font::PK_LOW_HOOK);
   CMAP(0x157, 'r', S_font::PK_LOW_HOOK);
   CMAP(0x158, 'R', S_font::PK_HOOK);
   CMAP(0x159, 'r', S_font::PK_HOOK);
   CMAP(0x15a, 'S', S_font::PK_LINE);
   CMAP(0x15b, 's', S_font::PK_LINE);
   CMAP(0x15c, 'S', S_font::PK_HOOK_I);
   CMAP(0x15d, 's', S_font::PK_HOOK_I);
   CMAP(0x15e, 'S', S_font::PK_LOW_HOOK);
   CMAP(0x15f, 's', S_font::PK_LOW_HOOK);

   CMAP(0x160, 'S', S_font::PK_HOOK);
   CMAP(0x161, 's', S_font::PK_HOOK);
   CMAP(0x162, 'T', S_font::PK_LOW_HOOK);
   CMAP(0x163, 't', S_font::PK_LOW_HOOK);
   CMAP(0x164, 'T', S_font::PK_HOOK);
   CMAP(0x165, 't', S_font::PK_SIDE_HOOK);
   CMAP(0x166, 'T', S_font::PK_NO);
   CMAP(0x167, 't', S_font::PK_NO);
   CMAP(0x168, 'U', S_font::PK_TILDE);
   CMAP(0x169, 'u', S_font::PK_TILDE);
   CMAP(0x16a, 'U', S_font::PK_HLINE);
   CMAP(0x16b, 'u', S_font::PK_HLINE);
   CMAP(0x16c, 'U', S_font::PK_HOOK);
   CMAP(0x16d, 'u', S_font::PK_HOOK);
   CMAP(0x16e, 'U', S_font::PK_CIRCLE);
   CMAP(0x16f, 'u', S_font::PK_CIRCLE);

   CMAP(0x170, 'U', S_font::PK_2LINES);
   CMAP(0x171, 'u', S_font::PK_2LINES);
   CMAP(0x172, 'U', S_font::PK_HOOK_I);
   CMAP(0x173, 'u', S_font::PK_HOOK_I);
   //CMAP(0x174, 'W', S_font::PK_HOOK_I);
   //CMAP(0x175, 'w', S_font::PK_HOOK_I);
   CMAP(0x176, 'Y', S_font::PK_HOOK_I);
   CMAP(0x177, 'y', S_font::PK_HOOK_I);
   CMAP(0x178, 'Y', S_font::PK_2DOT);
   CMAP(0x179, 'Z', S_font::PK_LINE);
   CMAP(0x17a, 'z', S_font::PK_LINE);
   CMAP(0x17b, 'Z', S_font::PK_DOT);
   CMAP(0x17c, 'z', S_font::PK_DOT);
   CMAP(0x17d, 'Z', S_font::PK_HOOK);
   CMAP(0x17e, 'z', S_font::PK_HOOK);
   CMAP(0x18f, 0x96, S_font::PK_NO);   //Azeri big e

   CMAP(0x1b0, 'u', S_font::PK_SIDE_HOOK);

   CMAP(0x259, 0x97, S_font::PK_NO);   //Azeri e
   CMAP(0x2c7, ' ', S_font::PK_HOOK);
   CMAP(0x2d9, '.', S_font::PK_NO);
   CMAP(0x2db, ' ', S_font::PK_LOW_HOOK);
   CMAP(0x2dc, '~', S_font::PK_NO);

                              //cyrillic
   CMAP(0x401, 'E', S_font::PK_2DOT);
   CMAP(0x404, 'E', S_font::PK_NO);
   CMAP(0x406, 'I', S_font::PK_NO);
   CMAP(0x407, 'I', S_font::PK_2DOT);
   CMAP(0x40e, 0xd3, S_font::PK_HOOK);

   CMAP(0x410, 'A', S_font::PK_NO);
   CMAP(0x411, 0xc1, S_font::PK_NO);
   CMAP(0x412, 'B', S_font::PK_NO);
   CMAP(0x413, 0xc3, S_font::PK_NO);
   CMAP(0x414, 0xc4, S_font::PK_NO);
   CMAP(0x415, 'E', S_font::PK_NO);
   CMAP(0x416, 0xc6, S_font::PK_NO);
   CMAP(0x417, 0xc7, S_font::PK_NO);
   CMAP(0x418, 0xc8, S_font::PK_NO);
   CMAP(0x419, 0xc9, S_font::PK_NO);
   CMAP(0x41a, 'K', S_font::PK_NO);
   CMAP(0x41b, 0xcb, S_font::PK_NO);
   CMAP(0x41c, 'M', S_font::PK_NO);
   CMAP(0x41d, 'H', S_font::PK_NO);
   CMAP(0x41e, 'O', S_font::PK_NO);
   CMAP(0x41f, 0xcf, S_font::PK_NO);
   CMAP(0x420, 'P', S_font::PK_NO);
   CMAP(0x421, 'C', S_font::PK_NO);
   CMAP(0x422, 'T', S_font::PK_NO);
   CMAP(0x423, 0xd3, S_font::PK_NO);
   CMAP(0x424, 0xd4, S_font::PK_NO);
   CMAP(0x425, 'X', S_font::PK_NO);
   CMAP(0x426, 0xd6, S_font::PK_NO);
   CMAP(0x427, 0xd7, S_font::PK_NO);
   CMAP(0x428, 0xd8, S_font::PK_NO);
   CMAP(0x429, 0xd9, S_font::PK_NO);
   CMAP(0x42a, 0xfa, S_font::PK_NO);
   CMAP(0x42b, 0xfb, S_font::PK_NO);
   CMAP(0x42c, 0xfc, S_font::PK_NO);
   CMAP(0x42d, 0xdd, S_font::PK_NO);
   CMAP(0x42e, 0xde, S_font::PK_NO);
   CMAP(0x42f, 0xdf, S_font::PK_NO);

   CMAP(0x430, 'a', S_font::PK_NO);
   CMAP(0x431, 0xe1, S_font::PK_NO);
   CMAP(0x432, 0xe2, S_font::PK_NO);
   CMAP(0x433, 0xe3, S_font::PK_NO);
   CMAP(0x434, 0xe4, S_font::PK_NO);
   CMAP(0x435, 'e', S_font::PK_NO);
   CMAP(0x436, 0xe6, S_font::PK_NO);
   CMAP(0x437, 0xe7, S_font::PK_NO);
   CMAP(0x438, 0xe8, S_font::PK_NO);
   CMAP(0x439, 0xe9, S_font::PK_NO);
   CMAP(0x43a, 0xea, S_font::PK_NO);
   CMAP(0x43b, 0xeb, S_font::PK_NO);
   CMAP(0x43c, 0xec, S_font::PK_NO);
   CMAP(0x43d, 0xed, S_font::PK_NO);
   CMAP(0x43e, 'o', S_font::PK_NO);
   CMAP(0x43f, 0xef, S_font::PK_NO);
   CMAP(0x440, 'p', S_font::PK_NO);
   CMAP(0x441, 'c', S_font::PK_NO);
   CMAP(0x442, 0xf2, S_font::PK_NO);
   CMAP(0x443, 'y', S_font::PK_NO);
   CMAP(0x444, 0xf4, S_font::PK_NO);
   CMAP(0x445, 'x', S_font::PK_NO);
   CMAP(0x446, 0xf6, S_font::PK_NO);
   CMAP(0x447, 0xf7, S_font::PK_NO);
   CMAP(0x448, 0xf8, S_font::PK_NO);
   CMAP(0x449, 0xf9, S_font::PK_NO);
   CMAP(0x44a, 0xfa, S_font::PK_NO);
   CMAP(0x44b, 0xfb, S_font::PK_NO);
   CMAP(0x44c, 0xfc, S_font::PK_NO);
   CMAP(0x44d, 0xfd, S_font::PK_NO);
   CMAP(0x44e, 0xfe, S_font::PK_NO);
   CMAP(0x44f, 0xff, S_font::PK_NO);
   CMAP(0x451, 'e', S_font::PK_2DOT);
   CMAP(0x454, 'e', S_font::PK_NO);
   CMAP(0x456, 'i', S_font::PK_NO);
   CMAP(0x457, 'i', S_font::PK_2DOT);
   CMAP(0x458, 'j', S_font::PK_NO);
   //CMAP(0x459, '!', S_font::PK_NO); //
   //CMAP(0x45a, '!', S_font::PK_NO); //
   CMAP(0x45b, 'h', S_font::PK_NO);
   CMAP(0x45e, 'y', S_font::PK_HOOK);

   /*
                              //vietnamese (not good mapping)
   CMAP(0x1eb7, 'a', S_font::PK_HOOK);
   CMAP(0x1ec3, 'e', S_font::PK_HOOK_I);
   CMAP(0x1ec7, 'e', S_font::PK_HOOK_I);
   CMAP(0x1ecd, 'o', S_font::PK_NO);
   CMAP(0x1ed1, 'o', S_font::PK_HOOK_I);
   CMAP(0x1ed9, 'o', S_font::PK_HOOK_I);
   CMAP(0x1edf, 'o', S_font::PK_NO);
   CMAP(0x1ee1, 'o', S_font::PK_TILDE);
   CMAP(0x1ee5, 'u', S_font::PK_NO);
   CMAP(0x1ee7, 'u', S_font::PK_NO);
   CMAP(0x1ee9, 'u', S_font::PK_LINE);
   CMAP(0x1eed, 'u', S_font::PK_NO);
   CMAP(0x1eef, 'u', S_font::PK_TILDE);
   */

                           //some exotic chars from 0x20xx - Punctation, Sub/Superscripts, Currency (someone really uses them in text files...)
   CMAP(0x2002, ' ', S_font::PK_NO);
   CMAP(0x2003, ' ', S_font::PK_NO);
   CMAP(0x2009, ' ', S_font::PK_NO);
   CMAP(0x200b, ' ', S_font::PK_NO);  //zero-width space
   CMAP(0x200c, '|', S_font::PK_NO);
   CMAP(0x200d, '|', S_font::PK_NO);
   CMAP(0x2010, '-', S_font::PK_NO);
   CMAP(0x2011, '-', S_font::PK_NO);
   CMAP(0x2012, '-', S_font::PK_NO);
   CMAP(0x2013, '-', S_font::PK_NO);
   CMAP(0x2014, '-', S_font::PK_NO);
   CMAP(0x2018, '\'', S_font::PK_NO);
   CMAP(0x2019, '\'', S_font::PK_NO);
   CMAP(0x201a, ',', S_font::PK_NO);
   CMAP(0x201b, '\'', S_font::PK_NO);
   CMAP(0x201c, '\"', S_font::PK_NO);
   CMAP(0x201d, '\"', S_font::PK_NO);
   CMAP(0x201e, '\"', S_font::PK_NO);
   CMAP(0x2020, '|', S_font::PK_NO);
   CMAP(0x2021, '|', S_font::PK_NO);
   CMAP(0x2022, 192, S_font::PK_NO);
   CMAP(0x2026, 0xbf, S_font::PK_NO);
   CMAP(0x2030, '%', S_font::PK_LOW_HOOK);
   CMAP(0x2039, '<', S_font::PK_NO);
   CMAP(0x203a, '>', S_font::PK_NO);
   CMAP(0x20ac, 0xd5, S_font::PK_NO);  //Euro
   CMAP(0x2122, 0xa0, S_font::PK_NO);  //™private
   CMAP(0x2212, '-', S_font::PK_NO);

                              //Symbian eol indicators
   CMAP(0xe125, 0x80, S_font::PK_NO);
   CMAP(0x21b2, 0x80, S_font::PK_NO);
                              //forced dot
   CMAP(0xffff, 191, S_font::PK_NO);
   }
   if(c >= 127 || c<' '){
      c = '?';
   }
   return c;
}

//----------------------------
#include <ImgLib.h>

void C_application_base::InitInternalFonts(const C_zip_package *dta, int size_delta){

   //LOG_RUN("InitInternalFonts");
   system_font.use = false;
   system_font.Close();
   internal_font.curr_def_font_size = GetDefaultFontSize(false);
   internal_font.curr_relative_size = size_delta;
   int font_size = Max(0, Min(S_internal_font::NUM_INTERNAL_FONTS-2, internal_font.curr_def_font_size+size_delta));

   font_defs = internal_font.all_font_defs + font_size;
   fds = font_defs[UI_FONT_SMALL];
   fdb = font_defs[UI_FONT_BIG];

   internal_font.curr_subpixel_mode = DetermineSubpixelMode();
   internal_font.screen_rotation = GetScreenRotation();
   //const int mode = internal_font.curr_subpixel_mode==FONT_PIXEL_GRAYSCALE ? 1 : 0;

   for(int fi=0; fi<NUM_FONTS; fi++){
      const S_font &fd = font_defs[fi];
      S_internal_font::S_font_data &data = internal_font.font_data[fi];

      Cstr_w fn; fn.Format(L"Font\\#02%x#02%.pcx") <<fd.cell_size_x <<fd.cell_size_y;

      C_file_zip fl;
      if(!fl.Open(fn, dta))
         Fatal("Cannot open font", fi);
      C_image_loader *ldr = CreateImgLoaderPCX(&fl);
      ldr->GetImageInfo(0x0801050a);

      dword n = internal_font.NUM_FONT_LETTETS/8*fd.cell_size_y;
      data.width_empty_bits.Resize(internal_font.NUM_FONT_LETTETS + n);
      byte *letter_widths = data.width_empty_bits.Begin();
      byte *e_bits = letter_widths + internal_font.NUM_FONT_LETTETS;
      const dword src_sx = ldr->GetSX(), src_sy = ldr->GetSY();
      byte *img_data = new(true) byte[src_sx*src_sy];
      for(dword y=0; y<src_sy; y++){
         C_vector<C_image_loader::t_horizontal_line*> lines;
         ldr->ReadOneOrMoreLines(lines);
         assert(lines.size()==1);
         C_image_loader::t_horizontal_line *line = lines.front();
         byte *dp = img_data + src_sx*y;
         for(dword x=0; x<src_sx; x++){
            const C_image_loader::S_rgb &rgb = (*line)[x];
            dp[x] = (rgb.r!=0);
         }
         delete line;
      }
      delete ldr;

      data.italic_add_sx = fd.cell_size_y/4;
      {
         MemSet(e_bits, 0, n);
                              //determine which lines are empty
         for(int li=0; li<internal_font.NUM_FONT_LETTETS; li++){
            const byte *src = img_data;
            src += (fd.cell_size_x * 16 * 3 * fd.cell_size_y * (li/16) + fd.cell_size_x * (li&15) * 3);
            int max_x = 0;
            for(int y=0; y<fd.cell_size_y; y++){
               int x;
               for(x=fd.cell_size_x*3; x--; ){
                  if(!src[x])
                     break;
               }
                              //mark empty line
               if(x==-1)
                  e_bits[(li/8) * fd.cell_size_y + y] |= 1 << (li&7);
               else
                  max_x = Max(max_x, x+1);
               src += src_sx;
            }
            max_x += 2;
            byte &lw = letter_widths[li];
            lw = byte(max_x / 3);
            if(lw){
                              //adjust spacing for pixels covering only 1/3 of pixel
               if((max_x - lw*3) < 1)
                  lw |= 0x80;
                  //--lw;
            }
         }
      }


      for(int vi=0; vi<4; vi++){
         int sx = fd.cell_size_x;
         int sy = fd.cell_size_y;
                              //add pixels for antialiasing and transformations
         bool bold = (vi&1);
         bool italic = (vi&2);
         if(bold)
            sx += fd.bold_width_pix;
         if(italic)
            sx += data.italic_add_sx;
         sx += 2;

         word *compr_data = new(true) word[(sx*sy+1)*internal_font.NUM_FONT_LETTETS];
         dword offs = internal_font.NUM_FONT_LETTETS;

                              //scan all letters
         for(int li=0; li<internal_font.NUM_FONT_LETTETS; li++){
            const byte *src = img_data;
            src += (fd.cell_size_x * 16 * 3 * fd.cell_size_y * (li/16) + fd.cell_size_x * (li&15) * 3);
            const byte *ebits_base = e_bits + (li/8) * fd.cell_size_y;
            const byte ebit = byte(1<<(li&7));

            int sx1 = letter_widths[li] & 0x7f;
                              //skip empty letters
            if(!sx1)
               continue;
            if(bold)
               sx1 += fd.bold_width_pix;
            if(italic)
               sx1 += data.italic_add_sx;
            int write_sx = sx1;
            sx1 += 2;

                              //store offset to compressed font
            assert(offs<0x20000);
            assert(!(offs&1));
            compr_data[li] = word(offs/2);

            for(int y=0; y<sy; y++, src+=src_sx){
                              //if empty line, skip it
               if(ebits_base[y] & ebit)
                  continue;

               const byte *s_line = src;
               int italic_shift = 0;   //italic shift in source pixels
               if(italic){
                  italic_shift = Max(0, (sy-2-y)*3/4);
                  s_line -= italic_shift;
               }
               const int src_sx1 = fd.cell_size_x*3;

                              //line pixels (in RGB)
               dword line[40];
               MemSet(line, 0, sx1*sizeof(dword));

               for(int x=0, ssx = -italic_shift; x<write_sx; x++, s_line += 3, ssx+=3){
                  bool sp[3];
                  for(int i=0; i<3; i++){
                     bool p = (ssx>=-i && ssx<src_sx1-i && s_line[i]==0);
                           //look n pixels back
                     if(bold){
                        for(int bi=1; !p && bi<=fd.bold_width_subpix; bi++){
                           int ii = i-bi;
                           p = (ssx>=-ii && ssx<src_sx1-ii && s_line[ii]==0);
                        }
                     }
                     sp[i] = p;
                  }
                  /*
                  if(bold){
                              //bold text
                     sp[2] = sp[2] || sp[1] || sp[0];
                     sp[1] = sp[1] || sp[0];
                     if(!sp[1] && ssx>=1 && ssx<src_sx1+1)
                        sp[1] = (bytes_per_pixel==2) ? (s_line_w[-1]==0) : !(s_line_d[-1]&0xffffff);
                     if(!sp[0] && ssx>=1 && ssx<src_sx+1){
                        sp[0] = (bytes_per_pixel==2) ? (s_line_w[-1]==0) : !(s_line_d[-1]&0xffffff);
                        if(!sp[0] && ssx>=2 && ssx<src_sx1+2)
                           sp[0] = (bytes_per_pixel==2) ? (s_line_w[-2]==0) : !(s_line_d[-2]&0xffffff);
                     }
                  }
                  */
                  dword *p = line + x + 1;

                  //if(!mode){
                     const dword I0 = 85, I1 = 57, I2 = 28;
                     switch(internal_font.curr_subpixel_mode){
                     case LCD_SUBPIXEL_UNKNOWN:
                        /*
                        if(sp[0]){
                           p[-1] += 0x1c1c1c;
                           p[0] += 0x393939;
                        }
                        if(sp[1]){
                           p[0] += 0x555555;
                        }
                        if(sp[2]){
                           p[0] += 0x393939;
                           p[1] += 0x1c1c1c;
                        }
                        */
                              //sharper font for grayscale
                        if(sp[0]){
                           p[-1] += 0x151515;
                           p[0] += 0x404040;
                        }
                        if(sp[1]){
                           p[0] += 0x555555;
                        }
                        if(sp[2]){
                           p[0] += 0x404040;
                           p[1] += 0x151515;
                        }
                        break;
                     case LCD_SUBPIXEL_RGB:
                        if(sp[0]){
                           p[-1] += I2<<8;
                           p[-1] += I1;
                           p[0] += I0<<16;
                           p[0] += I1<<8;
                           p[0] += I2;
                        }
                        if(sp[1]){
                           p[-1] += I2;
                           p[0] += I1<<16;
                           p[0] += I0<<8;
                           p[0] += I1;
                           p[1] += I2<<16;
                        }
                        if(sp[2]){
                           p[0] += I2<<16;
                           p[0] += I1<<8;
                           p[0] += I0;
                           p[1] += I1<<16;
                           p[1] += I2<<8;
                        }
                        break;
                     case LCD_SUBPIXEL_BGR:
                        if(sp[0]){
                           p[-1] += I2<<8;
                           p[-1] += I1<<16;
                           p[0] += I0;
                           p[0] += I1<<8;
                           p[0] += I2<<16;
                        }
                        if(sp[1]){
                           p[-1] += I2<<16;
                           p[0] += I1;
                           p[0] += I0<<8;
                           p[0] += I1<<16;
                           p[1] += I2;
                        }
                        if(sp[2]){
                           p[0] += I2;
                           p[0] += I1<<8;
                           p[0] += I0<<16;
                           p[1] += I1;
                           p[1] += I2<<8;
                        }
                        break;
                     }
                     /*
                  }else{
                     const dword I0 = 127, I1 = 64;
                     switch(internal_font.curr_subpixel_mode){
                     case LCD_SUBPIXEL_UNKNOWN:
                        if(sp[0]){
                           p[-1] += 0x151515;
                           p[0] += 0x404040;
                        }
                        if(sp[1]){
                           p[0] += 0x555555;
                        }
                        if(sp[2]){
                           p[0] += 0x404040;
                           p[1] += 0x151515;
                        }
                        break;
                     case LCD_SUBPIXEL_RGB:
                        if(sp[0]){
                           p[-1] += I1;   //B
                           p[0] += I0<<16;//R
                           p[0] += I1<<8; //G
                        }
                        if(sp[1]){
                           p[0] += I1<<16;//R
                           p[0] += I0<<8; //G
                           p[0] += I1;    //B
                        }
                        if(sp[2]){
                           p[0] += I1<<8; //G
                           p[0] += I0;    //B
                           p[1] += I1<<16;//R
                        }
                        break;
                     case LCD_SUBPIXEL_BGR:
                        if(sp[0]){
                           p[-1] += I1<<16;
                           p[0] += I0;
                           p[0] += I1<<8;
                        }
                        if(sp[1]){
                           p[0] += I1;
                           p[0] += I0<<8;
                           p[0] += I1<<16;
                        }
                        if(sp[2]){
                           p[0] += I1<<8; //G
                           p[0] += I0<<16;
                           p[1] += I1;
                        }
                        break;
                     }
                  }
                  */
               }
                              //convert to 16-bit 565 RGB format
               for(int xx=0; xx<sx1; xx++){
                  dword p = line[xx];
                  if(p){
                     dword r = (p >> 8) & 0xf800;
                     dword g = (p >> 5) & 0x07c0;
                     dword b = (p >> 3) & 0x003f;
                     p = r | g | b;
                  }
                  compr_data[offs++] = word(p);
               }
            }
                              //align offs to 2
            offs = (offs+1) & -2;
         }
         data.compr_data[vi].Resize(offs);
         MemCpy(data.compr_data[vi].Begin(), compr_data, offs*sizeof(word));
         delete[] compr_data;
      }
      delete[] img_data;
   }
}

//----------------------------

void C_application_base::RenderLetter(const S_internal_font::S_render_letter &rl, byte *dst, dword letter_index, int x, int y) const{

   const S_pixelformat &pf = GetPixelFormat();
   const dword screen_pitch = GetScreenPitch();
   const dword pixel_format_bits = pf.bits_per_pixel;

   const word *src = rl.font_src + rl.font_src[letter_index]*2;

   const byte *ebits_base = rl.width_empty_bits + internal_font.NUM_FONT_LETTETS + (letter_index/8) * rl.letter_size_y;
   const byte empty_bit = byte(1 << (letter_index&7));
   int letter_width  = (rl.width_empty_bits[letter_index]&0x7f) + 2;
   if(rl.font_flags&FF_BOLD)
      letter_width += rl.letter_bold_width;
   if(rl.font_flags&FF_ITALIC)
      letter_width += rl.font_data->italic_add_sx;
   int real_size_x = letter_width;

   dword dst_pitch_x, dst_pitch_y;

   const S_rect &curr_clip_rect = GetClipRect();
   int clip_sx = curr_clip_rect.sx, clip_sy = curr_clip_rect.sy;
   switch(rl.rotation){
   default: assert(0);
   case 0:
      dst_pitch_x = pf.bytes_per_pixel;
      dst_pitch_y = screen_pitch;
      dst += y*dst_pitch_y + x*pf.bytes_per_pixel;
      break;
   case 1:                    //90 CCW
      dst_pitch_x = -int(screen_pitch);
      dst_pitch_y = pf.bytes_per_pixel;
      dst += (clip_sy-x-1)*screen_pitch + y*pf.bytes_per_pixel;
      Swap(clip_sx, clip_sy);
      break;
   case 2:                    //180
      dst_pitch_x = -int(pf.bytes_per_pixel);
      dst_pitch_y = -int(screen_pitch);
      dst += (clip_sy-y-1)*screen_pitch + (clip_sx-x-1)*pf.bytes_per_pixel;
      break;
   case 3:                   //90 CW
      dst_pitch_x = screen_pitch;
      dst_pitch_y = -int(pf.bytes_per_pixel);
      dst += x*screen_pitch + (clip_sx-y-1)*pf.bytes_per_pixel;
      Swap(clip_sx, clip_sy);
      break;
   }
                           //clip and render
   if(x<0){
      if((real_size_x += x) <= 0)
         return;
      src -= x;
      dst -= x*dst_pitch_x;
      x = 0;
   }
   int lr = x+real_size_x - clip_sx;
   if(lr>0 && (real_size_x -= lr) <= 0)
      return;

   int top_clip = 0;
   int real_size_y = rl.letter_size_y;
   if(y<0){
      if((real_size_y += y) <= 0)
         return;
      dst -= y*dst_pitch_y;
      top_clip = -y;
                              //clip source (only non-empty lines)
      do{
         if(!(*ebits_base & empty_bit))
            src += letter_width;
         ++ebits_base;
      } while(++y < 0);
   }
   int lb = y+real_size_y - clip_sy;
   if(lb>0 && (real_size_y -= lb) <= 0)
      return;

   int t_r = (rl.text_color>>16) & 0xff;
   int t_g = (rl.text_color>>8) & 0xff;
   int t_b = rl.text_color & 0xff;
   int t_a = (rl.text_color>>24) & 0xff;
   if(!t_a){
      assert(0);
      //t_a = 0xff;
      return;
   }
   if(t_a==0xff){
      t_a = 0x100;
   }else{
      t_r = (t_r*t_a)>>8;
      t_g = (t_g*t_a)>>8;
      t_b = (t_b*t_a)>>8;
   }

   for(int yy=real_size_y; yy--; dst += dst_pitch_y, ++ebits_base){
                              //skip empty lines
      if(*ebits_base & empty_bit)
         continue;

      const word *sp = src;
      byte *dp = dst;
      for(int x1=0; x1<real_size_x; x1++, dp+=dst_pitch_x){
         dword p = *sp++;
         if(!p)
            continue;
         int p_r = (p>>8) & 0xf8;
         int p_g = (p>>3) & 0xfc;
         int p_b = (p<<3) & 0xf8;
         if(rl.physical_rotation){
            if(rl.physical_rotation&1){
                              //remove sub-pixel data for 90deg rotation
               p_r = p_g = p_b = (p_r*77+p_g*154+p_b*25)>>8;
            }else{
                              //180deg rogation, swap R and B
               Swap(p_r, p_b);
            }
         }
                              //multiply pixel intensity by text color
         int r = (p_r * t_r) >> 8;
         int g = (p_g * t_g) >> 8;
         int b = (p_b * t_b) >> 8;

         if(p!=0xffff || t_a!=0x100){
                              //semi-transparent pixel, blend with background
            int bb_r, bb_g, bb_b;
            switch(pixel_format_bits){
#ifdef SUPPORT_12BIT_PIXEL
            case 12:
               bb_b = *(word*)dp;
               bb_r = (bb_b>>4) & 0xf0;
               bb_g = bb_b & 0xf0;
               bb_b = (bb_b<<4) & 0xf0;
               break;
#endif
            case 16:
               bb_b = *(word*)dp;
               bb_r = (bb_b>>8) & 0xf8;
               bb_g = (bb_b>>3) & 0xfc;
               bb_b = (bb_b<<3) & 0xf8;
               break;
            case 32:
               bb_b = *(dword*)dp;
               bb_r = (bb_b>>16) & 0xff;
               bb_g = (bb_b>>8) & 0xff;
               bb_b = bb_b & 0xff;
               break;
            default:
               assert(0);
               bb_r = bb_g = bb_b = 0;
            }
            if(t_a==0x100){
               r += ((255-p_r) * bb_r) >> 8;
               g += ((255-p_g) * bb_g) >> 8;
               b += ((255-p_b) * bb_b) >> 8;
            }else{
               r += ((0x10000-(p_r*t_a)) * bb_r) >> 16;
               g += ((0x10000-(p_g*t_a)) * bb_g) >> 16;
               b += ((0x10000-(p_b*t_a)) * bb_b) >> 16;
            }
            assert(r>=0 && r<=255 && g>=0 && g<=255 && b>=0 && b<=255);
         }
         switch(pixel_format_bits){
#ifdef SUPPORT_12BIT_PIXEL
         case 12:
            *(word*)dp = word(((r&0xf0)<<4) | (g&0xf0) | (b>>4));
            break;
#endif
         case 16:
            *(word*)dp = word(((r&0xf8)<<8) | ((g&0xfc)<<3) | (b>>3));
            break;
         case 32:
            *(dword*)dp = ((r<<16) | (g<<8) | b);
            break;
         default:
            assert(0);
         }
      }
      src += letter_width;
   }
}


#endif   //!USE_SYSTEM_FONT
//----------------------------

int C_application_base::GetDefaultFontSize(bool use_system_font) const{

   dword sz;
#if defined S60 && defined __SYMBIAN_3RD__
   if(system::GetDeviceId()==0x20002496)   //fast fix for E90, 2 screens with same dpi but different resolution, should have same font size
      return use_system_font ? 5 : 2;
#endif
   S_point fsz = GetFullScreenSize();
   dword screen_area = fsz.x*fsz.y;
   if(use_system_font){
      const int DIV = 38000;
      sz = (short)Min(NUM_SYSTEM_FONTS, (int(screen_area)+DIV/2)/DIV);
   }else{
      const int DIV = 85000;
      sz = (short)Min((dword)S_internal_font::NUM_INTERNAL_FONTS-2, (screen_area+DIV/2)/DIV);
   }
   return sz;
}

//----------------------------

void C_application_base::DrawLetter(int x, int y, wchar c, dword curr_font, dword font_flags, dword text_color){

   bool underline = (font_flags&FF_UNDERLINE);
   bool fill_bg = (font_flags&FF_ACTIVE_HYPERLINK);

   if((c&0xff7f)==' '){
#ifdef DEBUG_DISPLAY_SPACE
      c = '.';
      y -= 2;
#else
      if(!underline && !fill_bg)
         return;
#endif
   }

   const S_font &fd = font_defs[curr_font];
   if(system_font.use){
      RenderSystemChar(x, y, c, curr_font, font_flags, text_color);
      return;
   }

#ifndef USE_SYSTEM_FONT
   bool bold = (font_flags&FF_BOLD);
   bool italic = (font_flags&FF_ITALIC);

   dword rotation = font_flags>>FF_ROTATE_SHIFT;

   S_font::E_PUNKTATION punkt;
   dword char_map = S_internal_font::GetCharMapping(c, punkt);
   c = wchar(char_map);

   if(char_map<' ' || char_map>=internal_font.NUM_FONT_LETTETS+32)
      char_map = c = '.';
   dword orig_char_map = char_map;
                              //letters with punktation - change to smaller version
   if(punkt && punkt!=S_font::PK_LOW_HOOK){
      if((char_map>='A' && char_map<='Z')
         //|| (char_map>='a' && char_map<='z')
         ){
                              //trade capital ascended chars for shrinked version
         char_map += 64;
      }else
      if(char_map=='i')
         char_map += 64;
   }
                              //make index to font
   dword letter_index = char_map - ' ';
   assert(int(letter_index) < internal_font.NUM_FONT_LETTETS);

   const S_internal_font::S_font_data &fdata = internal_font.font_data[curr_font];

   S_internal_font::S_render_letter rl = {
      fdata.compr_data[(bold ? 1 : 0) + (italic ? 2 : 0)].Begin(),
      fdata.width_empty_bits.Begin(),
      text_color,
      0xffffff,
      fd.cell_size_x, fd.cell_size_y, fd.bold_width_pix,
      font_flags,
      rotation, (rotation+internal_font.screen_rotation)&3,
      &fdata
   };
   int letter_width = ((c&0xff7f)==' ') ? fd.space_width : (rl.width_empty_bits[letter_index]&0x7f);
                              //adjust width based on parameters, account for font filtering
   --x;
   ++y;
   letter_width += 2;
   if(bold)
      letter_width += fd.bold_width_pix;
   if(italic)
      letter_width += fdata.italic_add_sx;

   const S_pixelformat &pf = GetPixelFormat();
   byte *scrn_dst = (byte*)GetBackBuffer();
   const dword screen_pitch = GetScreenPitch();
   assert(scrn_dst);

                              //adjust for clipping by defined clipping rect
   const S_rect &curr_clip_rect = GetClipRect();

   scrn_dst += curr_clip_rect.y*screen_pitch + curr_clip_rect.x*pf.bytes_per_pixel;
   x -= curr_clip_rect.x;
   y -= curr_clip_rect.y;

   if(fill_bg){
                              //fill background by text color
      dword pix = GetPixel(rl.text_color);
      rl.text_color ^= 0xffffff;
      //dword pix = igraph->GetRGBConv()->GetPixel(0xff0000);

      int ly = y-1, sy = fd.cell_size_y;
      if(ly < 0){
         sy += ly;
         ly = 0;
      }
      if(ly < curr_clip_rect.sy && sy > 0){
         if(ly+sy > curr_clip_rect.sy)
            sy = curr_clip_rect.sy - ly;
         int lx = x, sx = letter_width+1;
         if(lx < 0){
            sx += lx;
            lx = 0;
         }
         if(lx < curr_clip_rect.sx && sx > 0){
            if(lx+sx > curr_clip_rect.sx)
               sx = curr_clip_rect.sx - lx;
            byte *dst = scrn_dst + ly*screen_pitch + lx*pf.bytes_per_pixel;
            while(sy--){
               byte *line = dst;
               switch(pf.bytes_per_pixel){
               case 2:
                  for(int x1=sx; x1--; ){
                     *(word*)line = word(pix);
                     line += 2;
                  }
                  break;
               case 4:
                  for(int x1=sx; x1--; ){
                     *(dword*)line = pix;
                     line += 4;
                  }
                  break;
               }
               dst += screen_pitch;
            }
         }
      }
   }
   if(underline){
      dword pix = GetPixel(rl.text_color);
                              //draw line
      int ly = y + fd.cell_size_y - 1;
      if(ly>=0 && ly<curr_clip_rect.sy){
         int sx = letter_width+1;
         int lx = x;
         if(lx < 0){
            sx += lx;
            lx = 0;
         }
         if(lx < curr_clip_rect.sx && sx > 0){
            if(lx+sx > curr_clip_rect.sx)
               sx = curr_clip_rect.sx - lx;
            byte *dst = scrn_dst + ly*screen_pitch + lx*pf.bytes_per_pixel;
            switch(pf.bytes_per_pixel){
            case 2:
               while(sx--){
                  *(word*)dst = word(pix);
                  dst += 2;
               }
               break;
            case 4:
               while(sx--){
                  *(dword*)dst = pix;
                  dst += 4;
               }
               break;
            }
         }
      }
   }
   RenderLetter(rl, scrn_dst, letter_index, x, y);
   if(punkt){
      dword lw = rl.width_empty_bits[orig_char_map-' '];
      int punkt_letter_width = lw & 0x7f;
      int dx = punkt_letter_width/2;
      dx -= fd.punkt_center[punkt];

      int dy = 1;
      switch(punkt){
      case S_font::PK_LOW_HOOK:
         ++dy;
         break;
      case S_font::PK_SIDE_HOOK:
         dx = punkt_letter_width;
         if(!(lw&0x80))
            ++dx;
         dy = 0;
         break;
      case S_font::PK_SIDE_LINE:
         {
            byte pc = punkt_char_code[punkt];
            dx = punkt_letter_width;
            if(!(lw&0x80))
               ++dx;
            dy = 0;
            const byte *ebits_base = fdata.width_empty_bits.Begin() + internal_font.NUM_FONT_LETTETS + (pc/8) * fd.cell_size_y;
            const byte empty_bit = byte(1 << (pc&7));
            for(int y1=fd.cell_size_y; y1--; ebits_base++){
               if(!(*ebits_base&empty_bit))
                  break;
               --dy;
            }
         }
         break;
      default:
         {
                                 //determine letter's height
            const byte *ebits_base = fdata.width_empty_bits.Begin() + internal_font.NUM_FONT_LETTETS + (letter_index/8) * fd.cell_size_y;
            const byte empty_bit = byte(1 << (letter_index&7));
            int h = fd.cell_size_y;
            for(int y1=fd.cell_size_y; y1--; ebits_base++){
               if(!(*ebits_base&empty_bit))
                  break;
               --h;
            }
            /*
            const S_letter &let = fd.letter_info[orig_char_map-' '];
            int h = let.punkt_height;
            if(!h)
               h = (c>='A' && c<='Z') ? fd.default_up_punkt_height : fd.default_lo_punkt_height;
            */
            dy -= h;
                                 //adjust italic shift
            if(italic)
               dx += (h+2) / 4;
         }
      }
      x += dx;
      y += dy;
      RenderLetter(rl, scrn_dst, punkt_char_code[punkt], x, y);
   }
#endif
}

//----------------------------

int C_application_base::GetCharWidth(wchar c, bool bold, bool italic, dword font_index) const{

   const S_font &fd = font_defs[font_index];
   int letter_width;
   if(system_font.use){
      letter_width = system_font.GetCharWidth(wchar(c), bold, italic, font_index);
   }else
#ifndef USE_SYSTEM_FONT
   {
      if((c&0xff7f)==' ')
         letter_width = fd.space_width;
      else{
         S_font::E_PUNKTATION punkt;
         c = (wchar)S_internal_font::GetCharMapping(c, punkt);
         const byte *width_empty_bits = internal_font.font_data[font_index].width_empty_bits.Begin();
         letter_width = width_empty_bits[c-' '];
         if(letter_width&0x80){
                              //logical width is 1 pixel less than drawn width
            letter_width = (letter_width&0x7f) - 1;
         }
         if(punkt){
                              //check special punktations affecting char width
            switch(c){
            case 'l':
            case 'd':
            case 't':
               switch(punkt){
               case S_font::PK_SIDE_HOOK:
               case S_font::PK_SIDE_LINE:
                              //add punctation width to letter width
                  letter_width += (width_empty_bits[punkt_char_code[punkt]]&0x7f) + 1;
                  break;
               }
               break;
            case 'i':
               if(punkt==S_font::PK_LINE)
                  letter_width = width_empty_bits[punkt_char_code[punkt]]&0x7f;
               break;
            }
         }
      }
   }
#else
   letter_width = system_font.GetCharWidth(wchar(c), bold, italic, font_index);
#endif
   if(bold)
      letter_width += fd.bold_width_add;
   return letter_width;
}

//----------------------------

void C_application_base::DrawEtchedFrame(const S_rect &rc){

   const dword c0 = GetColor(COL_SHADOW), c1 = GetColor(COL_HIGHLIGHT);
   DrawOutline(rc, c0, c1);
   {
      S_rect rc1 = rc;
      rc1.Expand();
      DrawOutline(rc1, c1, c0);
   }
}

//----------------------------

void C_application_base::ToggleUseClientRect(){

   dword flags = ig_flags;
   flags &= IG_SCREEN_USE_CLIENT_RECT;
   flags ^= IG_SCREEN_USE_CLIENT_RECT;
   UpdateGraphicsFlags(flags, IG_SCREEN_USE_CLIENT_RECT);
}

//----------------------------

void C_application_base::ScreenReinit(bool resized){

                              //set clip rect directly, calling ResetClipRect would use gc and may lead to crash
   //ResetClipRect();
   clip_rect = S_rect(0, 0, ScrnSX(), ScrnSY());
   if(resized)
      InitAfterScreenResize();
   else if(system_font.use){
                              //need to reinit system font
      C_application_base::InitAfterScreenResize();
   }
}

//----------------------------

void C_application_base::InitAfterScreenResize(){

   //LOG_RUN("C_application_base::InitAfterScreenResize");
                              //reinit font
                              //set defaults
   int relative_size = 0, small_font_size_percent = 82;
   bool use_system_font = true, antialias = true;

                              //now as application
   GetFontPreference(relative_size, use_system_font, antialias, small_font_size_percent);
#ifndef USE_SYSTEM_FONT
   if(!use_system_font){
                              //reinit internal font only if default font size changed, or subpixel mode changed
      if(system_font.use || internal_font.curr_subpixel_mode!=DetermineSubpixelMode() || internal_font.curr_def_font_size!=GetDefaultFontSize(false) || internal_font.curr_relative_size!=relative_size){
         InitInternalFonts(CreateDtaFile(), relative_size);
      }
   }else
#endif
   {
      InitSystemFont(relative_size, antialias, small_font_size_percent);
   }
}

//----------------------------

IGraph *IGraphCreate(class C_application_base *app, dword flags = 0);

void C_application_base::InitGraphics(dword _ig_flags){

   LOG_RUN("InitGraphics");
   igraph = IGraphCreate(this, _ig_flags);
   LOG_RUN("InitGraphics OK");
   ResetClipRect();
                              //finish application graphics initialization by this call
   InitAfterScreenResize();
   LOG_RUN(" InitGraphics OK");
}

//----------------------------
