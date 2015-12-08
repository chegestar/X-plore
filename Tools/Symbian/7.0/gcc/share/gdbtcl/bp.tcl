#
# BpWin - Implements breakpoint window for gdb
# ----------------------------------------------------------------------
#
#   PUBLIC ATTRIBUTES:
#
#
#   METHODS:
#
#     reconfig ....... called when preferences change
#     update ......... breakpoint change hook
#
#
#   X11 OPTION DATABASE ATTRIBUTES
#
#
# ----------------------------------------------------------------------
#   AUTHOR:  Martin M. Hunt <hunt@cygnus.com>
#   Copyright (C) 1997, 1998 Cygnus Solutions
#

itcl_class BpWin {
  # ------------------------------------------------------------------
  #  CONSTRUCTOR - create new breakpoint window
  # ------------------------------------------------------------------
  constructor {config} {
    #
    #  Create a window with the same name as this object
    #
    set class [$this info class]
    @@rename $this $this-tmp-
    @@frame $this -class $class 
    @@rename $this $this-win-
    @@rename $this-tmp- $this

    add_hook gdb_breakpoint_change_hook "$this update"
    if {[pref getd gdb/bp/menu] != ""} {
      set mbar 0
    }
    build_win
  }


  # ------------------------------------------------------------------
  #  METHOD:  build_win - build the main breakpoint window
  # ------------------------------------------------------------------
  method build_win {} {
    global _bp_en _bp_disp tixOption
    global tcl_platform
    set bg1 $tixOption(input1_bg)

    frame $this.f -bg $bg1
    if {$tcl_platform(platform) == "windows"} {
      tixScrolledWindow $this.f.sw -scrollbar both -sizebox 1
    } else {
      tixScrolledWindow $this.f.sw -scrollbar auto
    }
    set twin [$this.f.sw subwidget window]
    $twin configure -bg $bg1

    # write header
    if {$tracepoints} {
    label $twin.num0 -text "Num" -relief raised -bd 2 -anchor center \
      -font src-font
    } 
    label $twin.addr0 -text "Address" -relief raised -bd 2 -anchor center \
      -font src-font
    label $twin.file0 -text "File" -relief raised -bd 2 -anchor center \
      -font src-font
    label $twin.line0 -text "Line" -relief raised -bd 2 -anchor center \
      -font src-font
    label $twin.func0 -text "Function" -relief raised -bd 2 -anchor center \
      -font src-font

    if {$tracepoints} {
      label $twin.pass0 -text "PassCount" -relief raised -borderwidth 2 \
         -anchor center -font src-font
      grid x $twin.num0 $twin.addr0 $twin.file0 $twin.line0 $twin.func0 $twin.pass0 \
         -sticky new
    } else {
      grid x $twin.addr0 $twin.file0 $twin.line0 $twin.func0 -sticky new
    }

    # Let the File and Function columns expand; no others.
    grid columnconfigure $twin 2 -weight 1
    grid columnconfigure $twin 4 -weight 1

    # The last row must always suck up all the leftover vertical
    # space.
    set next_row 1
    grid rowconfigure $twin $next_row -weight 1

    if { $mbar } {
      menu $this.m -tearoff 0
      [winfo toplevel $this] configure -menu $this.m
      if { $tracepoints == 0 } {
         $this.m add cascade -menu $this.m.bp -label "Breakpoint" -underline 0
      } else {
         $this.m add cascade -menu $this.m.bp -label "Tracepoint" -underline 0
      }
      set m [menu $this.m.bp]
      if { $tracepoints == 0 } {
         $m add radio -label "Normal" -variable _bp_disp($selected) \
            -value donttouch -underline 0 -state disabled
         $m add radio -label "Temporary" -variable _bp_disp($selected) \
            -value delete -underline 0 -state disabled
      } else {
         $m add command -label "Actions" -underline 0 -state disabled
      }

      $m add separator
      $m add radio -label "Enabled" -variable _bp_en($selected) -value 1 \
	-underline 0 -state disabled
      $m add radio -label "Disabled" -variable _bp_en($selected) -value 0 \
	-underline 0 -state disabled
      $m add separator
      $m add command -label "Remove" -underline 0 -state disabled
      $this.m add cascade -menu $this.m.all -label "Global" -underline 0
      set m [menu $this.m.all]
      $m add command -label "Disable All" -underline 0 -command "$this bp_all disable"
      $m add command -label "Enable All" -underline 0 -command "$this bp_all enable"
      $m add separator
      $m add command -label "Remove All" -underline 0 -command "$this bp_all delete"
    }

    set Menu [menu $this.pop -tearoff 0]
 
   if { $tracepoints == 0 } {
        $Menu add radio -label "Normal" -variable _bp_disp($selected) \
              -value donttouch -underline 0
        $Menu add radio -label "Temporary" -variable _bp_disp($selected) \
              -value delete -underline 0
    } else {
        $Menu add command -label "Actions" -underline 0 
    }
    $Menu add separator
    $Menu add radio -label "Enabled" -variable _bp_en($selected) -value 1 -underline 0
    $Menu add radio -label "Disabled" -variable _bp_en($selected) -value 0 -underline 0
    $Menu add separator
    $Menu add command -label "Remove" -underline 0
    $Menu add cascade -menu $Menu.all -label "Global" -underline 0
    set m [menu $Menu.all]
    $m add command -label "Disable All" -underline 0 -command "$this bp_all disable"
    $m add command -label "Enable All" -underline 0 -command "$this bp_all enable"
    $m add separator
    $m add command -label "Remove All" -underline 0 -command "$this bp_all delete"

    if { $tracepoints == 0 } {
      # insert all breakpoints
      foreach i [gdb_get_breakpoint_list] {
        bp_add $i
      }
    } else {
      # insert all tracepoints
      foreach i [gdb_get_tracepoint_list] {
        bp_add $i 1
      }
    }

    pack $this.f.sw -side left -expand true -fill both
    pack $this.f -side top -expand true -fill both
  }


  # ------------------------------------------------------------------
  #  METHOD:  bp_add - add a breakpoint entry
  # ------------------------------------------------------------------
  method bp_add { bpnum {tracepoint 0}} {
    global _bp_en _bp_disp tcl_platform _files
    
    if {$tracepoint} {
      set bpinfo [gdb_get_tracepoint_info $bpnum]
      lassign $bpinfo file func line pc enabled pass_count \
	step_count thread hit_count actions
      set disposition tracepoint
      set bptype tracepoint
    } else {
      set bpinfo [gdb_get_breakpoint_info $bpnum]
      lassign $bpinfo file func line pc type enabled disposition \
	ignore_count commands cond thread hit_count
      set bptype breakpoint
    }

    #debug "bp_add bpnum=$bpnum row=$next_row"
    set i $next_row
    set _bp_en($i) $enabled
    set _bp_disp($i) $disposition
    set temp($i) ""
    switch $disposition {
      donttouch { set color [pref get gdb/src/bp_fg] }
      delete { 
	set color [pref get gdb/src/temp_bp_fg]
	set temp($i) delete
      }
      tracepoint {
	set color [pref get gdb/src/trace_fg]
      }
      default { set color yellow }
    }
    
    if {$tcl_platform(platform) == "windows"} {
      checkbutton $twin.en$i -relief flat -variable _bp_en($i) -bg $bg1 \
	-activebackground $bg1 -command "$this bp_able $i" -fg $color 
    } else {
      checkbutton $twin.en$i -relief flat -variable _bp_en($i) -selectcolor $color \
	-command "$this bp_able $i" -bg $bg1 -activebackground $bg1
    }

    if {$tracepoints} {
      label $twin.num$i -text "$bpnum " -relief flat -anchor e -font src-font -bg $bg1
    }
    label $twin.addr$i -text "$pc " -relief flat -anchor e -font src-font -bg $bg1
    if {[info exists _files(short,$file)]} {
      set file $_files(short,$file)
    } else {
      # FIXME.  Really need to do better than this.
      set file [@@file tail $file]
    }
    label $twin.file$i -text "$file " -relief flat -anchor e -font src-font -bg $bg1
    label $twin.line$i -text "$line " -relief flat -anchor e -font src-font -bg $bg1
    label $twin.func$i -text "$func " -relief flat -anchor e -font src-font -bg $bg1
    
    if {$tracepoints} {
      label $twin.pass$i -text "$pass_count " -relief flat -anchor e -font src-font -bg $bg1
    }

    if {$mbar} {
      set zz [list addr file func line]
      if {$tracepoints} {
	lappend zz num pass
      }
      foreach thing $zz {
	bind $twin.${thing}${i} <1> "$this bp_select $i"
	bind $twin.${thing}${i} <Double-1> "$this goto_bp $i"
      }
    }

    if {$tracepoints} {
    grid $twin.en$i $twin.num$i $twin.addr$i $twin.file$i $twin.line$i \
      $twin.func$i $twin.pass$i -sticky new -ipadx 4 -ipady 2
    } else {
    grid $twin.en$i $twin.addr$i $twin.file$i $twin.line$i \
      $twin.func$i -sticky new -ipadx 4 -ipady 2
    }

    # This used to be the last row.  Fix it vertically again.
    grid rowconfigure $twin $i -weight 0

    set index_to_bpnum($i) $bpnum
    set Index_to_bptype($i) $bptype
    incr i
    set next_row $i
    grid rowconfigure $twin $i -weight 1
  }

  # ------------------------------------------------------------------
  #  METHOD:  bp_select - select a row in the grid
  # ------------------------------------------------------------------
  method bp_select { r } {
    global tixOption _bp_en _bp_disp

    set zz [list addr file func line]
    if {$tracepoints} {
      lappend zz num pass
    }

    if {$selected} {
      set i $selected
      
      foreach thing $zz {
	$twin.${thing}${i}  configure -fg $tixOption(fg) -bg $bg1
	bind $twin.${thing}${i} <3> break
      }
    }

    # if we click on the same line, unselect it and return
    if {$selected == $r} {
      set selected 0

      if {$tracepoints == 0} {
      $this.m.bp entryconfigure "Normal" -state disabled
      $this.m.bp entryconfigure "Temporary" -state disabled
      } else {
      $this.m.bp entryconfigure "Actions" -state disabled
      }
      $this.m.bp entryconfigure "Enabled" -state disabled
      $this.m.bp entryconfigure "Disabled" -state disabled
      $this.m.bp entryconfigure "Remove" -state disabled
      
      foreach thing $zz {
	bind $twin.${thing}${r} <3> break
      }

      return
    }

    foreach thing $zz {
      $twin.${thing}${r}  configure -fg $tixOption(select_fg) -bg $tixOption(select_bg)
      bind $twin.${thing}${r}  <3> "tk_popup $Menu %X %Y"
    }
 
    if {$tracepoints == 0} {
      $this.m.bp entryconfigure "Normal" -variable _bp_disp($r) \
	-command "$this bp_type $r" -state normal
      $this.m.bp entryconfigure "Temporary" -variable _bp_disp($r) \
	-command "$this bp_type $r" -state normal
      $Menu entryconfigure "Normal" -variable _bp_disp($r)      \
	-command "$this bp_type $r" -state normal
      $Menu entryconfigure "Temporary" -variable _bp_disp($r)      \
	-command "$this bp_type $r" -state normal
    } else {
    $this.m.bp entryconfigure "Actions" -command "$this get_actions $r" -state normal
    $Menu entryconfigure "Actions" -command "$this get_actions $r" -state normal
    }
    $this.m.bp entryconfigure "Enabled" -variable _bp_en($r)   \
      -command "$this bp_able $r" -state normal
    $this.m.bp entryconfigure "Disabled" -variable _bp_en($r)   \
      -command "$this bp_able $r" -state normal
    $this.m.bp entryconfigure "Remove" -command "$this bp_remove $r" -state normal
    $Menu entryconfigure "Enabled" -variable _bp_en($r)        \
      -command "$this bp_able $r" -state normal
    $Menu entryconfigure "Disabled" -variable _bp_en($r)        \
      -command "$this bp_able $r" -state normal
    $Menu entryconfigure "Remove" -command "$this bp_remove $r" -state normal
 
    set selected $r
  }

  # ------------------------------------------------------------------
  #  METHOD:  bp_modify - modify a breakpoint entry
  # ------------------------------------------------------------------
  method bp_modify { bpnum {tracepoint 0} } {
    global _bp_en _bp_disp tcl_platform _files

    if {$tracepoint} {
      set bpinfo [gdb_get_tracepoint_info $bpnum]
      lassign $bpinfo file func line pc enabled pass_count \
	step_count thread hit_count actions
      set disposition tracepoint
      set bptype tracepoint
    } else {
      set bpinfo [gdb_get_breakpoint_info $bpnum]
      lassign $bpinfo file func line pc type enabled disposition \
	ignore_count commands cond thread hit_count
      set bptype breakpoint
    }

    set found 0
    for {set i 1} {$i < $next_row} {incr i} {
      if { $bpnum == $index_to_bpnum($i)
	 && "$Index_to_bptype($i)" == "$bptype"} {
	incr found
	break
      }
    }

    if {!$found} {
      debug "ERROR: breakpoint number $bpnum not found!"
      return
    }

    if {$_bp_en($i) != $enabled} {
      set _bp_en($i) $enabled
    }

    if {$_bp_disp($i) != $disposition} {
      set _bp_disp($i) $disposition
    }

    switch $disposition {
      donttouch { set color [pref get gdb/src/bp_fg] }
      delete { 
	set color [pref get gdb/src/temp_bp_fg]
      }
      tracepoint { set color [pref get gdb/src/trace_fg] }
      default { set color yellow}
    }
    if {$tcl_platform(platform) == "windows"} then {
      $twin.en$i configure -fg $color 
    } else {
      $twin.en$i configure -selectcolor $color
    }
    if {$tracepoints} {
    $twin.num$i configure  -text "$bpnum "
    }
    $twin.addr$i configure -text "$pc " 
    if {[info exists _files(short,$file)]} {
      set file $_files(short,$file)
    } else {
      # FIXME.  Really need to do better than this.
      set file [@@file tail $file]
    }
    $twin.file$i configure -text "$file "
    $twin.line$i configure  -text "$line "
    $twin.func$i configure  -text "$func "
    if {$tracepoints} {
    $twin.pass$i configure  -text "$pass_count "
    }
  }

  # ------------------------------------------------------------------
  #  METHOD:  bp_able - enable/disable a breakpoint
  # ------------------------------------------------------------------
  method bp_able { i } {
    global _bp_en
       
    bp_select $i

    switch $Index_to_bptype($i) {
      breakpoint {set type {}}
      tracepoint {set type "tracepoint"}
    }

    if {$_bp_en($i) == "1"} {
      set command "enable $type $temp($i) "
    } else {
      set command "disable $type "
    }

    append command  "$index_to_bpnum($i)"
    gdb_cmd "$command"
  }

  # ------------------------------------------------------------------
  #  METHOD:  bp_remove - remove a breakpoint
  # ------------------------------------------------------------------
  method bp_remove { i } {

    bp_select $i

    switch $Index_to_bptype($i) {
      breakpoint { set type {} }
      tracepoint { set type "tracepoint" }
    }

    gdb_cmd "delete $type $index_to_bpnum($i)"
  }

  # ------------------------------------------------------------------
  #  METHOD:  bp_type - change the breakpoint type (disposition)
  # ------------------------------------------------------------------
  method bp_type { i } {
    
    if {"$Index_to_bptype($i)" != "breakpoint"} {
      return
    }

    set bpnum $index_to_bpnum($i)
    #debug "bp_type $i $bpnum"
    set bpinfo [gdb_get_breakpoint_info $bpnum]
    lassign $bpinfo file func line pc type enabled disposition \
      ignore_count commands cond thread hit_count
    bp_select $i
    switch $disposition {
      delete {  
	gdb_cmd "delete $bpnum"
	gdb_cmd "break *$pc"
      }
      donttouch {
	gdb_cmd "delete $bpnum"
	gdb_cmd "tbreak *$pc"
      }
      default { debug "Unknown breakpoint disposition: $disposition" }
    }
  }

  # ------------------------------------------------------------------
  #  METHOD:  bp_delete - delete a breakpoint
  # ------------------------------------------------------------------
  method bp_delete { bpnum } {
    for {set i 1} {$i < $next_row} {incr i} {
      if { $bpnum == $index_to_bpnum($i) } {
       if {$tracepoints} {
	grid forget $twin.en$i $twin.num$i $twin.addr$i $twin.file$i $twin.line$i $twin.func$i $twin.pass$i
	destroy $twin.en$i $twin.num$i $twin.addr$i $twin.file$i $twin.line$i $twin.func$i $twin.pass$i
       } else {
        grid forget $twin.en$i $twin.addr$i $twin.file$i $twin.line$i $twin.func$i
        destroy $twin.en$i $twin.addr$i $twin.file$i $twin.line$i $twin.func$i
       }
	if {$selected == $i} {
	  set selected 0
	}
	return
      } 
    }
  }

  # ------------------------------------------------------------------
  #  METHOD:  update - update widget when a breakpoint changes
  # ------------------------------------------------------------------
  method update {action bpnum addr {linenum {}} {file {}} {type 0}} {
    #debug "bp update $action $bpnum $type"

    if {$type == "tracepoint"} {
      set tp 1
    } else {
      set tp 0
    }

    switch $action {
      modify { bp_modify $bpnum $tp}
      create { bp_add $bpnum $tp}
      delete { bp_delete $bpnum }
      default { debug "Unknown breakpoint action: $action" }
    }
  }

  # ------------------------------------------------------------------
  #  METHOD:  bp_all - perform a command on all breakpoints
  # ------------------------------------------------------------------
  method bp_all { command } {

    # Do all breakpoints
    foreach bpnum [gdb_get_breakpoint_list] {
      if { $command == "enable"} {
	for {set i 1} {$i < $next_row} {incr i} {
	  if { $bpnum == $index_to_bpnum($i)
	       && "$Index_to_bptype($i)" == "breakpoint"} {
	    gdb_cmd "enable $temp($i) $bpnum"
	    break
	  }
	}
      } else {
	gdb_cmd "$command $bpnum"
      }
    }

    # Do all tracepoints
    foreach bpnum [gdb_get_tracepoint_list] {
      if { $command == "enable"} {
	for {set i 1} {$i < $next_row} {incr i} {
	  if { $bpnum == $index_to_bpnum($i)
	       && "$Index_to_bptype($i)" == "tracepoint"} {
	    gdb_cmd "enable tracepoint $bpnum"
	    break
	  }
	}
      } else {
	gdb_cmd "$command tracepoint $bpnum"
      }
    }


  }

  # ------------------------------------------------------------------
  #  DESTRUCTOR - destroy window containing widget
  # ------------------------------------------------------------------
  destructor {
    remove_hook gdb_breakpoint_change_hook "$this update"
    set top [winfo toplevel $this]
    destroy $this
    destroy $top
  }

  # ------------------------------------------------------------------
  #  METHOD:  get_actions - pops up the add trace dialog on a selected 
  #                         tracepoint
  # ------------------------------------------------------------------
  method get_actions {bpnum} {
 
   set bpnum $index_to_bpnum($bpnum)
   set bpinfo [gdb_get_tracepoint_info $bpnum]
   lassign $bpinfo file func line pc enabled pass_count \
     step_count thread hit_count actions

   set filename [@@file tail $file]
   manage create tracedlg -File $filename -Lines $line
  }


  # ------------------------------------------------------------------
  #  METHOD:  config - used to change public attributes
  # ------------------------------------------------------------------
  method config {config} {}
    
  # ------------------------------------------------------------------
  #  METHOD:  reconfig - used when preferences change
  # ------------------------------------------------------------------
  method reconfig {} {
    destroy $this.sw
    build_win
  }
  
  # ------------------------------------------------------------------
  #  METHOD:  goto_bp - show bp in source window
  # ------------------------------------------------------------------
  method goto_bp {r} {    
    set bpnum $index_to_bpnum($r)
    if {$tracepoints} {
      set bpinfo [gdb_get_tracepoint_info $bpnum]
    } else {
      set bpinfo [gdb_get_breakpoint_info $bpnum]
    }
    set pc [lindex $bpinfo 3]

    # !! FIXME: multiple source windows?
    set src [lindex [manage find src] 0]
    set info [gdb_loc *$pc]
    $src location BROWSE_TAG $info
  }

  #
  #  PUBLIC DATA
  #
  public tracepoints 0

  #
  #  PROTECTED DATA
  #
  protected twin
  protected next_row 0
  protected index_to_bpnum
  protected Index_to_bptype
  protected temp
  protected mbar 1
  protected selected 0
  protected bg1
  protected Menu
}
