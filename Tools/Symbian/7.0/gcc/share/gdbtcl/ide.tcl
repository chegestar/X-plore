#
# ide.tcl
# ----------------------------------------------------------------------
# Implements IDE procedures
#
#   PROCS:
#
#     gdbtk_ide_init - initialize IDE functions
#     src_ide_proc - send a command to the source window
#     gdb_exit_check - check for exit
#     ide_run_server - tell GDB to run
#     target_name_changed - hook for target name changes
#     exe_name_changed - hook for executable name changes
#     receive_file_changed - hook for any file changes
#
# ----------------------------------------------------------------------
#   AUTHOR:  Martin M. Hunt <hunt@cygnus.com>
#   Copyright (C) 1997, 1998 Cygnus Solutions
#

# This procedure is called to initialize when we are using the IDE.
# We register code to handle various IDE requests.
proc gdbtk_ide_init {} {
  global gdb_checking_for_exit gdb_pretty_name tcl_platform Paths
  #debug "ide_init"

  # When the IDE asks us to quit, we set gdb_checking_for_exit true
  # while we are running the check routines.
  set gdb_checking_for_exit 0

  # set the global target and executable names then
  # set up callbacks for when they are updated
  set_target_name [ide_property get target-gdb-name]
  set gdb_pretty_name [ide_property get target-pretty-name]

  set_exe_name [lindex [ide_property get vmake/exelist] 0]
  property add_hook target-gdb-name target_name_changed
  property add_hook target-pretty-name target_pretty_name_changed
  property add_hook vmake/exelist exe_name_changed

  # Register the generic windows.  Note that we can get requests to
  # open them as soon as we register them, so we must be fully
  # initialized and prepared to handle them at this point.
  manage create src
  ide_window_register generic "Foundry _Debugger" src_ide_proc

  ide_exit register_check gdb_exit_check
  ide_exit register_exit gdb_force_quit

  ide_window_register restorer ""

  ide_event initialize \
    { file-created file-changed file-removed file-deleted } \
    { }
  ide_event receive file-created receive_file_changed
  ide_event receive file-changed receive_file_changed
  ide_event receive file-removed receive_file_changed
  ide_event receive file-deleted receive_file_changed

  # initialize Windows help system
  if {$tcl_platform(platform) == "windows"} then {
    set helpdir [file join $Paths(prefix) help]
    ide_help initialize [file join $helpdir foundry.hlp] \
      [file join $helpdir foundry.hh]
  }
}

# This procedure is called when the IDE wants to send a command to the
# source window.  If the IDE wants to bring up the source window, then
# we want to force a download if it has not already been done.
proc src_ide_proc {method args} {
  global gdb_exe_name gdb_target_name gdb_loaded gdb_downloading
  debug "source window request: $method exe=$gdb_exe_name targ=$gdb_target_name $args"
  if {$method == "set_visibility"} {
    if {[lindex $args 0] == "normal" \
	  && $gdb_exe_name != "" \
	  && $gdb_target_name != "" \
	  && !$gdb_loaded \
	  && !$gdb_downloading \
	  && [file exists $gdb_exe_name]} {
      ide_run_server $gdb_exe_name 0
    } else {
      manage open src
    }
  } elseif {$method == "destroy"} {
    # To gdb, destroying the source window exits the program.
    gdb_force_quit
  }
}

# This procedure is used by the IDE to check whether we should exit
# from gdb.
proc gdb_exit_check {} {
  global gdb_checking_for_exit gdb_downloading

  set gdb_checking_for_exit 1
  if {$gdb_downloading} {
    set result [gdbtk_tcl_query "Closing Foundry will cancel the\ndownload to the target.\n\nDo you still want to close Foundry?" no]
  } else {
    set result [gdb_confirm_quit]
  }

  set gdb_checking_for_exit 0
  return $result
}

# This procedure is called when the IDE asks GDB to run an executable.
# We bring up the source window, and run the executable.
proc ide_run_server {exe {start 1}} {
  global gdb_target_name

  # We do everything in an idle callback, so that the caller doesn't
  # have to wait for the user to respond to a dialog or error message.
  after idle [list ide_do_run_server $exe $start]
}

# This is called by ide_run_server as an idle callback.
proc ide_do_run_server {exe start} {
  global gdb_loaded gdb_target_changed gdb_exe_changed
  if {! [file exists $exe]} {
    error "Request to run non-existent executable $exe"
    return
  }

  run_executable $start
}

# This procedure is called by the IDE whenever the target name is
# changed.
proc target_name_changed {propset var targ} {
  global gdb_target_name
  if {$propset} {
    set_target_name $targ
    debug "target_name_changed: $gdb_target_name"
  }
}

# This procedure is called by the IDE whenever the target's pretty
# name is changed.
proc target_pretty_name_changed {propset var targ} {
  global gdb_pretty_name
  if {$propset} {
    set gdb_pretty_name $targ
  }
}

# This procedure is called by the IDE when the executable name is
# changed.
# It's not clear if this is needed because ide_run_server
# also gets an executable name
proc exe_name_changed {propset var exe} {
  if {$propset} {
    set_exe_name [lindex $exe 0]
  }
}

# This procedure is called when somebody posts a process-ended event.
# We look for one from vmake, which may mean that we should change the
# file we are debugging.
proc receive_file_changed {type data} {
  global gdb_exe_name gdb_target_name gdb_exe_changed gdb_target_changed
  global gdb_loaded gdb_pretty_name tcl_platform gdb_running gdb_loaded

  debug "receive_file_changed $type $data"

  # Do a quick string comparison to see if the changed file might be
  # the one we are debugging.
  if {$tcl_platform(platform) == "windows"}  {
    set f1 [string tolower [file tail $data]]
    set f2 [string tolower [file tail $gdb_exe_name]]
  } else {
    set f1 [file tail $data]
    set f2 [file tail $gdb_exe_name]
  }
  if {$f1 != $f2} {
    if {[string match *.o $f1] || [string match *.obj $f1]} {
      # ignore object file changes
      return
    }
    if {$gdb_running || $gdb_loaded} {
      switch $type {
	file-created {set action "added to"}
	file-removed {set action "removed from"}
	file-deleted {set action "deleted from"}
	default {set action "changed in"}
      }
      if {$gdb_running} {
	set act2 "rebuild your project and restart the debugger on it."
      } else {
	set act2 "build and download the new project executable."
      }
      tk_messageBox -icon warning -default ok \
	-title "Foundry Debugger" -type ok -modal system \
	-message "File $data was $action your project.\nYou should probably $act2"
    }
    return
  }
  
  if {[file exists $gdb_exe_name]} {
    set msg "Do you want to debug the new $gdb_exe_name on the target \"$gdb_pretty_name\"?"
    set result [tk_messageBox -default yes -icon question \
	-message $msg -title "Foundry Debugger" -type yesno -modal system]
    if {$result == "yes"} {
      set gdb_loaded 0
      run_executable 1
    }
  }
}
