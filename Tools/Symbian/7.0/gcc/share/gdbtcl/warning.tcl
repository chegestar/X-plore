#
# WarningDlg
# ----------------------------------------------------------------------
# Implements GDB warning dialogs
#
#   PUBLIC ATTRIBUTES:
#
#   METHODS:
#
#     config ....... used to change public attributes
#     build_win..... used to create a warning dialog window
#     ok............ 
#     
#
#     PRIVATE METHODS
#
#   X11 OPTION DATABASE ATTRIBUTES
#
#
# ----------------------------------------------------------------------
#   AUTHOR:  Elena Zannoni   <ezannoni@cygnus.com>
#   Copyright (C) 1998 Cygnus Solutions
#

 
itcl_class WarningDlg {
  # ------------------------------------------------------------------
  #  CONSTRUCTOR - create new warning dialog
  # ------------------------------------------------------------------
  constructor {config} {
   global gignore
    #
    #  Create a window with the same name as this object
    #
    #debug "config display_it is $display_it"
    set class [$this info class]
    @@rename $this $this-tmp-
    @@frame $this -class $class
    @@rename $this $this-win-
    @@rename $this-tmp- $this

   if {$display_it} {
    set gignore $ignore
    build_win
   } else {
    WarningDlg@@destructor
   }
  }
 
  # ------------------------------------------------------------------
  #  METHOD:  build_win - build the dialog
  # ------------------------------------------------------------------
  method build_win {} {
   global gignore

    #debug "WarningDlg::build_win display_it is $display_it"

    if {$attach != 1} {
      frame $this.f
      frame $this.f.a -relief raised -bd 1
      frame $this.f.b -relief raised -bd 1
      set f $this.f.a
    } else {
      set f $this
    }
 
    label $f.bitmap -bitmap warning
    label $f.lab -text $message
    pack $f.bitmap $f.lab -side left -padx 10 -pady 10

    if {$attach != 1} {

      if {$ignorable} {
      checkbutton $this.f.b.ignore -text "Don't show this warning again" \
           -variable gignore -anchor w 
      }

      button $this.f.b.ok -text OK -underline 0 -command "$this ok"
      
      if {$ignorable} {
        pack $this.f.b.ignore
      }
      pack $this.f.b.ok -expand yes -side left 
      pack $this.f.a 
      pack $this.f.b  -fill x
      pack $this.f $this
    }
  }
 


  # ------------------------------------------------------------------
  #  METHOD: ok - set things up for next time and delete window
  # ------------------------------------------------------------------
  method ok {} {
   global gignore

    #debug "method ok $gignore"

    set_next_display $gignore

    manage delete $this

  }

  # ------------------------------------------------------------------
  #  DESTRUCTOR - destroy window containing widget
  # ------------------------------------------------------------------
  destructor {
    set top [winfo toplevel $this]
    manage delete $this 1
    destroy $this
    if {$attach != 1} {
      destroy $top
    }
  }
 
  # ------------------------------------------------------------------
  #  METHOD:  config - used to change public attributes
  # ------------------------------------------------------------------
  method config {config} {}
 
  # ------------------------------------------------------------------
  #  METHOD:  reconfig - used when preferences change
  # ------------------------------------------------------------------
  method reconfig {} {}

  proc set_next_display {value} {
    set display_it [expr {!$value}]
  }
 
  #
  # PUBLIC DATA
  #
  public attach 0
  public ignorable 0
  public message ""

  protected ignore 0
  
  common display_it 1
}
 

