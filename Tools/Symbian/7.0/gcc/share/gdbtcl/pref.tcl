#
# Pref
# ----------------------------------------------------------------------
# Implements GDBtk preferences dialogs
#
#   PUBLIC ATTRIBUTES:
#
#   METHODS:
#
#     config ....... used to change public attributes
#
#   PRIVATE METHODS
#
#     build_win .... build the main preferences dialog
#
#   X11 OPTION DATABASE ATTRIBUTES
#
#
# ----------------------------------------------------------------------
#   AUTHOR:  Martin M. Hunt	hunt@cygnus.com     
#            Copyright (C) 1997 Cygnus Solutions   
#


itcl_class Pref {
  # ------------------------------------------------------------------
  #  CONSTRUCTOR - create new preferences dialog
  # ------------------------------------------------------------------
  constructor {config} {
    #
    #  Create a window with the same name as this object
    #
    set class [$this info class]
    @@rename $this $this-tmp-
    @@frame $this -class $class 
    @@rename $this $this-win-
    @@rename $this-tmp- $this
    build_win
  }

  # ------------------------------------------------------------------
  #  METHOD:  build_win - build the main preferences window
  # ------------------------------------------------------------------
  method build_win {} {
    frame $this.f
    tixNoteBook $this.f.n 
    
#    $this.f.n add gl -label "GDB" -underline 1
    $this.f.n add load -label "Connection" -underline 0
#    $this.f.n add view -label "View" -underline 0
#    $this.f.n add tb -label "Toolbar" -underline 0
#    $this.f.n add con -label "Console" -underline 0
#    $this.f.n add src -label "Source" -underline 0
#    $this.f.n add reg -label "Register" -underline 0

    # NOTICE: when adding new windows here, be delete them in the destructor
    # and in reconfig. Shouldn't have to do this, but ...
#    pack [Toolpref [$this.f.n subwidget tb].tool -attach 1]
#    pack [Globalpref [$this.f.n subwidget gl].gl -attach 1]
#    pack [RegWinPref [$this.f.n subwidget reg].reg -attach 1]
#    pack [SrcPref [$this.f.n subwidget src].src -attach 1]
#    pack [LoadPref [$this.f.n subwidget view].view -attach 1]
    pack [LoadPref [$this.f.n subwidget load].load -attach 1]

    frame $this.f.b
    button $this.f.b.ok -text OK -width 7 -command "$this save"
    button $this.f.b.cancel -text Cancel -width 7 -command "$this cancel"
    button $this.f.b.help -text Help -width 7 -state disabled

    standard_button_box $this.f.b

    pack $this.f.n $this.f.b $this.f -expand yes -fill both -padx 5 -pady 5
    focus $this.f.b.ok

    wm resizable [winfo toplevel $this] 0 0
  }

  # ------------------------------------------------------------------
  #  DESTRUCTOR - destroy window containing widget
  # ------------------------------------------------------------------
  destructor {
    set top [winfo toplevel $this]
    manage_delete $this 1
    destroy $this
    destroy $top
  }

  # ------------------------------------------------------------------
  #  METHOD:  save - save new defaults before quitting
  # ------------------------------------------------------------------
  method save {} {
    [$this.f.n subwidget load].load save
    manage delete $this
  }

  # ------------------------------------------------------------------
  #  METHOD:  cancel - restore previous values and quit
  # ------------------------------------------------------------------
  method cancel {} {
    [$this.f.n subwidget load].load cancel
    manage delete $this
  }

  # ------------------------------------------------------------------
  #  METHOD:  reconfig - used when preferences change
  # ------------------------------------------------------------------
  method reconfig {} {
    # for now, just delete and recreate
#    [$this.f.n subwidget load].load delete
    destroy $this.f
    build_win
  }
    
  # ------------------------------------------------------------------
  #  METHOD:  config - used to change public attributes
  # ------------------------------------------------------------------
  method config {config} {}

  #
  #  PROTECTED DATA
  #
}
