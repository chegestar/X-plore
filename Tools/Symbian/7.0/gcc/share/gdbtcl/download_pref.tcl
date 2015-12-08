#
# LoadPref
# ----------------------------------------------------------------------
# Implements GDB download preferences dialog
#
#   PUBLIC ATTRIBUTES:
#
#   METHODS:
#
#     config ....... used to change public attributes
#
#     PRIVATE METHODS
#
#   X11 OPTION DATABASE ATTRIBUTES
#
#
# ----------------------------------------------------------------------
#   AUTHOR:  Martin M. Hunt	hunt@cygnus.com
#            Copyright (C) 1997, 1998 Cygnus Solutions
#


itcl_class LoadPref {
  # The base class is in libide so that it can be used in vmake as
  # well.
  inherit GdbLoadPref

  # ------------------------------------------------------------------
  #  CONSTRUCTOR - create new toolbar preferences dialog
  # ------------------------------------------------------------------
  constructor {config} {
    GdbLoadPref@@constructor
    $this config -okcommand [list $this _button] \
      -cancelcommand [list $this _button] \
      -helpcommand [list $this help] -gdbrunning 1
    wm transient [winfo toplevel $this] .
  }

  # ------------------------------------------------------------------
  #  DESTRUCTOR - destroy window containing widget
  # ------------------------------------------------------------------
  destructor {
    set top [winfo toplevel $this]
    manage delete $this 1
    if {$attach != 1} {
      destroy $top
    }
  }

  # ------------------------------------------------------------------
  #  METHOD:  _button - called when user clicks some button
  # ------------------------------------------------------------------
  method _button {} {
    if {$attach != 1} {
      manage delete $this
    }
  }

  # ------------------------------------------------------------------
  #  METHOD:  help - launches context sensitive help
  # ------------------------------------------------------------------
  method help {} {
    ide_help topic Debugger_menus_File_Debugger_Preferences
  }
}
