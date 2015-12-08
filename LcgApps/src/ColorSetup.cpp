#include "Main.h"
#include "ColorSetup.h"

//----------------------------

static dword MakeColorFromHue(int hue, int width = 256){
   const int sx3 = (width+1)/3, sx6 = (width+3)/6;
   int r, g, b;
   if(hue < sx3){
      if(hue<sx6){
         r = 255;
         g = Min(255, hue*256/sx6);
      }else{
         r = Min(255, (sx3-hue)*256/sx6);
         g = 255;
      }
      b = 0;
   }else
   if(hue < sx3*2){
      hue -= sx3;
      if(hue<sx6){
         g = 255;
         b = Min(255, hue*256/sx6);
      }else{
         g = Min(255, (sx3-hue)*256/sx6);
         b = 255;
      }
      r = 0;
   }else{
      hue -= sx3*2;
      if(hue<sx6){
         b = 255;
         r = Min(255, hue*256/sx6);
      }else{
         b = Max(0, Min(255, (sx3-hue)*256/sx6));
         r = 255;
      }
      g = 0;
   }
   return (r<<16) | (g<<8) | b;
}

//----------------------------

static dword ApplyBrighness(dword color, int pos, int width = 256){

   int mult = Min(0xffff, ((width-pos)<<16)/width);
   for(int i=0; i<3; i++){
      int c = (color>>(8*i))&0xff;
      c = 255-(((255-c)*mult)>>16);
      color &= ~(255<<(8*i));
      color |= c<<(8*i);
   }
   return color;
}

//----------------------------

static dword ApplySaturation(dword color, int pos, int width = 255){

   int r = (color>>16)&0xff;
   int g = (color>>8)&0xff;
   int b = color&0xff;
   //dword median = (g*153 + r*77 + b*25)>>8;
   dword median = (g + r + b)/3;
   for(int i=0; i<3; i++){
      int c = (color>>(8*i))&0xff;
      c = (c*pos + median*(width-pos)) / width;
      color &= ~(255<<(8*i));
      color |= c<<(8*i);
   }
   return color;
}

//----------------------------

dword C_client_color_setup::HsbToRgb(S_hsb_color hsb){

   dword col = MakeColorFromHue(hsb.hue);
   col = ApplyBrighness(col, hsb.brightness);
   col = ApplySaturation(col, hsb.saturation);
   return col | 0xff000000;
}

//----------------------------

static void DrawArrow16(word *dst, dword pitch, int x, int y, int size){

   dst += y*pitch + x;
   for(int i=0; i<size; i++){
      if(i==size-1){
         for(int j=i; j--; ){
            dst[j] = 0;
            dst[-j] = 0;
         }
      }else{
         dst[i] = 0;
         dst[-i] = 0;
         for(int j=i; j--; ){
            dst[j] = 0xffff;
            dst[-j] = 0xffff;
         }
      }
      dst += pitch;
   }
}

//----------------------------

static void DrawArrow32(dword *dst, dword pitch, int x, int y, int size){

   dst += y*pitch + x;
   for(int i=0; i<size; i++){
      if(i==size-1){
         for(int j=i; j--; ){
            dst[j] = 0;
            dst[-j] = 0;
         }
      }else{
         dst[i] = 0;
         dst[-i] = 0;
         for(int j=i; j--; ){
            dst[j] = 0xffffffff;
            dst[-j] = 0xffffffff;
         }
      }
      dst += pitch;
   }
}

//----------------------------

void C_client_color_setup::SetMode(S_hsb_color *cols, dword nc, const S_color_preset *presets, dword num_presets, t_ApplyColor ac){

   C_mode_this &mod = *new(true) C_mode_this(*this, mode, cols, nc, presets, num_presets, ac);
   CreateTimer(mod, 20);
   InitLayout(mod);
   EditColor(mod, COL_BACKGROUND);
   ActivateMode(mod);
}

//----------------------------
const int NUM_LINES =
#if defined USE_SYSTEM_SKIN || defined USE_OWN_SKIN
   5;
#else
   4;
#endif

void C_client_color_setup::InitLayout(C_mode_this &mod){

   int sx = ScrnSX();
   mod.rc = S_rect(fdb.cell_size_x, ScrnSY(), sx-fdb.cell_size_x*2, NUM_LINES*(fdb.line_spacing*2)+fdb.line_spacing);
   mod.rc.y -= mod.rc.sy+GetSoftButtonBarHeight()+fdb.line_spacing;

   C_scrollbar &sb = mod.sb;
   sb.visible_space = NUM_LINES;
   const int width = GetScrollbarWidth();
   sb.rc = S_rect(mod.rc.Right()-width-3, mod.rc.y+3, width, mod.rc.sy-6);
   sb.total_space = NUM_LINES+1;
   sb.visible = true;
}

//----------------------------

void C_client_color_setup::EditColor(C_mode_this &mod, E_COLOR col){

   mod.color = mod.colors[col];
   mod.base_for_br = MakeColorFromHue(mod.color.hue);
   mod.base_for_sat = ApplyBrighness(mod.base_for_br, mod.color.brightness);
}

//----------------------------

void C_client_color_setup::ProcessMenu(C_mode_this &mod, int itm, dword menu_id){

   switch(itm){
   case TXT_COL_PRESET:
      mod.menu = mod.CreateMenu();
      for(dword i=0; i<mod.num_presets; i++)
         mod.menu->AddItem(mod.presets[i].name);
      PrepareMenu(mod.menu);
      break;
   case TXT_BACK:
      if(mod.need_save)
         SaveConfig();
      CloseMode(mod);
      return;
   default:
      if(itm!=-1){
         dword pi = itm-0x10000;
         assert(pi<mod.num_presets);
         for(dword i=0; i<mod.num_colors; i++){
            mod.colors[i] = mod.presets[pi].colors[i];
            (this->*mod.ApplyColor)((E_COLOR)i);
         }
         EditColor(mod, (E_COLOR)mod.item);
         mod.need_save = true;
      }
   }
}

//----------------------------

void C_client_color_setup::ProcessInput(C_mode_this &mod, S_user_input &ui, bool &redraw){

#ifdef USE_MOUSE
   if(!ProcessMouseInSoftButtons(ui, redraw)){
      if((ui.mouse_buttons&(MOUSE_BUTTON_1_DOWN|MOUSE_BUTTON_1_DRAG)) && ui.CheckMouseInRect(mod.rc)){
         dword sx = ScrnSX();
         const int max_x = (mod.sb.visible ? mod.sb.rc.x : sx)-fdb.cell_size_x/2;
         const int width = max_x-mod.rc.x-fdb.cell_size_x*2;
         if(mod.curr_drag_item==-1 && (ui.mouse_buttons&MOUSE_BUTTON_1_DOWN)){
            S_rect rc(mod.rc.x, mod.rc.y+fdb.line_spacing/2, mod.rc.sx, fdb.line_spacing*2);
            for(int i=0; i<NUM_LINES; i++){
               if(ui.CheckMouseInRect(rc)){
                  redraw = true;
                  int sel = i;
#if defined USE_SYSTEM_SKIN || defined USE_OWN_SKIN
                  if(sel && !config.color_theme)
                     break;
                  --sel;
#endif
                  switch(sel){
                  case -1:
                     mod.selection = 0;
                     if(ui.mouse_buttons&MOUSE_BUTTON_1_DOWN)
                        ui.key = K_ENTER;
                     break;
                  case 0:
                     if(mod.selection != i){
                        mod.selection = i;
                        redraw = true;
                     }
                     ui.key = (ui.mouse.x<mod.rc.CenterX()) ? K_CURSORLEFT : K_CURSORRIGHT;
                     break;
                  default:
                     mod.selection = i;
                     mod.curr_drag_item = sel;
                              //immediately skip to clicked position (do it by simulating drag)
                     byte val = sel==1 ? mod.color.hue : sel==2 ? mod.color.brightness : mod.color.saturation;
                     mod.drag_mouse_x_x8 = (mod.rc.x + fdb.cell_size_x + val*width/256)<<8;
                     ui.mouse_buttons |= MOUSE_BUTTON_1_DRAG;
                  }
                  break;
               }
               rc.y += fdb.line_spacing * 2;
            }
         }
         if(mod.curr_drag_item!=-1 && (ui.mouse_buttons&MOUSE_BUTTON_1_DRAG)){
            byte &val = mod.curr_drag_item==1 ? mod.color.hue : mod.curr_drag_item==2 ? mod.color.brightness : mod.color.saturation;
            int v8 = (ui.mouse.x<<8)-mod.drag_mouse_x_x8;
            int delta = v8/width;
            mod.drag_mouse_x_x8 = (ui.mouse.x<<8) - (v8-delta*width);
            byte nv = (byte)Max(0, Min(255, int(val)+delta));
            if(val!=nv){
               val = nv;
               switch(mod.curr_drag_item){
               case 1:
                  mod.base_for_br = MakeColorFromHue(mod.color.hue);
                  mod.base_for_sat = ApplyBrighness(mod.base_for_br, mod.color.brightness);
                  break;
               case 2:
                  mod.base_for_sat = ApplyBrighness(mod.base_for_br, mod.color.brightness);
                  break;
               }
               mod.colors[mod.item] = mod.color;
               (this->*mod.ApplyColor)((E_COLOR)mod.item);
               mod.need_save = true;
               redraw = true;
            }
         }
      }
      if(ui.mouse_buttons&MOUSE_BUTTON_1_UP)
         mod.curr_drag_item = -1;
   }
#endif

   switch(ui.key){
   case K_ESC:
   case K_RIGHT_SOFT:
   case K_BACK:
      if(mod.need_save)
         SaveConfig();
      CloseMode(mod);
      return;

   case K_ENTER:
#if defined USE_SYSTEM_SKIN || defined USE_OWN_SKIN
      if(!mod.selection){
         config.color_theme = !config.color_theme;
         EnableSkinsByConfig();
         SaveConfig();
         redraw = true;
         break;
      }
                              //flow...
#endif

   case K_LEFT_SOFT:
   case K_MENU:
      mod.menu = mod.CreateMenu();
      if(mod.num_presets){
         bool en = true;
#if defined USE_SYSTEM_SKIN || defined USE_OWN_SKIN
         if(!config.color_theme)
            en = false;
#endif
         mod.menu->AddItem(TXT_COL_PRESET, C_menu::HAS_SUBMENU | (en ? 0 : C_menu::DISABLED));
      }
      mod.menu->AddSeparator();
      mod.menu->AddItem(TXT_BACK);
      PrepareMenu(mod.menu);
      break;

   case K_CURSORDOWN:
   case K_CURSORUP:
#if defined USE_SYSTEM_SKIN || defined USE_OWN_SKIN
      if(!config.color_theme)
         break;
#endif
      if(ui.key==K_CURSORUP){
         if(!mod.selection)
            mod.selection = NUM_LINES;
         --mod.selection;
      }else{
         if(++mod.selection==NUM_LINES)
            mod.selection = 0;
      }
      redraw = true;
      break;

   case K_CURSORLEFT:
   case K_CURSORRIGHT:
      {
         int selection = mod.selection;
#if defined USE_SYSTEM_SKIN || defined USE_OWN_SKIN
         --selection;
#endif
         if(!selection){
            int ns = mod.item;
            if(ui.key==K_CURSORLEFT)
               ns = Max(0, ns-1);
            else
               ns = Min(int(mod.num_colors-1), ns+1);
            if(mod.item!=ns){
               mod.item = ns;
               EditColor(mod, (E_COLOR)mod.item);
               redraw = true;
            }
         }
      }
      break;
   }
   mod.key_bits = ui.key_bits;
}

//----------------------------

void C_client_color_setup::Tick(C_mode_this &mod, dword time, bool &redraw){

   int selection = mod.selection;
#if defined USE_SYSTEM_SKIN || defined USE_OWN_SKIN
   --selection;
#endif
   if(selection){
      const int MINS = 50, MAXS = 200, ACC = 10;
      int move = mod.acc[0].GetValue((mod.key_bits&GKEY_RIGHT), time, MINS, MAXS, ACC);
      move -= mod.acc[1].GetValue((mod.key_bits&GKEY_LEFT), time, MINS, MAXS, ACC);
      bool change = false;
      if(move)
      switch(selection){
      case 1:
         {
            mod.color.hue = byte((mod.color.hue + move)&0xff);
            if(mod.color.hue<0)
               mod.color.hue += 255;
            if(mod.color.hue>255)
               mod.color.hue -= 255;
            mod.base_for_br = MakeColorFromHue(mod.color.hue);
            mod.base_for_sat = ApplyBrighness(mod.base_for_br, mod.color.brightness);
            change = true;
         }
         break;
      case 2:
         {
            int ns = Max(0, Min(255, mod.color.brightness+move));
            if(mod.color.brightness!=ns){
               mod.color.brightness = byte(ns);
               mod.base_for_sat = ApplyBrighness(mod.base_for_br, mod.color.brightness);
               change = true;
            }
         }
         break;
      case 3:
         {
            int ns = Max(0, Min(255, mod.color.saturation+move));
            if(mod.color.saturation!=ns){
               mod.color.saturation = byte(ns);
               change = true;
            }
         }
         break;
      }
      if(change){
         mod.colors[mod.item] = mod.color;
         (this->*mod.ApplyColor)((E_COLOR)mod.item);
         mod.need_save = true;
         redraw = true;
      }
   }
}

//----------------------------

void C_client_color_setup::Draw(const C_mode_this &mod){

   dword col_text = GetColor(COL_TEXT_POPUP);
   const dword sx = ScrnSX();
   ClearToBackground(S_rect(0, 0, sx, ScrnSY()));
   dword col_rc = GetColor(COL_AREA);

   DrawTitleBar(GetText(TXT_COLOR_SETUP));

   DrawDialogBase(mod.rc, false);
                              //draw all items
   int max_x = (mod.sb.visible ? mod.sb.rc.x : sx)-fdb.cell_size_x/2;
   int x = mod.rc.x;
   int y = mod.rc.y+fdb.line_spacing/2;
   for(int i=0; i<NUM_LINES; i++){
      if(i==mod.selection){
         S_rect rc(x+fdb.cell_size_x/2, y, max_x-mod.rc.x-fdb.cell_size_x, fdb.line_spacing*2-1);
         DrawSelection(rc);
      }
      int selection = i;
#if defined USE_SYSTEM_SKIN || defined USE_OWN_SKIN
      --selection;
#endif

      if(selection==1 && mod.selection>=2)
         DrawHorizontalLine(x+fdb.letter_size_x, y-1, max_x-x-fdb.letter_size_x*2, MulColor(col_rc, 0xe000));

                              //title
      E_TEXT_ID title;
      switch(selection){
      default:
      case 0: title = TXT_COL_ITEM; break;
      case 1: title = TXT_COL_HUE; break;
      case 2: title = TXT_COL_BRIGHTNESS; break;
      case 3: title = TXT_COL_SATURATION; break;
#if defined USE_SYSTEM_SKIN || defined USE_OWN_SKIN
      case -1: title = TXT_USE_SYSTEM_THEME; break;
#endif
      }
      DrawString(GetText(title), x+fdb.cell_size_x*3/4, y, UI_FONT_BIG, 0, col_text);

                              //item
      switch(selection){
#if defined USE_SYSTEM_SKIN || defined USE_OWN_SKIN
      case -1:
         {
            int SZ = fdb.line_spacing;
            int xx = mod.sb.rc.x-SZ*2, yy = y+fdb.line_spacing;
            DrawCheckbox(xx-SZ/2, yy-SZ/2, SZ, !config.color_theme);
         }
         break;
#endif
      case 0:
         {
            DrawString(GetText((E_TEXT_ID)(TXT_COL_ITM_BACKGROUND+mod.item)), (x+max_x)/2, y+fdb.line_spacing, UI_FONT_BIG, FF_CENTER, col_text);
            for(int j=0; j<2; j++){
               if(!j){
                  if(!mod.item)
                     continue;
               }else{
                  if(mod.item==int(mod.num_colors-1))
                     continue;
               }
               const int arrow_size = (fdb.line_spacing/2) | 1;
               int x = !j ? (mod.rc.x+fdb.cell_size_x) : (max_x-arrow_size-fdb.cell_size_x);
               int yy = y + fdb.line_spacing + fdb.cell_size_y/2;
               yy -= arrow_size/2;
               DrawArrowHorizontal(x, yy, arrow_size, 0xff000000|col_text, j);
            }
         }
         break;
      default:
         S_rect rc(x+fdb.cell_size_x, y+fdb.line_spacing, max_x-x-fdb.cell_size_x*2, fdb.cell_size_y);
         DrawOutline(rc, 0x80000000, 0x80ffffff);
         S_rect rcc(rc.x, rc.y, 1, rc.sy);
         int ax = 0;
         switch(selection){
         case 1:
                              //draw spectrum
            for(int x=0; x < rc.sx; x++){
               dword col = MakeColorFromHue(x, rc.sx);
               FillRect(rcc, 0xff000000 | col);
               ++rcc.x;
            }
            ax += mod.color.hue*rc.sx/256;
            break;
         case 2:
                              //draw brightness
            for(int x=0; x < rc.sx; x++){
               dword col = ApplyBrighness(mod.base_for_br, x, rc.sx);
               FillRect(rcc, 0xff000000 | col);
               ++rcc.x;
            }
            ax += mod.color.brightness*rc.sx/256;
            break;
         case 3:
                              //draw saturation
            for(int x=0; x < rc.sx; x++){
               dword col = ApplySaturation(mod.base_for_sat, x, rc.sx-1);
               FillRect(rcc, 0xff000000 | col);
               ++rcc.x;
            }
            ax += mod.color.saturation*rc.sx/256;
            break;
         }
         ax = rc.x + Min(ax, rc.sx-1);
         int aw = fdb.cell_size_x;
         int ay = rc.Bottom() - aw + 1;
         if(GetPixelFormat().bytes_per_pixel==2)
            DrawArrow16((word*)GetBackBuffer(), GetScreenPitch()/2, ax, ay, aw);
         else
            DrawArrow32((dword*)GetBackBuffer(), GetScreenPitch()/4, ax, ay, aw);
      }
      y += fdb.line_spacing * 2;
   }
   DrawScrollbar(mod.sb);

   ClearSoftButtonsArea();
   DrawSoftButtonsBar(mod, TXT_MENU, TXT_BACK);
   SetScreenDirty();
}

//----------------------------
