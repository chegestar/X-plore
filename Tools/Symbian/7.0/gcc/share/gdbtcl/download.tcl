#
# Download
# ----------------------------------------------------------------------
# Implements Download window
#
#   PUBLIC ATTRIBUTES:
#
#   METHODS:
#
#     config ....... used to change public attributes
#
#     PRIVATE METHODS
#
# ----------------------------------------------------------------------
#   AUTHOR:  Martin M. Hunt <hunt@cygnus.com>
#   Copyright (C) 1997, 1998 Cygnus Solutions
#

itcl_class Download {
  # ------------------------------------------------------------------
  #  CONSTRUCTOR - create new toolbar preferences dialog
  # ------------------------------------------------------------------
  constructor {config} {
    global download_cancel_ok total_bytes
    global download_start_time download_verbose gdb_pretty_name
    debug DOWNLOAD
    if {$download_verbose} {
      #
      #  Create a window with the same name as this object
      #
      set class [$this info class]
      @@rename $this $this-tmp-
      @@frame $this -class $class 
      @@rename $this $this-win-
      @@rename $this-tmp- $this
      set top [winfo toplevel $this]
      wm withdraw $top
      wm title $top "Download Status"
      add_hook download_progress_hook "$this update_download"
    } 
    foreach src [manage find src] {
      if !$download_verbose {
	manage raise $src
      }
      $src.toolbar configure -runstop 2
      @@update idletasks
    }

    if {[catch {gdb_load_info $filename} val]} {
      set result "$filename: $val"
      tk_dialog .load_warn "" "$result"  error 0 Ok
      $this delete
      return
    }

    set i 0
    set total_bytes 0
    set section(names) {}
    foreach x $val {
      set s [lindex $x 0]
      lappend section(names) $s
      set section($s) $i
      set b [lindex $x 1]
      set bytes($i) [expr double($b)]
      incr total_bytes $b
      incr i
    }

    set last_num [expr {$i - 1}]
    set last_section ""
    set download_cancel_ok 0
    set download_start_time [clock seconds]
    
    if {$download_verbose} {
      label $this.download -text "Downloading $filename to $gdb_pretty_name"
      label $this.stat
      set f [frame $this.f]
      set i 0
      foreach x $val {
	tixMeter $f.meter$i -value 0 -text 0 
	label $f.sec$i -text [lindex $x 0] -anchor w
	label $f.num$i -text [lindex $x 1] -anchor e
	grid $f.sec$i $f.meter$i $f.num$i -padx 4 -pady 4 -sticky news
	incr i
      }
      grid $this.download -padx 5 -pady 5
      grid $this.stat -padx 5 -pady 5
      grid $this.f -padx 5 -pady 5

      button $this.cancel -text Cancel -command "$this cancel" \
	-state active -width 10
      grid $this.cancel -padx 4 -pady 4 
      grid $this
      
      # make window a transient with a source window as parent
      set src [lindex [manage find src] 0]
      if {$src != ""} {
	wm transient $top [winfo toplevel $src]
      }
    }
  }
 
  # ------------------------------------------------------------------
  #  METHOD:  update_download - update the download meters
  # ------------------------------------------------------------------
  method update_download { sec num tot } {

    # Loop through all the sections, marking each as either done or
    # updating its meter. This will mark all previous sections prior to
    # SEC as complete. 
    foreach s $section(names) {
      set i $section($s)

      if {$s == $sec} {
	$this.f.meter$i config -value [expr {$num / $bytes($i)}] -text $num
	break
      } else {
	if {[expr {double([$this.f.meter$i cget -value])}] != 1.0} {
	  $this.f.meter$i config -value 1.0 -text [expr {int($bytes($i))}]
	}
      }
    }
    @@update
  }

  # ------------------------------------------------------------------
  #  METHOD:  done - notification that the download is really complete
  # ------------------------------------------------------------------
  method done { {msg ""} } {
    global total_bytes download_verbose download_start_time
    bell
    if {$download_verbose} {
      if {$msg == ""} {
	# download finished
	set secs [expr {[clock seconds] - $download_start_time}]
	if {$secs == 0} { incr secs }
	$this.cancel config -state disabled
	set bps [expr {8 * $total_bytes / $secs / 1000}]
	$this.stat config -text "$total_bytes bytes in $secs seconds ($bps kbps)"
	
	# set all indicators to FULL
	foreach sec $section(names) {
	  set i $section($sec)
	  $this.f.meter$i config -value 1.0 -text "DONE"
	}
      } else {
	# download failed
	if {$msg != "CANCEL"} {
	  $this.stat config -text $msg
	}
      }

      # enable OK button
      $this.cancel config -state active -text OK -command "$this delete"
      @@update
    }
    
    foreach src [manage find src] {
      if {$msg == "CANCEL"} {
	$src download_progress CANCEL 1 1
      } else {
	$src download_progress DONE 0 $total_bytes $msg
      }
    }
    
    delete
  }

  # ------------------------------------------------------------------
  #  METHOD:  cancel - cancel the download
  # ------------------------------------------------------------------
  method cancel {} {
    global download_cancel_ok
    debug "canceling the download"
    set download_cancel_ok 1
  }
  
  # ------------------------------------------------------------------
  #  DESTRUCTOR - destroy window containing widget
  # ------------------------------------------------------------------
  destructor {
    global download_verbose download_dialog
    set download_dialog ""
    if {$download_verbose} {
      remove_hook download_progress_hook "$this update_download"
      manage delete $this 1
      destroy $this
      destroy $top
    } else {
      destroy $this
    }
  }
  
  # ------------------------------------------------------------------
  #  METHOD:  config - used to change public attributes
  # ------------------------------------------------------------------
  method config {config} {}

  #
  #  PROTECTED DATA
  #
  protected section
  protected bytes
  protected top ""
  protected last_num
  protected last_section

  #
  # Public varaibles
  #
  public filename
}

proc do_download_hooks {} {
  global download_timer

  set download_timer(ok) 1
}

proc download_hash { section num } {
  global download_cancel_ok total_bytes download_timer
  #debug "hash: $section $num download_cancel_ok=$download_cancel_ok"
  update

  # Only run the timer at discrete times...
  if [info exists download_timer(timer)] {
    after cancel $download_timer(timer)
  }
  
  set download_timer(timer) [after 333 do_download_hooks]
  if {![info exists download_timer(ok)] || $download_timer(ok)} {
    run_hooks download_progress_hook $section $num $total_bytes
    unset download_timer(timer)
    set download_timer(ok) 0
  }

  return $download_cancel_ok
}

# Download the executable.
proc download_it { } {
  global gdb_exe_name gdb_downloading gdb_loaded download_cancel_ok
  global gdb_target_name download_dialog download_verbose gdb_pretty_name
  global gdb_target_changed gdb_running IDE_ENABLED

  debug "download_it: exe=$gdb_exe_name downloading=$gdb_downloading"
  debug "    loaded=$gdb_loaded target=$gdb_target_name running=$gdb_running"

  if {$gdb_downloading} {
    return 0
  }

  set gdb_downloading 1
  set gdb_loaded 0
  # Make sure the source window has had time to be created
  update

  gdbtk_busy

  # always reissue a target command before attempting
  # the download.  This shouldn't be necessary but it is
  # because otherwise we can hang if the target is reset 
  # after the last target command was issued.
  if {$gdb_target_name != "sim"} {
    set gdb_target_changed 1
    set_baud
  }

  if {!$IDE_ENABLED} {
    set_exe
    switch [set_target] {
      0 {
	# target command failed
	set gdb_downloading 0
	gdbtk_idle
	return 0
      }
      2 {
	# command cancelled by user
	set gdb_downloading 0
	if {$gdb_running} {
	  # Run the idle hooks (free the UI)
	  gdbtk_update
	  gdbtk_idle
	} else {
	  gdbtk_idle
	}
	return 1
      }
    }
  }

  if {! [file exists $gdb_exe_name]} {
    tk_messageBox -icon error -title "Foundry Debugger" -type ok -modal task\
      -message "Request to download non-existent executable $gdb_exe_name"
    set gdb_downloading 0
    gdbtk_idle
    return 0
  }

  if {$IDE_ENABLED} {
    if {! [ide_build up-to-date $gdb_exe_name]} {
      set msg "$gdb_exe_name is out of date.  Do you still want to download $gdb_exe_name to the target \"$gdb_pretty_name\"?"
      set result [tk_messageBox -default no -icon warning \
		    -message $msg -title "Foundry Debugger" -type yesno \
		    -modal task]
      if {$result == "no"} {
	set gdb_downloading 0
	gdbtk_idle
	return 0
      }
    }
  }
    
  if {$download_dialog != ""} {
    $download_dialog delete
  }

  debug "downloading $gdb_exe_name"

  if {$IDE_ENABLED} {
    set target [ide_property get target-internal-name]
  } else {
    set target $gdb_target_name
  }

  #debug "target=$target verbose=[pref getd gdb/load/$target-verbose]"
  if {[pref getd gdb/load/$target-verbose] == "1"} {
    debug "verbose"
    set download_verbose 1
    set download_dialog [manage create load -filename $gdb_exe_name]
  } else {
    set download_verbose 0
    set download_dialog "download"
    Download $download_dialog -filename $gdb_exe_name
  }

  set download_error ""
  debug "starting load"
  update idletasks
  if {[catch {gdb_cmd "load $gdb_exe_name"} errTxt]} {
    debug "load returned $errTxt"
    if {[regexp -nocase cancel $errTxt]} {
      set download_error "CANCEL"
    } else {
      set download_error $errTxt
    }
    set download_cancel_ok 1
  }

  debug "Done loading"
  set gdb_downloading 0
  if {$download_cancel_ok} {
    set gdb_loaded 0
    if {$download_dialog != ""} {
      $download_dialog done $download_error
    }
  } else {
    set gdb_loaded 1
    if {$download_dialog != ""} {
      $download_dialog done
    }
  }

  set download_cancel_ok 0
  set download_dialog ""

  gdbtk_idle
  return 0
}
