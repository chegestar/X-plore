# local variable window
#   Copyright (C) 1997, 1998 Cygnus Solutions

itcl_class LocalsWin {
  inherit VariableWin

  # ------------------------------------------------------------------
  #  CONSTRUCTOR - create new locals window
  # ------------------------------------------------------------------
  constructor {config} {
    VariableWin@@constructor
    update
  }

  # ------------------------------------------------------------------
  # DESTRUCTOR - delete locals window
  # ------------------------------------------------------------------
  destructor {
    VariableWin@@destructor
  }

  method build_win {f} {
    global tcl_platform
    build_menu_helper Variable
    if {$tcl_platform(platform) == "windows"} {
      frame $f.f
      VariableWin@@build_win $f.f
      pack $f.f -expand yes -fill both -side top
      frame $f.stat
      pack $f.stat -side bottom -fill x
    } else {
      VariableWin@@build_win $f
    }
  }

  # ------------------------------------------------------------------
  # METHOD: getVariablesBlankPath
  # Overrides VarialbeWin::getVariablesBlankPath. For a Locals Window,
  # this method returns a list of local variables.
  # ------------------------------------------------------------------
  method getVariablesBlankPath {} {
    #    debug "LocalsWin::getVariablesBlankPath"
    return [getLocals]
  }

  method update {} {
    global Update Display
#    debug "LocalsWin::update"
    debug "START LOCALS UPDATE CALLBACK"
    # Check that a context switch has not occured
    set locals [getLocals]
    #    debug "old= $Locals"
    if {"$locals" != "$Locals"} {
      #      debug "new= $locals"
      #      debug "CONTEXT SWITCH"
      # our context has changed... repopulate with new variables
      # destroy the old tree and create a new one
      deleteTree
      set Locals $locals
      set ChangeList {}
      
      if {$locals != ""} {
	populate {}
      }
    }
    
    previous update
    debug "END LOCALS UPDATE CALLBACK"
  }     
}

