#
# util.tcl
# ----------------------------------------------------------------------
# Misc routines
#
#   PROCS:
#
#     keep_raised - keep a window raised
#     sleep - wait a certain number of seconds and return
#     toggle_debug_mode - turn debugging on and off
#     freeze - make a window modal
#     bp_exists - does a breakpoint exist on linespec?
#
# ----------------------------------------------------------------------
#   AUTHOR:  Martin M. Hunt <hunt@cygnus.com>
#   Copyright (C) 1997, 1998 Cygnus Solutions
#


# A helper procedure to keep a window on top.
proc keep_raised {top} {
  if {[winfo exists $top]} {
    wm deiconify $top
    raise $top
    after 1000 [info level 0]
  }
}

# sleep - wait a certain number of seconds then return
proc sleep {sec} {
  global __sleep_timer
  set __sleep_timer 0
  after [expr {1000 * $sec}] set __sleep_timer 1
  vwait __sleep_timer
}

proc toggle_debug_mode {} {
  if {[pref get global/debug]} {
    debug "Debugging OFF"
    pref set global/debug 0
    pref set global/show_tcl_errors 0
    pref set global/show_tcl_stack_trace 0
  } else {
    pref set global/debug 1
    pref set global/show_tcl_errors 1
    pref set global/show_tcl_stack_trace 1
    debug "Debugging ON"
  }
}

# ------------------------------------------------------------------
#  PROC:  auto_step - automatically step through a program
# ------------------------------------------------------------------

# FIXME FIXME
proc auto_step {} {
  after 2000 auto_step
  gdb_cmd next
}

# ------------------------------------------------------------------
#  PROC:  freeze - make a modal window
# ------------------------------------------------------------------
proc freeze { win  {on_top 0} } {
  global IDE_ENABLED
  set top [winfo toplevel $win]
  if {$on_top} {
    after 500 keep_raised $top
  }
  ide_grab_support disable_except $top
  focus -force $top
  if {$IDE_ENABLED} {
    idewindow_freeze $top
  }
  grab $top
  
  tkwait window $top
  
  grab release $top
#  idewindow_thaw $top
  ide_grab_support enable_all
}

 # ------------------------------------------------------------------
 #  PROC:  tfind_cmd -- to execute a tfind command on the target
 # ------------------------------------------------------------------
 proc tfind_cmd {command} {
    gdbtk_busy
    # need to call gdb_cmd because we want to ignore the output
    set err [catch {gdb_cmd $command} msg]
    if {$err || [regexp "Target failed to find requested trace frame" $msg]} {
       tk_messageBox -icon error -title "GDB" -type ok \
           -modal task -message $msg
       gdbtk_idle
       return
       } else {
    gdbtk_update
    gdbtk_idle
  }
 }

# ------------------------------------------------------------------
#  PROC:  do_test - invoke the test passed in
#           This proc is provided for convenience. For any test
#           that uses the console window (like the console window
#           tests), the file cannot be sourced directly using the
#           'tk' command because it will block the console window
#           until the file is done executing. This proc assures
#           that the console window is free for input by wrapping
#           the source call in an after callback.
#           Users may also pass in the verbose and tests globals
#           used by the testsuite.
# ------------------------------------------------------------------
proc do_test {{file {}} {verbose {}} {tests {}}} {
  global _test

  if {$file == {}} {
    error "wrong \# args: should be: do_test file ?verbose? ?tests ...?"
  }

  if {$verbose != {}} {
    set _test(verbose) $verbose
  } elseif {![info exists _test(verbose)]} {
    set _test(verbose) 0
  }

  if {$tests != {}} {
    set _test(tests) $tests
  }

  set _test(interactive) 1
  after 500 [list source $file]
}

# ------------------------------------------------------------------
#  PROCEDURE:  gdbtk_read_defs
#        Reads in the defs file for the testsuite. This is usually
#        the first procedure called by a test file. It returns
#        1 if it was successful and 0 if not (if run interactively
#        from the console window) or exits (if running via dejagnu).
# ------------------------------------------------------------------
proc gdbtk_read_defs {} {
  global _test env

  if {[info exists env(DEFS)]} {
    set err [catch {source $env(DEFS)} errTxt]
  } else {
    set err [catch {source defs} errTxt]
  }

  if {$err} {
    if {$_test(interactive)} {
      tk_messageBox -icon error -message "Cannot load defs file:\n$errTxt" -type ok
      return 0
    } else {
      puts stdout "cannot load defs files: $errTxt\ntry setting DEFS"
      exit 1
    }
  }

  return 1
}

# ------------------------------------------------------------------
#  PROCEDURE:  bp_exists
#            Returns BPNUM if a breakpoint exists at LINESPEC or
#            -1 if no breakpoint exists there
# ------------------------------------------------------------------
proc bp_exists {linespec} {

  lassign $linespec foo function filename line_number addr pc_addr

  set bps [gdb_get_breakpoint_list]
  foreach bpnum $bps {
    set bpinfo [gdb_get_breakpoint_info $bpnum]
    lassign $bpinfo file func line pc type enabled disposition \
      ignore_count commands cond thread hit_count
    if {$filename == $file && $function == $func && $addr == $pc} {
      return $bpnum
    }
  }

  return -1
}
