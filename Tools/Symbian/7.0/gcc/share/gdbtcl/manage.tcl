#
# window manager functions
# ----------------------------------------------------------------------
# 
# These functions keep track of what windows are currently open
#
# PUBLIC manage
#
#   manage init - initialize all arrays and open windows
#                 according to defaults and preferences
#
#   manage create win - create a new window of type $win.
#                       Returns the created window object.
#
#   manage open win - raise and focus on the first existing window of type
#                     $win. If none exists, create one. Returns
#                     the window object.
#
#   manage delete obj - delete window object $obj
#
#   manage restart - update or close and reopen all 
#                    existing window objects
#
#   manage save - save the states of all existing window
#                 objects to the preferences file.
#
#   manage raise - raise, focus, and deiconify a window
#
#   manage find win - Return list of windows of type $win
#
#   manage disable_all - Disable window (manager) functions
#
#   manage enable_all  - Enable window (manager) functions
#
# ----------------------------------------------------------------------
#   AUTHOR:  Martin M. Hunt <hunt@cygnus.com>
#   Copyright (C) 1997, 1998 Cygnus Solutions
#

proc manage { cmd args } {

  switch $cmd {

    open {
      return [eval manage_open $args]
    }

    create {
      return [eval manage_create $args]
    }

    find {
      return [eval manage_find $args]
    }

    delete {
      manage_delete [lindex $args 0] [lindex $args 1] 0 [lindex $args 2]
    }

    restart {
      manage_restart
    }

    save {
      manage_save
    }

    init {
      manage_init
    }

    raise {
      return [manage_raise [lindex $args 0]]
    }

    disable_all {
      manage_disable_all
    }

    enable_all {
      manage_enable_all
    }
  }
}

# PRIVATE variables
#
# _manage_active - list of active windows Each element of the list
# contains a window type and a window object.
#
# _manage_objects - array of all known managed objects

# ------------------------------------------------------------------
#  PRIVATE manage_init  - initialize the window objects
#  This is called after [pref init].  It uses
#  the prefs database to open the appropriate windows.
# ------------------------------------------------------------------

proc manage_init { } {
  global _manage_objects IDE_ENABLED _manage_active _manage_enabled_flag _manage_busy
  set _manage_active ""
  set _manage_enabled_flag 1

  # this array sets the default behavior of every possible
  # top-level window.  All new windows must be entered here.
  # -type sets the object type
  # -name sets the windows name (title)
  # -menu is set to 1 if it is a permanent window that
  #   should be in the window menu
  # -menuname sets the name in the windows menu
  
  array set _manage_objects {
    about-name "About GDB"
    about-type About
    about-menu 0
    about-save 1
    about-menuname "About"
    actiondlg-name "Edit Action"
    actiondlg-type ActionDlg
    actiondlg-menu 0
    actiondlg-menuname ""
    bp-name "Breakpoints"
    bp-type BpWin
    bp-menu 1
    bp-menuname "Breakpoints"
    bp-save 1
    console-name "Console Window"
    console-type Console
    console-menu 1
    console-menuname "Console"
    console-save 1
    globalpref-name "Global Preferences"
    globalpref-type Globalpref
    globalpref-menu 0
    globalpref-menuname "Global Preferences"
    browser-name "Function Browser"
    browser-type BrowserWin
    browser-menu 1
    browser-save 0
    browser-menuname "Function Browser"
    help-name "Help"
    help-type HtmlViewer
    help-menu 1
    help-save 0
    help-menuname "Help"
    load-name ""
    load-type Download
    load-menu 0
    load-menuname ""
    targetsel-name "Target Selection"
    targetsel-type TargetSelection
    targetsel-menu 0
    targetsel-menuname "Target Selection"
    ideloadpref-name "Download Options"
    ideloadpref-type LoadPref
    ideloadpref-menu 0
    ideloadpref-menuname "Download Options"
    locals-name "Local Variables"
    locals-type LocalsWin
    locals-menu 1
    locals-menuname "Local Variables"
    locals-save 1
    mem-name "Memory"
    mem-type MemWin
    mem-menu 1
    mem-save 1
    mem-menuname "Memory"
    mempref-name "Memory Preferences"
    mempref-type MemPref
    mempref-menu 0
    mempref-menuname "Memory Preferences"
    pref-name "Debugger Preferences"
    pref-type Pref
    pref-menu 0
    pref-menuname "Debugger Preferences"
    process-name "Threads"
    process-type ProcessWin
    process-menu 1
    process-menuname "Thread List"
    process-save 1
    reg-name "Registers"
    reg-type RegWin
    reg-menu 1
    reg-menuname "Registers"
    reg-save 1
    regpref-name "Register Preferences"
    regpref-type RegWinPref
    regpref-menu 0
    regpref-menuname "Register Preferences"
    src-name ""
    src-type SrcWin
    src-menu 1
    src-menuname "Source"
    src-save 1
    srcpref-name "Source Preferences"
    srcpref-type SrcPref
    srcpref-menu 0
    srcpref-menuname "Source Preferences"
    stack-name "Stack"
    stack-type StackWin
    stack-menu 1
    stack-menuname "Stack"
    stack-save 1
    tdump-name "Tdump"
    tdump-type TdumpWin
    tdump-menu 0
    tdump-menuname ""
    tfindlinearg-name "Tfind Line"
    tfindlinearg-type TfindArgs
    tfindlinearg-menu 0
    tfindlinearg-menuname ""
    tfindpcarg-name "Tfind PC"
    tfindpcarg-type TfindArgs
    tfindpcarg-menu 0
    tfindpcarg-menuname ""
    tfindtparg-name "Tfind Tracepoint"
    tfindtparg-type TfindArgs
    tfindtparg-menu 0
    tfindtparg-menuname ""
    tp-name "Tracepoints"
    tp-type BpWin
    tp-menu 1
    tp-menuname "Tracepoints"
    tp-save 1
    tracedlg-name "Add Tracepoint"
    tracedlg-type TraceDlg
    tracedlg-menu 0
    tracedlg-menuname ""
    warndlg-name "Warning"
    warndlg-type WarningDlg
    warndlg-menu 0
    warndlg-menuname ""
    watch-name "Watch Expressions"
    watch-type WatchWin
    watch-menu 1
    watch-menuname "Watch Expressions"
    watch-save 1
  }

  if {$IDE_ENABLED} {
    set _manage_objects(about-name) "About Cygnus Foundry"
  } else {
    set _manage_objects(about-name) "About GDB"
  }

  # withdraw the . toplevel
  wm withdraw .
  
  # now open all initial windows
  foreach win [array name _manage_objects *-name] {
    set i [string first "-" $win]
    incr i -1
    set win [string range $win 0 $i]

    if {! $IDE_ENABLED} {
      # When using the IDE we don't bother with this; we let the IDE
      # drive how the windows should be restored.

      # do not open windows that are not related to the current mode

      if { ($win == "tp"    && [pref get gdb/mode] == 0) || \
           ($win == "bp"    && [pref get gdb/mode] == 1) || \
           ($win == "tdump" && [pref get gdb/mode] == 0) } continue

      set num [pref getd gdb/$win/active]
      if {$num != ""} {
	for {set i 0} {$i < $num} {incr i} {
	  manage_create $win
	}
      }
    }
  }

  # Add hooks
  set _manage_busy 0
  add_hook gdb_idle_hook {global _manage_busy; set _manage_busy 0}
  add_hook gdb_no_inferior_hook {global _manage_busy; set _manage_busy 0}
  add_hook gdb_busy_hook {global _manage_busy; set _manage_busy 1}
}

 
# ------------------------------------------------------------------
#  PRIVATE manage_create - create a new window object
# ------------------------------------------------------------------

proc manage_create { args } {
  global _manage_objects _manage_active tcl_platform
  global IDE_ENABLED gdbtk_state env

  debug "manage create $args $IDE_ENABLED"
  set win [lindex $args 0]
  set args [lrange $args 1 end]

  set i 0
  if {$win == "src" && $IDE_ENABLED && ![winfo exists .src]} {
    set newwin .$win
    bind_for_toplevel_only . <Unmap> {
      manage_iconify iconify
    }
    bind_for_toplevel_only . <Map> {
      manage_iconify deiconify
    }
      set top .
      wm withdraw $top
    eval $_manage_objects(${win}-type) $newwin $args
  } else {
    # increment window numbers until we get an unused one
    while {[winfo exists .$win$i]} { incr i }
    while { 1 } {
      set top [toplevel .$win$i]
      wm withdraw $top
      set newwin $top.$win
      debug "newwin=$newwin"
      if {[catch {eval $_manage_objects(${win}-type) $newwin $args} msg]} {
	debug "failed: $msg"
	if {[string first "object already exists" $msg] != -1} {
	  # sometimes an object is still really around even though
	  # [winfo exists] said it didn't exist.  Check for this case
	  # and increment the window number again.
	  catch {destroy $top}
	  incr i
	} else {
	  return ""
	}
      } else {
	break
      }
    }
  }
  
  # add it to the active list
  lappend _manage_active [list $win $newwin]

  # special hack for gdb_console
  if {$win == "console"} {
    set gdbtk_state(console) $newwin
  }

  if {$i == 0} {
    set w $win
    if {$_manage_objects(${win}-name) != ""} {
      wm title $top "$_manage_objects(${win}-name)"
    }
  } else {
    set w "$win-$i"
    if {$_manage_objects(${win}-name) != ""} {
      wm title $top "$_manage_objects(${win}-name) $i"
    }
  }

  # set the window geometry if it was saved
  if {$IDE_ENABLED} {
    set g [ide_property get gdb/$w/geometry]
  } elseif {[info exists env(GDBTK_TEST_RUNNING)] && $env(GDBTK_TEST_RUNNING)} {
    set g "+0+0"
  } else {
    set g [pref getd gdb/$w/geometry]
  }
  
  # on Unix systems, create a small window to be used
  # as an icon when GDB is iconified.  Doesn't work
  # with all window managers
  if {[info exists _manage_objects(${win}-icon)] \
      && $tcl_platform(platform) == "unix"} {
    if {![winfo exists ${top}_icon]} {
      wm iconwindow $top [make_icon_window ${top}_icon $_manage_objects(${win}-icon)]
    }
  }

  wm protocol $top WM_DELETE_WINDOW "manage delete $newwin"
  
  pack $newwin -expand yes -fill both
 
  if {$IDE_ENABLED && $win != "src"} {
    if {$_manage_objects(${win}-menu)} {
      after idle manage_register_defaults $win $newwin $i
    }
  }

  if {$g != ""} {
    debug "setting geometry to $g"
    wm geometry $top $g
    wm positionfrom $top user
  }
  after idle manage_raise $top
  return $newwin
}

# ------------------------------------------------------------------
#  PRIVATE manage_open - raise and focus a window of type $win,
#  creating one if necessary.
# ------------------------------------------------------------------

proc manage_open { args} {
  global _manage_objects _manage_active IDE_ENABLED
  
  debug "manage open $args"
  
  set win [lindex $args 0]
  
  if {$win == "about" && $IDE_ENABLED} {
    # Request it from the IDE.
    idewindow_activate_by_name "@@!About"
    # FIXME: what to return here?  For now we rely on the fact that
    # nobody cares about the return value in this case.
    return ""
  }

  foreach w $_manage_active {
    if {[lindex $w 0] == $win} {
      set obj [lindex $w 1]
      set top [winfo toplevel $obj]
      wm deiconify $top
      focus -force [focus -lastfor $top]
      raise $top
      return $obj
    }
  }

  # if we reached here, we didn't find any windows
  # of type $win, so create one.
  return [eval manage_create $args]
}

# ------------------------------------------------------------------
#  PRIVATE manage_find - return a list of windows of a given type
# ------------------------------------------------------------------

proc manage_find { win } {
  global _manage_objects _manage_active

  set res ""
  foreach w $_manage_active {
    if {[lindex $w 0] == $win} {
      set obj [lindex $w 1]
      lappend res $obj
    }
  }
  return $res
}

# ------------------------------------------------------------------
#  PRIVATE manage_delete - delete a window object
# ------------------------------------------------------------------

proc manage_delete { obj {notify_only 0} {restarting 0} {wreg ""}} {
  global _manage_active gdbtk_state IDE_ENABLED gdb_downloading _manage_busy

  # if the IDE is running, deregister window
  if {!$_manage_busy && $IDE_ENABLED && $wreg != ""} {
    ide_window_register deregister $wreg
  }
  
  set win ""
  foreach w $_manage_active {
    if {[lindex $w 1] == $obj} {
      set win [lindex $w 0]
      break
    }
  }

  debug "manage_delete $obj $win $wreg notify_only=$notify_only"

  # now tell the object to delete itself
  if {$notify_only != "1"} {
    # if there are no more source windows, exit
    if {$win == "src" && $restarting == "0"} {
      set num_src 0
      foreach w $_manage_active {
	if {[lindex $w 0] == $win} {
	  incr num_src
	}
      }
      if {$num_src <= 1} {
	gdbtk_quit
	return
      }
    }
    if {$_manage_busy} { return }
    $obj delete
  }

  # remove from list
  set i 0
  foreach w $_manage_active {
    if {[lindex $w 1] == $obj} {
      set _manage_active [lreplace $_manage_active $i $i]
      break
    }
    incr i
  }

  # see if there's a place for console errors to go to
  if {$win == "console"} {
    set gdbtk_state(console) [manage_find console]
  }

}

# ------------------------------------------------------------------
#  PRIVATE manage_restart - update all windows to reflect new options
# ------------------------------------------------------------------

proc manage_restart {} {
  global _manage_active IDE_ENABLED

  # This is needed in case we've called "gdbtk_busy" before the restart.
  # This will configure the stop/run button as necessary
  after idle gdbtk_idle

  # All windows are implemented as objects.  If the object
  # has a method "reconfig", call that, otherwise delete
  # the window and create a new one.

  foreach w $_manage_active {
    set obj [lindex $w 1]
    if {[catch {$obj reconfig} msg]} {
      debug "reconfig failed: $msg"
      set g [wm geometry [winfo toplevel $obj]]
      manage_delete $obj 0 1
      wm geometry [winfo toplevel [manage_create [lindex $w 0]]] $g
    } else {
      wm deiconify [winfo toplevel $obj]
    }
  }
}


# ------------------------------------------------------------------
#  PRIVATE manage_iconify - iconify/deiconify/withdraw all windows
# ------------------------------------------------------------------

proc manage_iconify {func} {
  global _manage_active
  foreach w $_manage_active {
    set top [winfo toplevel [lindex $w 1]]
    if { $top != "." } {
      catch {wm $func $top}
    }
  }
}

# ------------------------------------------------------------------
#  PRIVATE manage_save - save the positions and state of all windows
# ------------------------------------------------------------------

proc manage_save {} {
  global _manage_active _manage_objects IDE_ENABLED

  debug manage_save

  # set all known windows inactive
  foreach win [array name _manage_objects *-name] {
    set i [string first "-" $win]
    incr i -1
    set win [string range $win 0 $i]
    if {$IDE_ENABLED} {
      ide_property set gdb/$win/active 0 project
    } else {
      pref setd gdb/$win/active 0
    }
  }

  # now scan the list of active windows
  foreach w $_manage_active {
    #debug "w=$w"
    set win [lindex $w 0]
    set obj [lindex $w 1]
    set top [winfo toplevel $obj]
    # record the states and geometries of all windows
    # with the "save" option set
    if { [info exists _manage_objects($win-save)]
	 && $_manage_objects($win-save) == 1} {
      set g [wm geometry $top]

      set d 0
      if {$top != "."} {
	scan $top ".$win%d" d
	set w [string trimleft $top "."]
      }
      if { $d > 0 } {
	if {$IDE_ENABLED} {
	  ide_property set gdb/$win-$d/geometry $g project
	} else {
	  pref setd gdb/$win-$d/geometry $g
	}
      } else {
	if {$IDE_ENABLED} {
	  ide_property set gdb/$win/geometry $g project
	} else {
	  pref setd gdb/$win/geometry $g
	}
      }

      # here we increment the preferences variable
      # because we want "active" to be the number of
      # windows of type $win active
      if {$IDE_ENABLED} {
	set i [ide_property get gdb/$win/active]
	incr i
	ide_property set gdb/$win/active $i project
      } else {
	set i [pref get gdb/$win/active]
	incr i
	pref setd gdb/$win/active $i
      }
    }
  }
  
  if {!$IDE_ENABLED} {
    pref_save ""
  }
}


# ------------------------------------------------------------------
#  manage_raise - deiconify, focus, and raise a specific window
# ------------------------------------------------------------------

proc manage_raise { win } {
    set top [winfo toplevel $win]
    wm deiconify $top
    focus $top
    raise $top
    return $win
}

# ------------------------------------------------------------------
#  PRIVATE manage_register_defaults - register default windows
#  This is called as an idle callback so that it doesn't trigger
#  the display of the window before the window is set up.
# ------------------------------------------------------------------

proc manage_register_defaults {win newwin number} {
  global _manage_objects
  if {$number != "0"} {
    set menuname "$_manage_objects(${win}-menuname)_$number"
  } else {
    set menuname "$_manage_objects(${win}-menuname)"
  }
  debug "registering $menuname"
  set reg [ide_window_register transient $menuname \
	     [list idewindow $newwin ""]]

  set top [winfo toplevel $newwin]
  wm protocol $top WM_DELETE_WINDOW "manage delete $newwin 0 $reg"
}

# ------------------------------------------------------------------
#  PRIVATE make_icon_window - create a small window with an icon in
#  it for use by certain Unix window managers.
# ------------------------------------------------------------------

proc make_icon_window {name file} {
  global gdb_ImageDir
  toplevel $name
  label $name.im -image [image create photo icon_photo -file [file join $gdb_ImageDir $file.gif]]
  pack $name.im
  return $name
}

# ------------------------------------------------------------------
# PROCEDURE:   manage_disable_all
#              Disable window functions on all windows.
#              Right now, we just stop people from destroying
#              windows -- should we also trap minimize
#              and maximize?
# ------------------------------------------------------------------
proc manage_disable_all {} {
  global _manage_active _manage_bindings _manage_enabled_flag
  if {$_manage_enabled_flag} {
    #debug "manage_disable_all"
    set _manage_enabled_flag 0
    foreach wins $_manage_active {
      set type [lindex $wins 0]
      set obj  [lindex $wins 1]
      set top [winfo toplevel $obj]
      if {$top != "."} {
	set _manage_bindings($obj,delete) [wm protocol $top WM_DELETE_WINDOW]
	wm protocol $top WM_DELETE_WINDOW "_manage_null_handler WM_DELETE_WINDOW $top"
      }
    }
  }
}

# ------------------------------------------------------------------
# PROCEDURE:   manage_enable_all
#              Enable any window functions that may have been
#              disabled with manage_disable_all
# ------------------------------------------------------------------
proc manage_enable_all {} {
  global _manage_bindings _manage_enabled_flag
  if {!$_manage_enabled_flag} {
    #debug "manage_enable_all"
    set _manage_enabled_flag 1
    foreach binding [array names _manage_bindings] {
      set names [split $binding ,]
      set obj [lindex $names 0]
      set property [lindex $names 1]
      _manage_set_property $obj $property $_manage_bindings($binding)
      unset _manage_bindings($binding)
    }
  }
}

# ------------------------------------------------------------------
# PRIVATE PROCEDURE:   _manage_set_property
#               This is helper proc for manage_enable_all which
#               simply maps the array names to properties.
# ------------------------------------------------------------------
proc _manage_set_property {obj property binding} {
  
  switch $property {
    delete {
      wm protocol [winfo toplevel $obj] WM_DELETE_WINDOW $binding
    }
    default {
      debug "Unknown property in _manage_set_property: $property"
    }
  }
}
