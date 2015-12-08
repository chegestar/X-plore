# debug.tcl - Handy functions for debugging.
# Copyright (C) 1997, 1998 Cygnus Solutions.
# Written by Tom Tromey <tromey@cygnus.com>.
# and Martin Hunt <hunt@cygnus.com>

# Please note that I adapted the re-source code from some code I wrote
# elsewhere, for non-Cygnus reasons.  Don't complain to me if you see
# something like it somewhere else.  - Tom.


# State for this module.
defarray DEBUG_state {
  window {}
  log_file {}
}

# Call this to display something interesting.  Although it will work
# to leave debug statements in all the time, you probably don't want
# to.
proc debug {what} {
  global DEBUG_state tixOption

  if {! [pref get global/debug]} then {
    return
  }

  if {$DEBUG_state(log_file) != ""} then {
    puts $DEBUG_state(log_file) $what
    return
  }

  if {$DEBUG_state(window) == ""
      || ! [winfo exists $DEBUG_state(window)]} then {
    set DEBUG_state(window) .[gensym]

    toplevel $DEBUG_state(window)
    tixScrolledText $DEBUG_state(window).t -scrollbar auto 
    [$DEBUG_state(window).t subwidget text] config -wrap none \
      -bg $tixOption(bg)
    pack $DEBUG_state(window).t -expand 1 -fill both

    wm title $DEBUG_state(window) Debug
    wm iconname $DEBUG_state(window) Debug
  }

  set t [$DEBUG_state(window).t subwidget text]
  $t insert end $what\n
  $t see insert
}

# Like debug, but print all args.
proc debug_args {args} {
    debug [join $args \n]
}


# Call this to turn on/off logging to a file.  When logging, all debug
# calls will be redirected to the file.  If FILE is the empty string,
# logging will be turned off.
proc debug_log {file} {
  global DEBUG_state

  if {$DEBUG_state(log_file) != ""} then {
    catch {close $DEBUG_state(log_file)}
  }

  if {$file == ""} then {
    set DEBUG_state(log_file) ""
  } else {
    set DEBUG_state(log_file) [open $file w]
    fconfigure $DEBUG_state(log_file) -buffering line
  }
}


# Helper for re-sourcing.  This records the timestamp of the sourced
# file.
proc DEBUG_after_source {code result filename args} {
  global DEBUG_state

  if {[file exists $filename]} {
    set DEBUG_state([list file $filename]) [file mtime $filename]
  }
}

# Initialize re-source module.
proc init_re_source {} {
  advise after source DEBUG_after_source
}

# Re-load all source files that have changed.
proc re_source {} {
  global DEBUG_state

  foreach item [array names DEBUG_state] {
    set file [lindex $item 1]
    if {! [file exists $file]} {
      unset DEBUG_state($file)
    } elseif {[file mtime $file] > $DEBUG_state($file)} {
      uplevel \#0 [list source $file]
    }
  }
}
