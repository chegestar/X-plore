#ifndef __COLOR_SETUP_H_
#define __COLOR_SETUP_H_

//----------------------------

class C_client_color_setup: public C_client{
public:
   struct S_hsb_color{
      byte hue;
      byte brightness;
      byte saturation;
      byte _res;
   };
   struct S_color_preset{
      const wchar *name;
      S_hsb_color colors[5];
   };

   typedef bool (C_client::*t_ApplyColor)(E_COLOR);
private:
   class C_mode_this;
   friend class C_mode_this;

   void SetMode(S_hsb_color *cols, dword nc, const S_color_preset *presets, dword num_presets, t_ApplyColor ac);
   void InitLayout(C_mode_this &mod);
   void ProcessMenu(C_mode_this &mod, int itm, dword menu_id);
   void ProcessInput(C_mode_this &mod, S_user_input &tc, bool &redraw);
   void Tick(C_mode_this &mod, dword time, bool &redraw);
   void Draw(const C_mode_this &mod);

   class C_mode_this: public C_mode{
      virtual bool WantInactiveTimer() const{ return false; }
   public:
      S_hsb_color *colors;
      const dword num_colors;
      t_ApplyColor ApplyColor;
      const S_color_preset *presets;
      const dword num_presets;

      S_rect rc;
      C_scrollbar sb;
      int selection;
      int item;               //color item that is active (COL_*)
      S_hsb_color color;
      dword base_for_br;
      dword base_for_sat;
      dword key_bits;
      S_key_accelerator acc[2];
      bool need_save;
      int curr_drag_item;      //-1 = no
      int drag_mouse_x_x8;

      C_mode_this(C_application_ui &app, C_mode *sm, S_hsb_color *cols, dword nc, const S_color_preset *psts, dword np, t_ApplyColor ac):
         C_mode(app, sm),
         colors(cols),
         num_colors(nc),
         presets(psts),
         num_presets(np),
         ApplyColor(ac),
         selection(0),
         key_bits(0),
         curr_drag_item(-1),
         need_save(false),
         item(0)
      {
      }
      virtual void InitLayout(){ static_cast<C_client_color_setup&>(app).InitLayout(*this); }
      virtual void ProcessInput(S_user_input &ui, bool &redraw){ static_cast<C_client_color_setup&>(app).ProcessInput(*this, ui, redraw); }
      virtual void ProcessMenu(int itm, dword menu_id){ static_cast<C_client_color_setup&>(app).ProcessMenu(*this, itm, menu_id); }
      virtual void Tick(dword time, bool &redraw){ static_cast<C_client_color_setup&>(app).Tick(*this, time, redraw); }
      virtual void Draw() const{ static_cast<C_client_color_setup&>(app).Draw(*this); }
   };
   void EditColor(C_mode_this &mod, E_COLOR col);
public:
   static void CreateMode(C_client *cl, S_hsb_color *cols, dword num_colors, const S_color_preset *presets, dword num_presets, t_ApplyColor ac){
      C_client_color_setup *_this = (C_client_color_setup*)cl;
      _this->SetMode(cols, num_colors, presets, num_presets, ac);
   }

   static dword HsbToRgb(S_hsb_color hsb);
};

//----------------------------
#endif
