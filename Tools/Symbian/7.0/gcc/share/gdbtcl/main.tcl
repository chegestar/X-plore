#
# main.tcl
# ----------------------------------------------------------------------
# GDB tcl entry point
# just initialize everything then set up main state engine
#
#   PROCS:
#
#     gdbtk_tcl_preloop - called by GDB after GDB is initialized
#     set_exe_name - set executable name
#     set_exe - issue "file" command for new executable if necessary
#     set_target_name - set target name
#     set_target - issue "target" command for new target if necessary
#     run_executable - download (if necessary) and run
#     set_baud - set GDB baud rate
#     do_state_hook - call hook for debugger state changes
#
# ----------------------------------------------------------------------
#   Copyright (C) 1997, 1998 Cygnus Solutions
#

# State is controlled by 5 global boolean variables.
#
# gdb_target_changed
# gdb_exe_changed
# gdb_running
# gdb_downloading
# gdb_loaded
 
# gdbtk_tcl_preloop() is called after gdb is initialized
# but before the mainloop is started.
proc gdbtk_tcl_preloop { } {
  global IDE_ENABLED gdb_exe_name

  set_baud

  # If we aren't using the IDE, just bring up the source window and
  # focus on main.  If we are using the IDE, we do everything in
  # response to requests.
  if {! $IDE_ENABLED} {
    tk appname gdbtk
    # If there was an error loading an executible specified on the command line
    # then we will have called pre_add_symbol, which would set us to busy,
    # but not the corresponding post_add_symbol.  Do this here just in case...
    after idle gdbtk_idle 
    set src [manage open src]
    debug "In preloop, with src: \"$src\" & error: \"$::errorInfo\""
    catch {$src location [gdb_loc main]}
    set msg ""
    catch {gdb_cmd "info files"} msg
    set line1 [lindex [split $msg \n] 0]
    set gdb_exe_name [lindex [split $line1] 2]
    set gdb_exe_name [string trim $gdb_exe_name {\".}]
    gdbtk_update
  }
}

# Update the executable name
proc set_exe_name {exe} {
  global gdb_exe_name gdb_exe_changed
  #debug "set_exe_name: exe=$exe  gdb=$gdb_exe_name"
  if {$gdb_exe_name != $exe} {
    set gdb_exe_name $exe
    set gdb_exe_changed 1    
  }
}

# Change the executable
proc set_exe {} {
  global gdb_exe_name gdb_exe_changed gdb_target_changed gdb_loaded file_done
  #debug "set_exe $gdb_exe_changed $gdb_exe_name"
  if {$gdb_exe_changed} {
    set gdb_exe_changed 0
    if {$gdb_exe_name == ""} { return }
    gdb_clear_file
    set err [catch {gdb_cmd "file '$gdb_exe_name'"} msg]
    if {$err} {
      debug "set_exe ERROR: $msg"
      set l [split $msg :]
      set errtxt [join [lrange $l 1 end] :]
      set msg "Error loading \"$gdb_exe_name\":\n"
      append msg $errtxt
      tk_messageBox -title "Error" -message $msg -icon error \
	-type ok
      set gdb_exe_name {}
      set file_done 0
      return
    } elseif {[string match {*no debugging symbols found*} $msg]} {
      tk_messageBox -icon error -default ok \
	-title "GDB" -type ok -modal system \
	-message "This executable has no debugging information."
      set gdb_exe_name {}
      set file_done 0
      return
    }

    # force new target command
    set gdb_target_changed 1
    set gdb_loaded 0
    set file_done 1
  }
}


# Update the target name
# if $prompt is 0, no dialog is displayed and
# the target command is created from preferences
# return 1 if successful, 0 if the not (the user canceled the target
# selection dialog)
proc set_target_name {targ {prompt 1}} {
  global gdb_target_name gdb_target_changed gdb_exe_changed IDE_ENABLED
  global gdb_target gdb_target_cmd gdb_pretty_name

  if {$IDE_ENABLED} {
    set target [ide_property get target-internal-name]

    set port [pref getd gdb/load/$target/port]
      if {$port == ""} {
        set port [pref get gdb/load/port]
      }
    set targ [lrep $targ "com1" $port]
      switch -regexp -- $target {
	  sim {
	      pref setd gdb/src/run_attach 1
	      pref setd gdb/src/run_load   1
              pref setd gdb/src/run_run    1
              pref setd gdb/src/run_cont   0
	  }

	  default {
	      pref setd gdb/src/run_attach 1
	      pref setd gdb/src/run_load   0
	      pref setd gdb/src/run_run    0
	      pref setd gdb/src/run_cont   1
	  }
      }

      debug "***** targ=$targ **** target=$target *****"
      if {$gdb_target_name != $targ || $gdb_target_changed} {
         set gdb_target_changed 1
         set gdb_exe_changed 1
         set gdb_target_name $targ
      }
      set gdb_target_cmd $gdb_target_name

  } elseif {$prompt} {
    set cmd_tmp $gdb_target_cmd
    set name_tmp $gdb_target_name
    set win [manage create targetsel -exportcancel 1]
    wm transient [winfo toplevel $win] .
    freeze $win
    if {$gdb_target_name == "CANCEL"} {
      set gdb_target_cmd $cmd_tmp
      set gdb_target_name $name_tmp
      return 0
    }
    set target $gdb_target_name
    set targ $gdb_target($target,cmd)
    set gdb_target_cmd $cmd_tmp
    set gdb_pretty_name $gdb_target($target,pretty-name)

    set targ_opts ""
    switch -regexp -- $gdb_target_name {
    sim {
      set targ $gdb_target_name
	set targ_opts [pref get gdb/load/sim-opts]
    }
    default {
      set port [pref getd gdb/load/$target-port]
      if {$port == ""} {
	set port [pref get gdb/load/default-port]
      }
      set portnum [pref getd gdb/load/$target-portname]
      if {$portnum == ""} {
	set portnum [pref get gdb/load/default-portname]
      }
      set hostname [pref getd gdb/load/$target-hostname]
      if {$hostname == ""} {
	set hostname [pref get gdb/load/default-hostname]
      }
      # replace "com1" with the real port name
      set targ [lrep $targ "com1" $port]
      # replace "tcpX" with hostname:portnum
      set targ [lrep $targ "tcpX" ${hostname}:${portnum}]
    }
  }

  #debug "set_target_name: $targ gdb_target_cmd=$gdb_target_cmd"
  if {$gdb_target_cmd != $targ || $gdb_target_changed} {
    set gdb_target_changed 1
      set gdb_target_cmd "$targ $targ_opts"
  }
}    
  return 1
}

# Change the target
proc set_target {} {
  global gdb_target_cmd gdb_target_changed gdb_pretty_name
  debug "set_target \"$gdb_target_changed\" \"$gdb_target_cmd\""
  global IDE_ENABLED
    if {!$IDE_ENABLED} {
  if {$gdb_target_cmd == ""} {
      if {![set_target_name ""]} {
      return 2
    }
    }  
   }

  if {$gdb_target_changed} {
    set top [winfo toplevel [lindex [manage find src] 0]]
    set saved_title [ wm title $top]
    wm title $top "Trying to communicate with target $gdb_pretty_name"
    update
    catch {gdb_cmd "detach"}
    debug "CONNECTING TO TARGET: $gdb_target_cmd"
    set err [catch {gdb_immediate "target $gdb_target_cmd"} msg ]
    if {$err} {
      if {[string first "Program not killed" $msg] != -1} {
	wm title $top $saved_title
	return 2
      }
      update
      if {$IDE_ENABLED} {
	  set dialog_title "Foundry Debugger"
	  set debugger_name "The debugger"
      } else {
	  set dialog_title "GDB"
	  set debugger_name "GDB"
      }	  
      tk_messageBox -icon error -title $dialog_title -type ok \
	-modal task -message "$msg\n\n$debugger_name cannot connect to the target board using [lindex $gdb_target_cmd 1].\nVerify that the board is securely connected and, if necessary,\nmodify the port setting with the debugger preferences."
      wm title $top $dialog_title
      return 0
    }
    set gdb_target_changed 0
    return 1
  }
  return 3
}

# This procedure is used to run an executable.  It is called when the
# IDE requests a run, or when the run button is used.
proc run_executable { {auto_start 1} } {
  global IDE_ENABLED gdb_loaded gdb_downloading gdb_target_name
  global gdb_exe_changed gdb_target_changed gdb_program_has_run
  global gdb_loaded

  debug "run_executable $auto_start $gdb_target_name"

  # This is a hack.  If the target is "sim" the opts are appended
  # to the target command. Otherwise they are assumed to be command line
  # args.  What about simulators that accept command line args?
  if {$gdb_target_name != "sim"} {
    # set args
    set gdb_args [pref getd gdb/load/$gdb_target_name-opts]
    if { $gdb_args != ""} {
      debug "set args $gdb_args"
      catch {gdb_cmd "set args $gdb_args"}
    }
  }

    if {$gdb_downloading} { return }
  if {![pref get gdb/mode]} {
    # Breakpoint mode

    set_exe

    # Attach
    if {[pref get gdb/src/run_attach]} {
      debug "Attaching...."
      while {1} {
	switch [set_target] {
	  0 {
	    # target command failed, ask for a new target name
	      debug "run_executable 1"
	      if {![set_target_name ""]} {
	      # canceled again
	      catch {gdb_cmd "delete $bp_at_main"}
	      catch {gdb_cmd "delete $bp_at_exit"}
	      debug "run_executable 2"
	      return
	    }
	  }
	  1 {
	    # success -- target changed
	      debug "run_executable 3"
	    set gdb_loaded 0
	    break 
	  }
	  2 {
	    # command cancelled by user
	      debug "run_executable 4"
	    catch {gdb_cmd "delete $bp_at_main"}
	    catch {gdb_cmd "delete $bp_at_exit"}
	    return
	  }

	  3 {
	      # success -- target NOT changed (i.e., rerun)
	      break
	  }
	}
      }
    }

    # Download
    if {[pref get gdb/src/run_load]} {
      debug "Downloading..."
      # If we have a new run request and the executable or target has changed
      # then we need to download the new executable to the new target.
      if {$gdb_exe_changed || $gdb_target_changed} {
	set gdb_loaded 0
      }
      debug "run_executable: gdb_loaded=$gdb_loaded"
      set saved_loaded $gdb_loaded
      # if the app has not been downloaded or the app has already
      # started, we need to redownload before running
      if {!$gdb_loaded} {
	if {[download_it]} {
	  # user cancelled the command
	  set gdb_loaded $saved_loaded
	  gdbtk_update
	  gdbtk_idle
	}
	if {!$gdb_loaded} {
	  # The user cancelled the download after it started
	  gdbtk_update
	  gdbtk_idle
	  return
	}
      }
    }

    # _Now_ set/clear breakpoints
    if {[pref get gdb/load/exit] && ![native_debugging]} {
      debug "Setting new BP at exit"
      catch {gdb_cmd "clear exit"}
      catch {gdb_cmd "break exit"}
    }
      
    if {[pref get gdb/load/main]} {
      debug "Setting new BP at main"
      catch {gdb_cmd "clear main"}
      catch {gdb_cmd "break main"}
    }

    # Run
    if {[pref get gdb/src/run_run] || [pref get gdb/src/run_cont]} {
      if {$auto_start} {
	if {[pref get gdb/src/run_run]} {
	  debug "Runnning target..."
	  set run run
	} else {
	  debug "Continuing target..."
	  set run cont
	}
	if {$gdb_target_name == "exec"} {
	  set run run
	}
	catch {gdb_immediate $run} msg
	#debug "msg=$msg"
      } else {
	set src [manage open src]
	catch {$src location [gdb_loc main]}
      }
    }
    gdbtk_update
    gdbtk_idle
  } else {
    # tracepoint -- need to tstart
    set gdb_running 1
    do_tstart
  } 

  return
}


# Tell GDB the baud rate.
proc set_baud {} {
  global gdb_target_name
  #set target [ide_property get target-internal-name]
  set baud [pref getd gdb/load/$gdb_target_name/baud]
  if {$baud == ""} {
    set baud [pref get gdb/load/baud]
  }
  debug "setting baud to $baud"
  catch {gdb_cmd "set remotebaud $baud"}
}

proc do_state_hook {varname ind op} {
  run_hooks state_hook $varname
}

 proc async_connect {} {
    global gdb_loaded file_done gdb_running
    debug "in async_connect"
    gdbtk_busy
    set result [set_target]
    debug "result is $result"
    switch $result {
     0 {
       gdbtk_idle
       set gdb_target_changed 1
       return 0
       }
     1 {
       set gdb_loaded 1
       if {[pref get gdb/load/check] && $file_done} {
             set err [catch {gdb_cmd "compare-sections"} errTxt]
             if {$err} {
               tk_messageBox -title "Error" -message $errTxt -icon error \
                -type ok
             }
       }
       gdbtk_idle
       tk_messageBox -title "GDB" -message "Successfully connected" -icon info           -type ok
       return 1
       } 
     2 {
        # cancelled by user
        gdbtk_idle
        tk_messageBox -title "GDB" -message "Connection Cancelled" -icon info \
         -type ok
        return 0
       }
    }     
 }



 
 proc async_disconnect {} {
   global gdb_loaded gdb_target_changed
   catch {gdb_cmd "detach"}
   # force a new target command to do something
   set gdb_loaded 0
   set gdb_target_changed 1
   set gdb_running 0 
 }

 proc do_tstart {} {
   set srcwin [lindex [manage find src] 0] 
   $srcwin.toolbar configure -runstop 1
   # Don't disable the Begin collection menu option
   #$srcwin.toolbar.m.run entryconfigure 2 -state disabled
   $srcwin.toolbar.m.run entryconfigure 3 -state normal
   if {[catch {gdb_cmd "tstart"} errTxt]} {
     tk_messageBox -title "Error" -message $errTxt -icon error \
       -type ok
    gdbtk_idle
   }
 }
 
 proc do_tstop {} {
   set srcwin [lindex [manage find src] 0] 
   $srcwin.toolbar configure -runstop 0
   $srcwin.toolbar.m.run entryconfigure 2 -state normal
   $srcwin.toolbar.m.run entryconfigure 3 -state disabled

   if {[catch {gdb_cmd "tstop"} errTxt]} {
     tk_messageBox -title "Error" -message $errTxt -icon error \
       -type ok
     gdbtk_idle
   }
 }
  

################### Initialization code #########################

# Require packages
package require Gdbtk 1.0
package require combobox 1.0

if {$tcl_platform(platform) == "unix"} {
#  tix resetoptions TK TK
#  tk_setPalette tan
  tix resetoptions TixGray [tix cget -fontset]
}

# initialize state variables

set gdb_target_changed 0
set gdb_exe_changed 0
set gdb_running 0
set gdb_downloading 0
set gdb_loaded 0
set gdb_program_has_run 0
set file_done 0
set gdb_target_name {}
set gdb_pretty_name {}
set gdb_exec {}
set gdb_target_cmd ""
    

# set traces on state variables
trace variable gdb_running w do_state_hook
trace variable gdb_downloading w do_state_hook
trace variable gdb_loaded w do_state_hook
define_hook state_hook

set download_dialog ""

# gdb_pretty_name is the name of the GDB target as it should be
# displayed to the user.
set gdb_pretty_name ""

# gdb_exe_name is the name of the executable we are debugging.  When
# using the IDE, we may know the name of the file even if it doesn't
# exist.
set gdb_exe_name ""

# Initialize readline
set gdbtk_state(readline) 0
set gdbtk_state(console) ""

# On the IDE, withdraw the main window now, so that it doesn't pop up
# as we make calls to the event manager.
if {$IDE_ENABLED} {
  wm withdraw .
}

# set up preferences
pref init

# let libide tell us how to feel
standard_look_and_feel

# now let GDB set its default preferences
pref_set_defaults

# When not using the IDE, we have to read preferences ourselves
if {!$IDE_ENABLED} {
  pref_read
}

manage init

# some initial commands to get gdb in the right mode
gdb_cmd {set height 0}
gdb_cmd {set width 0}

# If we're running the IDE, initialize it.  This should come after all
# other initialization, because we can get requests as soon as we
# register.
if {$IDE_ENABLED} {
  gdbtk_ide_init
} else {
  # gdb_target_name is the name of the GDB target; that is, the argument
  # to the GDB target command.
  set gdb_target_name [pref get gdb/load/target]
  set gdb_target_cmd ""
  set gdb_target_changed 1
  #set_target_name "" 0
}

update
gdbtk_idle
