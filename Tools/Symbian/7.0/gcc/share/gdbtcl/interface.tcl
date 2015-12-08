#
# Interface between Gdb and GdbTk.
#
#   Copyright (C) 1997, 1998 Cygnus Solutions

# This variable is reserved for this module.  Ensure it is an array.
global gdbtk_state
set gdbtk_state(busyCount) 0

# This is run when a breakpoint changes.  The arguments are the
# action, the breakpoint number, and the breakpoint info.
define_hook gdb_breakpoint_change_hook

####################################################################
#                                                                  #
#                        GUI STATE HOOKS                           #
#                                                                  #
####################################################################
# !!!!!   NOTE   !!!!!
# For debugging purposes, please put debug statements at the very
# beginning and ends of all GUI state hooks.

# GDB_BUSY_HOOK
#   This hook is used to register a callback when the UI should
#   be disabled because the debugger is either busy or talking
#   to the target.
#
#   All callbacks should disable ALL user input which could cause
#   any state changes in either the target or the debugger.
define_hook gdb_busy_hook

# GDB_IDLE_HOOK
#   This hook is used to register a callback when the UI should
#   be enabled because the debugger is no longer busy.
#
#   All callbacks should enable user input. These callbacks
#   should also be as fast as possible to avoid any significant
#   time delays when enabling the UI.
define_hook gdb_idle_hook

# GDB_UPDATE_HOOK
#   This hook is used to register a callback to update the widget
#   when debugger state has changed.
define_hook gdb_update_hook

# GDB_NO_INFERIOR_HOOK
#   This hook is used to register a callback which should be invoked
#   whenever there is no inferior (either at startup time or when
#   an inferior is killed).
#
#   All callbacks should reset their windows to a known, "startup"
#   state.
define_hook gdb_no_inferior_hook

# GDB_DISPLAY_CHANGE_HOOK
# This is run when a display changes.  The arguments are the action,
# the breakpoint number, and (optionally) the value.
define_hook gdb_display_change_hook

# ------------------------------------------------------------------
#  PROCEDURE:  gdbtk_busy - run all busy hooks
#
#         Use this procedure from within GUI code to indicate that
#         the debugger is busy, either running the inferior or
#         talking to the target. This will call all the registered
#         gdb_busy_hook's.
# ------------------------------------------------------------------
proc gdbtk_busy {} {

  set err [catch {run_hooks gdb_busy_hook} txt]
  if {$err} { debug "gdbtk_busy ERROR: $txt" }

  # Force the screen to update
  update
}

# ------------------------------------------------------------------
#   PROCEDURE:  gdbtk_update - run all update hooks
#
#          Use this procedure to force all widgets to update
#          themselves. This hook is usually run after command
#          that could change target state.
# ------------------------------------------------------------------
proc gdbtk_update {} {
  set err [catch {run_hooks gdb_update_hook} txt]
  if {$err} { debug "gdbtk_update ERROR: $txt" }

  # Force the screen to update
  update
}

# ------------------------------------------------------------------
#   PROCEDURE: gdbtk_idle - run all idle hooks
#          Use this procedure to run all the gdb_idle_hook's,
#          which should free the UI for more user input. This
#          hook should only be run AFTER all communication with
#          the target has halted, otherwise the risk of two (or
#          more) widgets talking to the target arises.
# ------------------------------------------------------------------
proc gdbtk_idle {} {
  global gdb_running

  set err [catch {run_hooks gdb_idle_hook} txt]
  if {$err} { debug "gdbtk_idle 1 ERROR: $txt" }

  if {!$gdb_running} {
    set err [catch {run_hooks gdb_no_inferior_hook} txt]
    if {$err} { debug "gdbtk_idle 2 ERROR: $txt" }
  }    
  # Force the screen to update
  update
}

define_hook download_progress_hook

# Random hook of procs to call just before exiting.
define_hook gdb_quit_hook

# ------------------------------------------------------------------
#  PROCEDURE:  gdbtk_quit - Quit the debugger
#         Call this procedure anywhere the user can request to quit.
#         This procedure will ask all the right questions and run
#         all the gdb_quit_hooks before exiting. 
# ------------------------------------------------------------------
proc gdbtk_quit {} {
  global _manage_busy gdb_downloading gdb_running

  if {$gdb_downloading} {
    set msg "Downloading to target,\n really close the debugger?"
    if {![gdbtk_tcl_query $msg no]} {
      return
    }
  } elseif {$_manage_busy || $gdb_running} {
    # While we are running the inferior, gdb_cmd is fenceposted and returns
    # immediately. Therefore, we need to ask here. Do we need to stop the target,
    # too?
    set msg "A debugging session is active.\n"
    append msg "Do you still want to close the debugger?"
    if {![gdbtk_tcl_query $msg no]} {
      return
    }
  }
  gdb_force_quit
}

# ------------------------------------------------------------------
#  PROCEDURE:  gdbtk_cleanup - called by GDB immediately
#         before exiting.  Last chance to cleanup!
# ------------------------------------------------------------------
proc gdbtk_cleanup {} {
  global IDE_ENABLED
  if {!$IDE_ENABLED} {
    # tell the window manager to cleanup and save window state
    manage save
  }
}


proc gdbtk_tcl_query {message {default yes}} {
  global IDE_ENABLED gdb_checking_for_exit gdbtk_state tcl_platform

  # FIXME We really want a Help button here.  But Tk's brain-damaged
  # modal dialogs won't really allow it.  Should have async dialog
  # here.

  if {$IDE_ENABLED} {
    set title "Foundry Debugger"
  } else {
    set title "GDB"
  }

  set modal "task"

  # If we are checking whether to exit gdb, we want a system modal
  # box.  Otherwise it may be hidden by some other program, and the
  # user will have no idea what is going on.
  if {[info exists gdb_checking_for_exit] && $gdb_checking_for_exit} {
    set modal "system"
  }
  
  if {$tcl_platform(platform) == "windows"} {
    # On Windows, we want to only ask each question once.
    # If we're already asking the question, just wait for the answer
    # to come back.
    set ans [list answer $message]
    set pending [list pending $message]

    if {[info exists gdbtk_state($pending)]} {
      incr gdbtk_state($pending)
    } else {
      set gdbtk_state($pending) 1
      set gdbtk_state($ans) {}

      ide_messageBox [list set gdbtk_state($ans)] -icon warning \
	-default $default -message $message -title $title \
	-type yesno -modal $modal -parent .
    }

    vwait gdbtk_state($ans)
    set r $gdbtk_state($ans)
    if {[incr gdbtk_state($pending) -1] == 0} {
      # Last call waiting for this answer, so clear it.
      unset gdbtk_state($pending)
      unset gdbtk_state($ans)
    }
  } else {
    # On Unix, apparently it doesn't matter how many times we ask a
    # question.
    set r [tk_messageBox -icon warning -default $default \
	     -message $message -title $title \
	     -type yesno -modal $modal -parent .]
  }

  update idletasks
  return [expr {$r == "yes"}]
}

proc gdbtk_tcl_warning {message} {
  debug "gdbtk_tcl_warning: $message"

# ADD a warning message here if the gui must NOT display it
# add the message at the beginning of the switch followed by - 

  switch -regexp $message {
        "Unable to find dynamic linker breakpoint function.*" {return}
        default {show_warning $message}
       }
}

proc show_warning {message} {
  global IDE_ENABLED tcl_platform

  # FIXME We really want a Help button here.  But Tk's brain-damaged
  # modal dialogs won't really allow it.  Should have async dialog
  # here.
  if {$IDE_ENABLED} {
    set title "Foundry Debugger"
  } else {
    set title "GDB"
  }
 
  set modal "task"
 
  if {$tcl_platform(platform) == "windows"} {
      ide_messageBox -icon warning \
        -default ok -message $message -title $title \
        -type ok -modal $modal -parent .
  } else {
    set r [tk_messageBox -icon warning -default ok \
             -message $message -title $title \
             -type ok -modal $modal -parent .]
  }
} 

proc gdbtk_tcl_ignorable_warning {message} {
  
manage create warndlg -message $message -ignorable 1

} 

proc gdbtk_tcl_fputs {message} {
  global gdbtk_state
  if {$gdbtk_state(console) != ""} {
    $gdbtk_state(console) insert $message
  }
}

proc echo {args} {
  gdbtk_tcl_fputs [concat $args]\n
}

proc gdbtk_tcl_fputs_error {message} {
  global gdbtk_state
  if {$gdbtk_state(console) != ""} {
    $gdbtk_state(console) einsert $message
    update
  }
}

proc gdbtk_tcl_flush {} {
  debug [info level 0]
}

proc gdbtk_tcl_start_variable_annotation {valaddr ref_type stor_cl
					  cum_expr field type_cast} {
  debug [info level 0]
}

proc gdbtk_tcl_end_variable_annotation {} {
  debug [info level 0]
}

proc gdbtk_tcl_breakpoint {action bpnum addr line file} {
#  debug "BREAKPOINT: $action $bpnum $addr $line $file"
  if {$action == "create" || $action == "modify"} {
    set bp_type [lindex [gdb_get_breakpoint_info $bpnum] 6]
  } else {
    set bp_type 0
  }
  run_hooks gdb_breakpoint_change_hook $action $bpnum $addr $line $file $bp_type
}

proc gdbtk_tcl_tracepoint {action tpnum addr line file pass_count} {
#  debug "TRACEPOINT: $action $tpnum $addr $line $file $pass_count"
  run_hooks gdb_breakpoint_change_hook $action $tpnum $addr $line $file tracepoint
}

################################################################
#
# Handle `readline' interface.
#

# Run a command that is known to use the "readline" interface.  We set
# up the appropriate buffer, and then run the actual command via
# gdb_cmd.  Calls into the "readline" callbacks just return values
# from our list.
 proc gdb_run_readline_command {command args} {
  global gdbtk_state
  debug "run readline_command $command $args"
  set gdbtk_state(readlineArgs) $args
  gdb_cmd $command
}

proc gdbtk_tcl_readline_begin {message} {
  global gdbtk_state
  debug "readline begin"
  set gdbtk_state(readline) 0
  if {$gdbtk_state(console) != ""} {
    $gdbtk_state(console) insert $message
  }
}

proc gdbtk_tcl_readline {prompt} {
  global gdbtk_state
  debug "gdbtk_tcl_readline $prompt"
  if {[info exists gdbtk_state(readlineArgs)]} {
    # Not interactive, so pop the list, and print element.
    set cmd [lvarpop gdbtk_state(readlineArgs)]
    command@@insert_command $cmd
  } else {
    # Interactive.
    debug "interactive"
    set gdbtk_state(readline) 1
    $gdbtk_state(console) activate $prompt
    vwait gdbtk_state(readline_response)
    set cmd $gdbtk_state(readline_response)
    debug "got response: $cmd"
    unset gdbtk_state(readline_response)
    set gdbtk_state(readline) 0
  }
  return $cmd
}

proc gdbtk_tcl_readline_end {} {
  global gdbtk_state
  debug "readline_end"
  catch {unset gdbtk_state(readlineArgs)}
  unset gdbtk_state(readlineActive)
  command@@end_multi_line_input
}

################################################################
#
# this is called immediately before gdb executes a command.
#
proc gdbtk_tcl_busy {} {
  global gdbtk_state
  if {[incr gdbtk_state(busyCount)] == 1} {
    gdbtk_busy
  }
}

################################################################
#
# this is called immediately after gdb executes a command.
#

proc gdbtk_tcl_idle {} {
  global gdbtk_state
  if {$gdbtk_state(busyCount) > 0
      && [incr gdbtk_state(busyCount) -1] == 0} {
    gdbtk_update
    gdbtk_idle
  }
}

proc gdbtk_tcl_tstart {} {
  set srcwin [lindex [manage find src] 0]
  $srcwin.toolbar configure -runstop 1
  $srcwin.toolbar.m.run entryconfigure 2 -state disabled
  $srcwin.toolbar.m.run entryconfigure 3 -state normal

}

proc gdbtk_tcl_tstop {} {
    set srcwin [lindex [manage find src] 0]
    $srcwin.toolbar configure -runstop 0
    $srcwin.toolbar.m.run entryconfigure 2 -state normal
    $srcwin.toolbar.m.run entryconfigure 3 -state disabled
  
}

# A display changed.  ACTION is `enable', `disable', `delete',
# `create', or `update'.  VALUE is only meaningful in the `update'
# case.
proc gdbtk_tcl_display {action number {value {}}} {
  # Handle create explicitly.
  if {$action == "create"} {
    manage create_if_never data
  }
  run_hooks gdb_display_change_hook $action $number $value
}

proc gdbtk_pc_changed {} {
  set pc [gdb_pc_reg]
  debug "PC CHANGED TO $pc"
  after idle [format {
    catch {gdb_cmd [list tbreak *%s]}
    catch {gdb_cmd continue} } $pc]
}

# ------------------------------------------------------------------
#  PROCEDURE:  gdbtk_tcl_pre_add_symbol
#         This hook is called before any symbol files
#         are loaded so that we can inform the user.
# ------------------------------------------------------------------
proc gdbtk_tcl_pre_add_symbol {file} {
  gdbtk_busy
  set srcs [manage find src]
  foreach w $srcs {
    $w reset
    $w set_status "Reading symbols from $file..."
  }
  update
}

# ------------------------------------------------------------------
#   PROCEDURE: gdbtk_tcl_post_add_symbol
#          This hook is called after we finish reading a symbol
#          file.
# ------------------------------------------------------------------
proc gdbtk_tcl_post_add_symbol {} {
  global gdb_loaded gdb_running

  set gdb_loaded 0
  set gdb_running 0

  # We need to force this to some default location. Assume main and
  # if that fails, let the source window guess (via gdb_loc using stop_pc).
  set src [lindex [manage find src] 0]
  if {[catch {$src location BROWSE_TAG [gdb_loc main]}]} {
  gdbtk_update
  }
  gdbtk_idle
}
