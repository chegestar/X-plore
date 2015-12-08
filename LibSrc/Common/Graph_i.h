

                              //class for combining clip rectangles during drawing of complex graphics
                              // it saves current clip rectangle, and can combine new clip rectangle with current to prepare drawing next shape
                              // in destructor, it restores original clip rectanlge
class C_clip_rect_helper{
   C_application_base &app;
   S_rect rc_save;
   bool need_restore;
public:
   C_clip_rect_helper(C_application_base &_app)
      : app(_app)
      , need_restore(false)
      , rc_save(_app.GetClipRect())
   {}
   ~C_clip_rect_helper(){
      Restore();
   }

//----------------------------
// Restore clip rectangle what was valid at time of constructing this class.
   void Restore(){
      if(need_restore){
         app.SetClipRect(rc_save);
         need_restore = false;
      }
   }

//----------------------------
// Combine new clip rectangle with existing clip rect.
// Returns true if new clip rectanle is non-zero, and it was set as active clip rect, so that shape may be drawn.
   bool CombineClipRect(const S_rect &rc){
      S_rect rc1;
      rc1.SetIntersection(rc_save, rc);
      if(rc1.IsEmpty())
         return false;
      app.SetClipRect(rc1);
      need_restore = true;
      return true;
   }
};
