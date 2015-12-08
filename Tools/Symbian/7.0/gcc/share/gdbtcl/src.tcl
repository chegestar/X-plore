#
# SrcWin
# ----------------------------------------------------------------------
# Implements a source display widget using primitive widgets as the building
# blocks.
#
# The main display for SrcWin is a SrcTextWin widget.  This file should contain
# all the code for controlling the SrcTextWin.  SrcTextWin should just display the
# requested file and lines, without having to be very intelligent.  If there
# are problems, error codes should be returned and SrcWin should figure out what
# to do.
#
# ----------------------------------------------------------------------
#   Copyright (C) 1997, 1998 Cygnus Solutions
#

itcl_class SrcWin {
  # ------------------------------------------------------------------
  #  CONSTRUCTOR - create new source window
  # ------------------------------------------------------------------
  constructor {config} {
    global IDE_ENABLED tcl_platform
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

    if {$IDE_ENABLED} {
      wm title $top "Foundry Debugger"
    } else {
      wm title $top "GDB"
    }
    update_title ""
    
    # On Windows, create a sizebox.
    if {$tcl_platform(platform) == "windows"} {
      ide_sizebox $this.sizebox
    }
  
    set Tracing [pref get gdb/mode]
    set current(filename) ""

    build_win

    # add hooks
    add_hook gdb_update_hook "$this update"
    add_hook gdb_busy_hook "$this busy"
    add_hook gdb_idle_hook "$this idle"
    add_hook gdb_no_inferior_hook "$this no_inferior"
    add_hook download_progress_hook "$this download_progress"
    add_hook state_hook "$this set_state"

    after idle [list wm deiconify $top]
  }

  # ------------------------------------------------------------------
  #  METHOD:  build_win - build the main source window
  # ------------------------------------------------------------------
  method build_win {} {
    global gdb_downloading gdb_running gdb_loaded

    set twin [SrcTextWin $this.p -Tracing $Tracing]

    frame $this.stat

    combobox::combobox $this.stat.name -maxheight 15 -font src-font\
      -command "$this name"
    set need_files 1

    combobox::combobox $this.stat.func -maxheight 15 -font src-font\
      -command "$this goto_func"

    combobox::combobox $this.stat.mode -width 9 -editable false -font src-font \
      -command "$this mode"
    $this.stat.mode list insert end SOURCE
    $this.stat.mode list insert end ASSEMBLY
    $this.stat.mode list insert end MIXED
    $this.stat.mode list insert end SRC+ASM

    label $this.status -relief sunken -bd 3 -font global/status

    # Search/download progress frame
    frame $this.stat.frame
    entry $this.stat.frame.search -bd 3 -font src-font -width 10
    bind_plain_key $this.stat.frame.search Return [list $this search forwards]
    bind_plain_key $this.stat.frame.search Shift-Return [list $this search backwards]
    canvas $this.stat.frame.progress -relief sunken -borderwidth 2 \
      -highlightthickness 0 -takefocus 0 -width 100 -height 0 -confine 1
    $this.stat.frame.progress create rectangle 0 0 0 \
      [winfo height $this.stat.frame.progress] -outline blue -fill blue -tags rect
 
    grid $this.stat.name $this.stat.func $this.stat.mode x  \
      -sticky wns -padx 10
    grid $this.stat.frame -column 4 -row 0 -sticky news -padx 10
    grid columnconfigure $this.stat 3 -weight 1
    pack $this.stat.frame.search -fill x -expand yes

    set_execution_status

    # build source toolbar
    GDBSrcBar $this.toolbar -source $this \
      -updatecommand [list $this toggle_updates] \
      -updatevalue $do_updates

    if {[winfo exists $this.sizebox]} {
      grid $this.toolbar -row 0 -columnspan 2 -sticky new
      grid $this.p -row 1 -columnspan 2 -sticky news
      grid $this.status -row 2 -columnspan 2 -sticky new
      grid $this.stat $this.sizebox -row 3 -sticky news
    } else {
      grid $this.toolbar -row 0 -sticky new
      grid $this.p -row 1 -sticky news
      grid $this.status -row 2 -sticky new
      grid $this.stat -row 3 -sticky news
    }

    grid rowconfigure $this 0 -weight 0
    grid rowconfigure $this 1 -weight 1
    grid rowconfigure $this 2 -weight 0
    grid rowconfigure $this 3 -weight 0
    grid columnconfigure $this 0 -weight 1

    # balloon help -- FIXME!! Requires knowledge of combobox internals
    balloon register $this.stat.name.entry "Current file name"
    balloon register $this.stat.name.button "Current file name"
    balloon register $this.stat.func.entry "Current function name"
    balloon register $this.stat.func.button "Current function name"
    balloon register $this.stat.mode.entry "Source mode"
    balloon register $this.stat.mode.button "Source mode"
    balloon register $this.stat.frame.search "Search for text"
    balloon variable $this.status ${twin}_balloon

    $this.stat.mode configure -value [$twin mode_get]

    # time to load the widget with a file.
    # If this is a new widget and the program is
    # not yet being debugged, load the file with "main" in it.
    if {$gdb_running} {
      update
    } else {
      if {[catch {gdb_loc main}] == 0} {
	location BROWSE_TAG [gdb_loc main]
      }
    }
  }


  # ------------------------------------------------------------------
  #  METHOD:  set_state - do things when program state changes
  # ------------------------------------------------------------------
  method set_state {varname} {
    global gdb_running gdb_downloading gdb_loaded gdb_program_has_run
    debug "SET_STATE: $varname l=$gdb_loaded d=$gdb_downloading r=$gdb_running"

    if {$varname == "gdb_loaded" && $gdb_loaded == 1} {
      set gdb_program_has_run 0
      #set current(filename) ""
      return
    }

    if {$gdb_running} {
      set state normal
      set gdb_program_has_run 1
    } else {
      set state disabled
    }
    if {!$Tracing} {
      $twin SetRunningState $state
    }
  }
  
  # ------------------------------------------------------------------
  #  METHOD:  download_progress - update the progress meter when downloading
  # ------------------------------------------------------------------
  method download_progress { section num tot {msg ""} } {
    global download_start_time download_cancel_ok gdb_loaded

    if {$last_section_start == 0} {
      pack forget $this.stat.frame.search
      pack $this.stat.frame.progress -fill both -expand yes
      @@update idletasks
    }

    if {$section == "DONE"} {
      set last_done $tot
      if {$gdb_loaded} {
	# loaded something
	set secs [expr {[clock seconds] - $download_start_time}]
	if {$secs} {
	  set bps [expr {8 * $tot / $secs}]
	  set_status "DOWNLOAD FINISHED: $tot bytes in $secs seconds ($bps bits per second)"
	} else {
	  set_status "DOWNLOAD FINISHED"
	}
      }
    } elseif {$section != "CANCEL"} {
      if {$section != $last_section} {
	set last_section $section
	set last_section_start $last_done
      }
      set last_done [expr {$last_section_start + $num}]
      set_status "Downloading section $section - $num bytes"
    }

    set canvas $this.stat.frame.progress
    set height [winfo height $canvas]
    if {$last_done} {
      set width [winfo width $canvas]
      set rw [expr {double ($last_done) * $width / $tot}]
      $canvas coords rect 0 0 $rw $height
      @@update
    }

    if {$last_done == $tot || $section == "CANCEL"} {
      $this.toolbar configure -runstop 0
      if {!$gdb_loaded} {
	update
	# errored or canceled
	if {$msg != ""} {
	  set_status "DOWNLOAD FAILED: $msg"
	} else {
	  set_status "DOWNLOAD CANCELLED"
	}
	$canvas coords rect 0 0 0 $height
	@@update idletasks
      }

      set last_section ""
      set last_done 0
      set last_section_start 0

      pack forget $this.stat.frame.progress
      pack $this.stat.frame.search -fill x -expand yes
      @@update idletasks
    }
  }

  # ------------------------------------------------------------------
  #  METHOD:  reconfig - used when preferences change
  # ------------------------------------------------------------------
  method reconfig {} {
    $this.toolbar reconfig
    $twin reconfig
  }


  # ------------------------------------------------------------------
  #  PRIVATE METHOD:  name - filename combobox callback
  #  This is only called when the user edits the name combobox.
  #  It is the only way that files can be inserted into the file list
  #  once the debugger is started. 
  # ------------------------------------------------------------------
  method name {w {val ""}} {
    global _files
    debug "name $w $val"
    if {$val != ""} {
      if {![info exists _files(short,$val)]} {
	if {![info exists _files(full,$val)]} {
	  set full [gdb_find_file $val]
	  if {$full == ""} {
	    set_status "Cannot find source file \"$val\""
	    $this.stat.name entryset [lindex [file split $current(filename)] end]
	    return
	  }
	  set _files(short,$full) $val
	  set _files(full,$val) $full
	  #$this.stat.name addhistory $val
	}
	set full $_files(full,$val)
      } else {
	set full $val
	set val $_files(short,$full)
      }
      $this.stat.name entryset $val
      location BROWSE_TAG [list $val "" $full 0 0 0]
    }
  }
  
  # ------------------------------------------------------------------
  #  PRIVATE METHOD:  toggle_updates - update toggle callback
  # ------------------------------------------------------------------
  method toggle_updates {value} {
    if {$value} {
      add_hook gdb_update_hook "$this update"
      add_hook gdb_busy_hook "$this busy"
    } else {
      remove_hook gdb_update_hook "$this update"
      remove_hook gdb_busy_hook "$this busy"
    }
    # save state in do_updates so it will be preserved
    # in window reconfigs
    set do_updates $value
  }

  # ------------------------------------------------------------------
  #  PRIVATE METHOD:  goto_func - function combobox callback
  # ------------------------------------------------------------------
  method goto_func {w {val ""}} {
    if {$val != ""} {
      set mang 0
      if {[info exists _mangled_func($val)]} {
	set mang $_mangled_func($val)
      }
      if {$mang} {
	set loc $val
      } else {
	set fn [lindex [@@file split $current(filename)] end]
	set loc $fn:$val
      }
      debug "GOTO $loc"
      if {![catch {gdb_loc $loc} result]} {
	location BROWSE_TAG [gdb_loc $loc]
      } else {
	debug "SrcWin::goto_fun: gdb_loc returned \"$result\""
      }
    }
  }
  
  # ------------------------------------------------------------------
  #  METHOD:  config - used to change public attributes
  # ------------------------------------------------------------------
  method config {config} {}
  
  # ------------------------------------------------------------------
  #  DESTRUCTOR - destroy window containing widget
  # ------------------------------------------------------------------
  destructor {
    # remove breakpoint hook
    remove_hook gdb_update_hook "$this update"
    remove_hook gdb_busy_hook "$this busy"
    remove_hook gdb_no_inferior_hook "$this no_inferior"
    remove_hook gdb_idle_hook "$this idle"
    remove_hook download_progress_hook "$this download_progress"
    remove_hook state_hook "$this set_state"

    $this.toolbar delete
    destroy $this
    destroy $top
  }    


  # ------------------------------------------------------------------
  #  METHOD:  FillNameCB - fill the name combobox
  # ------------------------------------------------------------------
  method FillNameCB {} {
    global IDE_ENABLED _files
    if {$IDE_ENABLED} {
      # in the IDE, get the file list from vmake
      set allfiles [ide_property get vmake/source-files]
      set root [ide_property get project-root]
      if {$tcl_platform(platform) == "windows"} {
	# root will be in the form C:/foo and GDB
	# internally uses //c/foo, so we'll conform to what GDB wants
	set root [ide_cygwin_path to_posix $root]
      }
      foreach f $allfiles {
	# vmake gives us basenames, but GDB will need full 
	# pathnames so the following is necessary
	set fullname [@@file join $root $f]
	set _files(full,$f) $fullname
	set _files(short,$fullname) $f
	$this.stat.name list insert end $f
      }
    } else {
      set allfiles [gdb_listfiles]
      foreach f $allfiles {
	#set fullname [gdb_find_file $f]
	#set _files(full,$f) $fullname
	#set _files(short,$fullname) $f
	$this.stat.name list insert end $f
      }
    }
    set need_files 0
  }


  # ------------------------------------------------------------------
  #  METHOD:  FillFuncCB - fill the function combobox
  # ------------------------------------------------------------------
  method FillFuncCB {name} {
    $this.stat.func list delete 0 end
    if {$name != ""} {
      set maxlen 10
      if {[catch {gdb_listfuncs $name} listfuncs]} {
	tk_messageBox -icon error -default ok \
	  -title "GDB" -type ok -modal system \
	  -message "This file can not be found or does not contain\ndebugging information."
	set_name ""
	return
      }
      foreach f $listfuncs {
	lassign $f func mang
	set _mangled_func($func) $mang
	$this.stat.func list insert end $func
	if {[string length $func] > $maxlen} {
	  set maxlen [string length $func]
	}
      }
      $this.stat.func configure -width [expr $maxlen + 1]
    }
  }

  # ------------------------------------------------------------------
  #  METHOD:  location - update the location displayed
  #
  #  a linespec looks like this:
  #  0: basename of the file
  #  1: function name
  #  2: full filename
  #  3: source line number
  #  4: address
  #  5: current PC - which will often be the same as address, but not when
  #  we are browsing, or walking the stack.
  #
  # linespec will be "{} {} {} 0 0x0 0x0" when GDB has not started debugging.
  # ------------------------------------------------------------------
  method location {tag linespec} {
    global gdb_running gdb_exe_name _files tcl_platform

    # We need to keep track of changes to the line, filename, function name
    # and address so we can keep the widgets up-to-date.  Otherwise we
    # basically pass things through to the SrcTextWin location method.

    debug "running=$gdb_running tag=$tag linespec=$linespec"    
    lassign $linespec foo funcname name line addr pc_addr

    # need to call this to update running state
    set_execution_status $line $addr

    # "update" doesn't set the tag so we do it here
    if {$tag == ""} {
      if {$addr == $pc_addr} {
	set tag PC_TAG
      } else {
	set tag STACK_TAG
      }
    }
    
    if {!$gdb_running} {
      # When we are not yet debugging, we need to force something
      # to be displayed, so we choose to find function "main" and
      # display the file with it.
      set tag BROWSE_TAG
      debug "not running: name=$name funcname=$funcname line=$line"
      if {$name == ""} {
	if {[catch {gdb_loc main} linespec]} {
	  # no "main" function found
	  return 
	}
	lassign $linespec foo funcname name line addr pc_addr
	debug "new linespec=$linespec"    
      }
    }

    # update file and function combobox
    if {$name != $current(filename)} {
      set_name $name
      FillFuncCB $name
    }

    # set address and line widgets
    $this.toolbar configure -address $addr -line $line

    # set function combobox
    $this.stat.func entryset $funcname

    # call SrcTextWin::location
    $twin location $tag $name $funcname $line $addr $pc_addr

    set current(filename) $name
  }

  # ------------------------------------------------------------------
  #  METHOD:  stack - handle stack commands
  # ------------------------------------------------------------------
  method stack {cmd} {
    if {$cmd == "bottom"} {
      set cmd "frame 0"
    }
    gdbtk_busy
    if {[catch {gdb_cmd "$cmd"} message]} {
      debug "STACK ERROR: $message"
    }
    gdbtk_update
    gdbtk_idle
  }

  # ------------------------------------------------------------------
  #  METHOD:  update - update widget when PC changes
  # ------------------------------------------------------------------
  method update {} {
    if {[catch {gdb_loc} loc]} {
      set_execution_status
    } else {
      debug "SRC:update loc=$loc"
      # See if name combobox needs filled.
      if {$need_files} {
	FillNameCB
      }
      location "" $loc
    }
  }

  # ------------------------------------------------------------------
  #  METHOD:  idle - callback for gdbtk_idle
  #  Called when the target is idle, so enable all buttons.
  # ------------------------------------------------------------------
  method idle {} {
    $this.toolbar configure -runstop 0
    enable_ui 1
  }

  # ------------------------------------------------------------------
  #  METHOD:  mode - set mode to SOURCE, ASSEMBLY, MIXED, SRC+ASM
  # ------------------------------------------------------------------
  method mode {w new_mode {go 1}} {
    $this.stat.mode entryset $new_mode
    $twin mode_set $new_mode $go
    $this.toolbar configure -displaymode $new_mode
  }

  # ------------------------------------------------------------------
  #  METHOD:  update_title - update title bar
  # ------------------------------------------------------------------
  method update_title {name} {
    global IDE_ENABLED gdb_target_name gdb_exe_name
    global gdb_target_changed gdb_exe_changed gdb_pretty_name
    set fn [lindex [@@file split $name] end]
    if {$fn == ""} {
      set prefix ""
    } else {
      set prefix "$fn - "
    }
    if {$IDE_ENABLED} {
      if {!$gdb_target_changed && !$gdb_exe_changed} {
	wm title $top "${prefix}Debugger: $gdb_exe_name on $gdb_pretty_name"
      }
    } else {
      wm title $top "${prefix}Source Window"
    }
  }
  
  # ------------------------------------------------------------------
  #  METHOD:  busy - disable things when gdb is busy
  # ------------------------------------------------------------------
  method busy {} {
    global gdb_loaded
    #debug "SrcWin::busy: $gdb_loaded"

    enable_ui 0
    if {$Running} {
      $this.toolbar configure -runstop 1
      if {$gdb_loaded} {
	set_status "Program is running."
      } 
    } else {
      $this.toolbar configure -runstop 3
    }
  }

  # ------------------------------------------------------------------
  #  METHOD:  set_name - set the name in the name combobox and in the title
  # ------------------------------------------------------------------
  method set_name { val {found 1} } {
    global _files
    update_title $val
    if {![info exists _files(short,$val)]} {
      if {![info exists _files(full,$val)]} {
	# not in our list; just display basename
	$this.stat.name entryset [lindex [@@file split $val] end]
	return
      }
    } else {
      set val $_files(short,$val)
    }
    if {$found} {
      $this.stat.name entryset $val
    } else {
      $this.stat.name entryset "$val (not found)"
    }
  }

  # ------------------------------------------------------------------
  #  METHOD:  set_status - write a message to the status line.
  #  If "tmp" is set, the status change will not be saved.
  # ------------------------------------------------------------------
  protected saved_msg ""	;# static

  method set_status { {msg ""} {tmp 0} } {
    if {$tmp} {
      $this.status configure -text $msg
      return
    }
    if {$msg != ""} {
      set saved_msg $msg
    }
    $this.status configure -text $saved_msg
  }

  # ------------------------------------------------------------------
  #  METHOD:  set_execution_status - write the current execution state 
  #  to the status line
  # ------------------------------------------------------------------
  method set_execution_status { {line ""} {pc ""}} {
    global gdb_running gdb_loaded gdb_program_has_run

    if {![gdb_target_has_execution]} {
      if {$gdb_running} {
	set gdb_running 0
	# tell text window program is no longer running
	$twin ClearTags
      }
      if {$gdb_loaded} {
	if {$gdb_program_has_run} {
	  set message "Program terminated. 'Run' will restart."
	  set gdb_running 0
	} else {
	  set message "Program is ready to run."
	}
      } else {
	set message "No program loaded."
      }
    } else {
      if {!$gdb_running} {
	set gdb_running 1
      }
      
      # only do a gdb_loc if we have to
      if {$line == "" && $pc == ""} {
	if {[catch {gdb_loc} loc] == 0} {
	  set line [lindex $loc 3] 
	  set pc [lindex $loc 4]
	}
      }

      if {$line == "" || $line == 0} {
	if {$pc == "" || $pc == 0} {
          if {$Tracing} {
            set message "Ready."
           } else {
	    set message "Program stopped."
	   }
	} else {
	  set message [format "Program stopped at %\#08x" $pc]
	}
      } else {
	if {$Tracing} {
	  set msg "Inspecting trace"
	} else {
	  set msg "Program stopped"
	}
	switch [$twin mode_get] {
	  ASSEMBLY {set message [format "$msg at %\#08x" $pc] }
	  MIXED {set message [format "$msg at line $line, %\#08x" $pc] }
	  SRC+ASM {set message [format "$msg at line $line, %\#08x" $pc] }
	  default {set message "$msg at line $line" }
	}
      }
    }
    set_status $message
}

  
  # ------------------------------------------------------------------
  #  METHOD:  print - print the contents of the text widget
  # ------------------------------------------------------------------
  method print {} {
    global IDE_ENABLED    
    if {$IDE_ENABLED} {
      idewindow_freeze $top
    }

    # Call the SrcTextWin's print method
    $twin print $top

    if {$IDE_ENABLED} {
      idewindow_thaw $top
    }
  }
  
  # ------------------------------------------------------------------
  # METHOD:   enable_ui
  #              Enable all UI elements for user interaction.
  # ------------------------------------------------------------------
  method enable_ui { on } {
    #debug "SrcWin::enable_ui $on"
    if {$on} {
      set Running 0
      set state normal
      set glyph ""
    } else {
      if {!$NoRun} {set Running 1}
      set state disabled
      set glyph watch
    }
    # combo boxes
    $this.stat.mode configure -state $state
    $this.stat.name configure -state $state
    $this.stat.func configure -state $state

    $twin enable $on
    $top configure -cursor $glyph
  }

  # ------------------------------------------------------------------
  # METHOD:   no_inferior
  #              Put the UI elements of this object into a state
  #              appropriate for an inferior which is not executing.
  #              For this object, this means:
  # Disable:
  # - key binding for all inferior control (not needed -- gdb does this
  #    for us)
  #
  # Enable:
  # - file/func/mode selectors
  # - right-click popups, since gdb DOES allow looking at exe fil
  # - selections
  # 
  # Change mouse pointer to normal
  # ------------------------------------------------------------------
  method no_inferior {} {
    #debug "SrcWin::no_inferior"
    set_execution_status
    enable_ui 1
  }

  # ------------------------------------------------------------------
  #   METHOD: reset - reset the source window
  # ------------------------------------------------------------------  
  method reset {} {
    set current(filename) ""
    set need_files 1
    set do_updates 1
    set last_section ""
    set last_section_start 0
    set last_done 0
    set saved_msg ""

    # do we need to flush the cache or clear the source windows?

    # Empty combo boxes
    $this.stat.name list delete 0 end
    $this.stat.name configure -value {}
    $this.stat.func list delete 0 end
    $this.stat.func configure -value {}
  }

  # ------------------------------------------------------------------
  #  METHOD:  search - search for text or jump to a specific line
  #           in source window, going in the specified DIRECTION.
  # ------------------------------------------------------------------
  method search {direction} {
    set exp [$this.stat.frame.search get]
    #debug "searching $direction for \"$exp\""
    set_status
    set_status [$twin search $exp $direction] 1
  }
  
  #
  #  PROTECTED DATA
  #
  protected top
  protected twin
  
  protected current
  protected need_files

  protected do_updates 1	;# if the window does updates

  # statics used for downloads
  protected last_section ""
  protected last_section_start 0
  protected last_done 0

  protected _mangled_func
  protected Tracing  

  # fenceposts
  protected Running 0
  protected NoRun 0
}

