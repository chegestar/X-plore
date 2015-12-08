#
# Local Preferences Functions
#
#
# On STARTUP:
# 1. Options database (.Xdefaults on Unix  or ? on Windows) is read
# 2. GDB prefs file "gdbtk.ini" is read
# 3. GDB init script is read
#
# Normally all preferences will be set in the prefs file, which is
# a generated file.  Hand-editing, if necessary, should be done to the
# GDB init script.
#
# when "save options" is selected, the contents of the
# preferences array is written out to the GDB prefs file.
#
# Usage:
#   pref_save
#   pref_read
# ----------------------------------------------------------------------
#   AUTHOR:  Martin M. Hunt <hunt@cygnus.com>
#   Copyright (C) 1997, 1998 Cygnus Solutions
#

proc pref_read {} {
  global prefs_init_filename env gdb_ImageDir GDBTK_LIBRARY

  set file_opened 0
  if {[file exists gdbtk.ini]} {
    if {[catch {open gdbtk.ini r} fd]} {
      debug "$fd"
      return
    }
    set prefs_init_filename gdbtk.ini
    set file_opened 1
  } elseif {[info exists env(HOME)]} {
    set name [file join $env(HOME) gdbtk.ini]
    if {[file exists $name]} {
      if {[catch {open $name r} fd]} {
	debug "$fd"
	return
      }
      set prefs_init_filename $name
      set file_opened 1
    }
  }
  
  if {$file_opened == "1"} {
    set section gdb
    while {[gets $fd line] >= 0} {
      switch -regexp -- $line {
	{^[ \t\n]*#.*} {
	  ;# comment; ignore it
	}
	
	{^[ \t\n]*$} {
	  ;# empty line; ignore it
	}
	
	{\[.*\]} {
	  regexp {\[(.*)\]} $line match section
	}
	
	{[ \t\n]*option.*} {
	  set line [string trimleft $line]
	  eval $line
	}

	default {
	  regexp "\[ \t\n\]*\(.+\)=\(.+\)" $line a name val
	  if {$section == "gdb"} {
	    pref setd gdb/$name $val
	  } elseif {$section == "global" && [regexp "^font/" $name]} {
	    set name [split $name /]
	    set f global/
	    append f [join [lrange $name 1 end] /]
	    if {[lsearch [font names] $f] == -1} {
	      # new font
	      eval define_font $f $val
	    } else {
	      # existing font
	      pref set global/font/[join [lrange $name 1 end] /] $val
	    }
	  } elseif {$section == "global"} {
	    pref setd $section/$name $val
	  } else {
	    pref setd gdb/$section/$name $val
	  }
	}
      }
    }
    close $fd
  } elseif {[info exists env(HOME)]} {
    set prefs_init_filename [file join $env(HOME) gdbtk.ini]
  } else {
    set prefs_init_filename gdbtk.ini
  }
  
  # now set global options
  set gdb_ImageDir [file join $GDBTK_LIBRARY [pref get gdb/ImageDir]]
}

# ------------------------------------------------------------------
#  PROC:  pref_save - save preferences to a file and delete window
# ------------------------------------------------------------------
proc pref_save {{win {}}} {
  global prefs_state prefs_init_filename
  
  debug "pref_save $prefs_init_filename"

  if {[catch {open $prefs_init_filename w} fd]} {
    debug "ERROR: $fd"
    return
  }
  
  puts $fd "\# GDBtk Init file"

  set plist [pref list]
  # write out global options
  puts $fd "\[global\]"
  foreach var $plist {
    set t [split $var /]
    if {[lindex $t 0] == "global"} {
      set x [join [lrange $t 1 end] /]
      set v [pref get $var]
      if {$x != "" && $v != ""} {
	puts $fd "\t$x=$v"
      }
    }
  }

  # write out gdb-global options
  puts $fd "\[gdb\]"
  foreach var $plist {
    set t [split $var /]
    if {[lindex $t 0] == "gdb" && [lindex $t 2] == ""} {
      set x [lindex $t 1]
      if {$x != ""} {
	puts $fd "\t$x=[pref get $var]"
      }
    }
  }

  #now loop through all sections writing out values
  foreach section {load console src reg stack locals watch bp search process} {
    puts $fd "\[$section\]"
    foreach var $plist {
      set t [split $var /]
      if {[lindex $t 0] == "gdb" && [lindex $t 1] == $section} {
	set x [lindex $t 2]
	set v [pref get $var]
	if {$x != "" && $v != ""} {
	  puts $fd "\t$x=$v"
	}
      }
    }
  }
  close $fd

  if {$win != ""} {
    $win delete
  }
}


# ------------------------------------------------------------------
#  PROC:  pref_set_defaults - set up default values
# ------------------------------------------------------------------
proc pref_set_defaults {} {
  global GDBTK_LIBRARY tcl_platform gdb_ImageDir

  # Global defaults
  pref define global/debug                0

  # Gdb global defaults
  pref define gdb/ImageDir                images2
  set gdb_ImageDir [file join $GDBTK_LIBRARY [pref get gdb/ImageDir]]
  pref define gdb/font_cache              ""
  pref define gdb/mode                    0;     # 0 sync, 1 async

  # set download and execution options
  pref define gdb/load/target	exec
  pref define gdb/load/verbose 0
  pref define gdb/load/main 1
  pref define gdb/load/exit 1
  pref define gdb/load/check 0     
  pref define gdb/load/baud 38400
  if {$tcl_platform(platform) == "windows"}  {
    pref define gdb/load/port com1
  } else {
    pref define gdb/load/port "/dev/ttyS0"
  }

  # Console defaults
  pref define gdb/console/prompt          "(gdb) "
  pref define gdb/console/deleteLeft      1
  pref define gdb/console/prompt_fg       DarkGreen
  pref define gdb/console/error_fg        red
  pref define gdb/console/font            src-font

  # Source window defaults
  pref define gdb/src/PC_TAG              green
  pref define gdb/src/STACK_TAG           gold
  pref define gdb/src/BROWSE_TAG          \#9595e2
  pref define gdb/src/active              1
  pref define gdb/src/handlebg            red
  pref define gdb/src/bp_fg               red
  pref define gdb/src/temp_bp_fg          orange
  pref define gdb/src/disabled_fg         black
  pref define gdb/src/font                src-font
  pref define gdb/src/break_fg            black
  pref define gdb/src/source2_fg          navy
  pref define gdb/src/variableBalloons    1
  pref define gdb/src/trace_fg            magenta
  pref define gdb/src/tab_size            4
  pref define gdb/src/linenums		  1

  # Define the run button's functions. These are defined here in case
  # we do a "run" with an exec target (which never causes target.tcl to 
  # source)...
  pref define gdb/src/run_attach          0
  pref define gdb/src/run_load            0
  pref define gdb/src/run_run             1
  pref define gdb/src/run_cont            0

  # set up src-font
  set val [pref get global/font/fixed]
  eval font create src-font $val

  # Trace the global/font/fixed preference
  pref add_hook global/font/fixed pref_src-font_trace

  # Variable Window defaults
  pref define gdb/variable/font           src-font
  pref define gdb/variable/highlight_fg   blue
  pref define gdb/variable/disabled_fg    gray

  # Stack Window
  pref define gdb/stack/font              src-font

  # Register Window
  pref define gdb/reg/highlight_fg        blue

  # Global Prefs Dialogs
  pref define gdb/global_prefs/save_fg    red
  pref define gdb/global_prefs/message_fg white
  pref define gdb/global_prefs/message_bg red

  # Search
  pref define gdb/search/static           0
  pref define gdb/search/last_symbol      ""
  pref define gdb/search/use_regexp       0
}

# This traces the global/fixed font and forces src-font to
# to be the same.
proc pref_src-font_trace {varname val} {
  eval font configure src-font $val
}
