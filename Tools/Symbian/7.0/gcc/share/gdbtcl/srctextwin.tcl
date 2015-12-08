#
# SrcTextWin
# ----------------------------------------------------------------------
# Implements the paned text widget with the source code in in.
# This widget is typically embedded in a SrcWin widget.
#
# ----------------------------------------------------------------------
#   Copyright (C) 1997, 1998 Cygnus Solutions

itcl_class SrcTextWin {

  constructor {config} {
    set class [$this info class]
    @@rename $this $this-tmp-
    @@frame $this -class $class
    @@rename $this $this-win-
    @@rename $this-tmp- $this
    
    set top [winfo toplevel $this]
    if {$parent == {}} {
      set parent [winfo parent $this]
    }

    if {![info exists sImage]} {
      set size [font measure [pref get gdb/src/font] "W"]
      set sImage   [makeBreakDot $size [pref get gdb/src/bp_fg]]
      set tImage   [makeBreakDot $size [pref get gdb/src/temp_bp_fg]]
      set dImage   [makeBreakDot $size [pref get gdb/src/disabled_fg]]
      set trImage  [makeBreakDot $size [pref get gdb/src/trace_fg]]
    }
    
    if {$ignore_var_balloons} {
      set UseVariableBalloons 0
    } else {
      set UseVariableBalloons [pref get gdb/src/variableBalloons]
    }

    set Linenums [pref get gdb/src/linenums]

    #Initialize state variables
    set pc(filename) ""
    set pc(funcname) ""
    set pc(line) 0
    set pc(addr) ""
    set pc(asm_line) 0
    set current(filename) ""
    set current(funcname) ""
    set current(line) 0
    set current(addr) ""
    set current(asm_line) 0
    set current(tag) "BROWSE_TAG"
    set current(mode) "SOURCE"

    build_win

    # add hooks
    add_hook gdb_breakpoint_change_hook "$this bp"
    if {$UseVariableBalloons} {
      add_hook gdb_idle_hook "$this updateBalloon"
    }
    global ${this}_balloon
    trace variable ${this}_balloon w "$this trace_help"
  }

  destructor {
    remove_hook gdb_breakpoint_change_hook "$this bp"
    remove_hook gdb_idle_hook "$this updateBalloon"
  }

  # ------------------------------------------------------------------
  #  METHOD:  build_win - build the main source paned window
  # ------------------------------------------------------------------
  method build_win {} {
    tixPanedWindow $this.p -handlebg [pref get gdb/src/handlebg]
    $this.p add pane1 -expand 1 -min 100
    set pane1 [$this.p subwidget pane1]
    set Stwc(gdbtk_scratch_widget) [tixScrolledText $this.stext0 -scrollbar "auto +y"]
    set twinp $Stwc(gdbtk_scratch_widget)
    set twin [$twinp subwidget text]
    pack $twinp -fill both -expand yes -in $pane1
    pack $this.p -fill both -expand yes
    config_win $twin 0
  }
  
  # ------------------------------------------------------------------
  #  METHOD:  SetRunningState - set state based on if GDB is running or not.
  #  This disables the popup menus when GDB is not running yet.
  # ------------------------------------------------------------------
  method SetRunningState {state} {
    debug "SetRunningState $twin.bmenu $state"
    $twin.bmenu entryconfigure 0 -state $state
    $twin.cmenu entryconfigure 2 -state $state
    if {[info exists bwin]} {
      $bwin.bmenu entryconfigure 0 -state $state
      $bwin.cmenu entryconfigure 2 -state $state
    }
  }

  # ------------------------------------------------------------------
  #  METHOD:  enable - enable or disable bindings and change cursor
  # ------------------------------------------------------------------
  method enable {on} {
    if {$on} {
      set Running 0
      set glyph ""
      set bnd ""
      set status normal
    } else {
      set Running 1
      set glyph watch
      set bnd "break"
      set status disabled
    }

    bind $twin <B1-Motion> $bnd
    bind $twin <Double-1> $bnd
    bind $twin <Triple-1> $bnd
    bind_src_tags $twin $status
    if {[info exists bwin]} {
      bind $bwin <B1-Motion> $bnd
      bind $bwin <Double-1> $bnd
      bind $bwin <Triple-1> $bnd
      bind_src_tags $bwin $status
    }

    $twin configure -cursor $glyph
    if {[info exists bwin]} {
      $bwin configure -cursor $glyph
    }
  }

  # ------------------------------------------------------------------
  # METHOD:  makeBreakDot - make the break dot for the screen
  # ------------------------------------------------------------------
  method makeBreakDot {size color {image {}}} {
    if {$size > 32} {
      set size 32
    } elseif {$size < 1} {
      set size 1
    }

    if {$image == ""} {
      set image [image create photo -width $size -height $size]
    } else {
      $image blank
      $image configure -width $size -height $size
    }

    set x1 1
    set x2 [expr {1 + $size}]
    set y1 1
    set y2 $x2
    $image put $color -to 1 1 $x2 $y2
    return $image
  }

  # ------------------------------------------------------------------
  # METHOD: setTabs - set the tabs for the assembly/src windows
  # ------------------------------------------------------------------
  method setTabs {win {asm 0}} {
    set fsize [font measure src-font "W"]
    set tsize [pref get gdb/src/tab_size]
    set rest ""
    
    if {$asm} {
      set first  [expr {$fsize * 12}]
      set second [expr {$fsize * 13}]
      set third  [expr {$fsize * 34}]
      for {set i 1} {$i < 8} {incr i} {
        lappend rest [expr {(34 + ($i * $tsize)) * $fsize}] left
      }
      set tablist [concat [list $first right $second left $third left] $rest]
    } else {
      # SOURCE window
      # The first tab right-justifies the line numbers and the second
      # tab is the left margin for the start on the source code.  The remaining
      # tabs should be regularly spaced depending on prefs.
      if {$Linenums} {
	set first  [expr {$fsize * 6}]	;# "- " plus 4 digit line number
	set second [expr {$fsize * 7}]	;# plus a space after the number 
	for {set i 1} {$i < 8} {incr i} {
	  lappend rest [expr {(7 + ($i * $tsize)) * $fsize}] left
	}
	set tablist [concat [list $first right $second left] $rest]
      } else {
	set first  [expr {$fsize * 2}]
	for {set i 1} {$i < 8} {incr i} {
	  lappend rest [expr {(2 + ($i * $tsize)) * $fsize}] left
	}
	set tablist [concat [list $first left] $rest]
      }
    }
    $win configure -tabs $tablist
  }

  method bind_src_tags {win how} {
    switch $how {
      normal {
	set cur1 dot
	set cur2 xterm
      }
      disabled {
	set cur1 watch
	set cur2 $cur1
      }
    }
    $win tag bind break_tag <Enter> "$win config -cursor $cur1"
    $win tag bind break_tag <Leave> "$win config -cursor $cur2"
    $win tag bind bp_tag <Enter> "$win config -cursor $cur1"
    $win tag bind bp_tag <Leave> "$win config -cursor $cur2"
  }

  # ------------------------------------------------------------------
  #  METHOD:  config_win - configure the source or assembly text window
  # ------------------------------------------------------------------
  method config_win {win asm} {
    global tixOption
    
    debug "config_win $win $asm Tracing=$Tracing"
    # Force focus down from the toplevel.
#    bind $top <FocusIn> [format {
#      set tmp_top %s
#      set tmp_twin %s
#      if {[focus] == $tmp_top} {
#	focus $tmp_twin
##      }
#    } $top $twin]
    
    # need this, but not sure why
#    bind $top <FocusOut> [format {
#      set tmp_top %s
#      set tmp_twin %s
#      if {[focus] == $tmp_top} {
#	focus $tmp_twin
#      }
#    } $top $twin]

    $win config -borderwidth 5 -relief groove -insertwidth 0 -wrap none
  
    # font
    set font [pref get gdb/src/font]
    $win configure -font $font

    setTabs $win $asm

    # set up some tags.  should probably be done differently
    # !! change bg?
    $win tag configure break_tag -font $font -foreground [pref get gdb/src/break_fg]
    $win tag configure bp_tag -font $font -foreground [pref get gdb/src/break_fg]
    $win tag configure source_tag -font $font
    $win tag configure source_tag2 -font $font -foreground [pref get gdb/src/source2_fg]
    $win tag configure PC_TAG -background [pref get gdb/src/PC_TAG]
    $win tag configure STACK_TAG -background [pref get gdb/src/STACK_TAG]
    $win tag configure BROWSE_TAG -background [pref get gdb/src/BROWSE_TAG]
    
    # search tag used to highlight searches
    foreach option [$win tag configure sel] {
      set op [lindex $option 0]
      set val [lindex $option 4]
      eval $win tag configure search $op $val
    }

    # popup menu
    if {![winfo exists $win.menu]} {
      menu $win.menu -tearoff 0
    }

    # mouse button 1 prints a variable's value
    $win tag bind source_tag <Double-1> "$this do_button1"
    $win tag bind source_tag2 <Double-1> "$this do_button1"

    # bind mouse button 3 to the popup men
    $win tag bind source_tag <Button-3> "$this do_popup $win.menu %X %Y %x %y"
    $win tag bind source_tag2 <Button-3> "$this do_popup $win.menu %X %Y %x %y"

    if {![winfo exists $win.bmenu]} {
      # breakpoint popup menu
      # FIXME: colors should be from prefs
      # don't enable hardware or conditional breakpoints until they are tested
      menu $win.bmenu -tearoff 0
      
      if {!$Tracing} {
	$win.bmenu add command -label "Continue to Here" \
	  -command "$this bp_line $win -1 TC" -activebackground green
	$win.bmenu add command -label "Set Breakpoint" \
	  -command "$this bp_line $win -1" -activebackground [pref get gdb/src/bp_fg]
	$win.bmenu add command -label "Set Temporary Breakpoint" \
	  -command "$this bp_line $win -1 T" -activebackground [pref get gdb/src/temp_bp_fg]
      } else {
	$win.bmenu add command -label "Set Tracepoint" \
	  -command "$this set_tracepoint $win -1" -activebackground [pref get gdb/src/trace_fg]
      }
      #    $twin.bmenu add command -label "Toggle conditional bp here" \
	-command {debug "conditional"} \
	-background gold
      #    $twin.bmenu add command -label "Toggle hardware bp here" \
	-command "$this bp_line -1 H" \
	-background navy -foreground white
    }

    if {![winfo exists $win.cmenu]} {
      # this popup is used when the line contains a set breakpoint
      menu $win.cmenu -tearoff 0
      $win.cmenu add command -label "Delete Breakpoint" \
	-command "$this bp_line $win -1" 
      $win.cmenu add separator    
      $win.cmenu add command -label "Continue to Here" \
	-command "$this bp_line $win -1 TC" -activebackground green
    }

    # Disable printing and cut and paste keys; makes the window readonly
    # We do this so we don't have to enable and disable the
    # text widget everytime we want to modify it.
    bind $win <Key> {if {"%A" != "{}"} {break}}
    bind $win <Delete> break
    bind $win <ButtonRelease-2> {break}

    # GDB key bindings
    # We need to explicitly ignore keys with the Alt modifier, since
    # otherwise they will interfere with selecting menus on Windows.

    if {!$Tracing} {
      bind_plain_key $win c "$this do_key continue; break" 
      bind_plain_key $win r "$this do_key run; break"
      bind_plain_key $win f "$this do_key finish; break"
    } else {
      bind_plain_key $win n "$this do_key tfind_next; break"
      bind_plain_key $win p "$this do_key tfind_prev; break"
      bind_plain_key $win f "$this do_key tfind_start; break"
      bind_plain_key $win l "$this do_key tfind_line; break"
      bind_plain_key $win h "$this do_key tfind_tp; break"
    }
    bind_plain_key $win u "$this do_key up; break"
    bind_plain_key $win d "$this do_key down; break"
    bind_plain_key $win x "$this do_key quit; break"

    if {!$Tracing} {
      if {$asm == 1} {
        bind_plain_key $win s "$this do_key stepi; break"
        bind_plain_key $win n "$this do_key next; break"
      } else {
        bind_plain_key $win s "$this do_key step; break"
        bind_plain_key $win n "$this do_key next; break"
      }
    }

    bind_plain_key $win Control-h "$this do_key thread_list; break"
    bind_plain_key $win Control-f "$this do_key browser; break"
    bind_plain_key $win Control-d "$this do_key download; break"
    bind_plain_key $win Control-p "$this do_key print"
    bind_plain_key $win Control-o [list $this do_key open]

    if {!$Tracing} {
      # Ctrl+F5 is another accelerator for Run
      bind_plain_key $win Control-F5 "$this do_key run"
    }

    bind_plain_key $win Control-F11 "$this do_key toggle_debug"
    bind_plain_key $win Alt-v "$win yview scroll -1 pages"
    bind_plain_key $win Control-v [format {
      %s yview scroll 1 pages
      break
    } $win]

    # bind mouse button 1 to the breakpoint method
    $win tag bind break_tag <Button-1> "$this bp_line $win %y"
    $win tag bind bp_tag <Button-1> "$this bp_line $win %y"
    
    # avoid special handling of double and triple clicks in break area
    bind $win <Double-1> [format {
      if {[lsearch [%s tag names @%%x,%%y] break_tag] >= 0} {
	break
      }
    } $win $win]
    bind $win <Triple-1> [format {
      if {[lsearch [%s tag names @%%x,%%y] break_tag] >= 0} {
	break
      }
    } $win $win]

    # bind mouse button 3 to the popup menu
    $win tag bind break_tag <Button-3> "$this do_popup $win.bmenu %X %Y -1 %y"
    $win tag bind bp_tag <Button-3> "$this do_popup $win.cmenu %X %Y -1 %y"

    # bind window shortcuts
    bind_plain_key $win Control-s "$this do_key stack"
    bind_plain_key $win Control-r "$this do_key registers"
    bind_plain_key $win Control-m "$this do_key memory"
    bind_plain_key $win Control-w "$this do_key watch"
    bind_plain_key $win Control-l "$this do_key locals"
    if { !$Tracing } {
      bind_plain_key $win Control-b "$this do_key breakpoints"
    } else {
      bind_plain_key $win Control-t "$this do_key tracepoints"
      bind_plain_key $win Control-u "$this do_key tdump"
    }
    bind_plain_key $win Control-n "$this do_key console"
    bind_src_tags $win normal

    if {$UseVariableBalloons && $current(mode) == "SOURCE"} {
      bind $win <Motion> "$this motion %x %y"
      bind $win <Leave> "$this cancelMotion"
    }

    # Up/Down arrow key bindings
    bind_plain_key $win Up [list %W yview scroll -1 units]
    bind_plain_key $win Down [list %W yview scroll +1 units]
  }

  # ------------------------------------------------------------------
  #  METHOD:  reconfig - used when preferences change
  # ------------------------------------------------------------------
  method reconfig {} {
    debug "SrcTextWin reconfig"

    # Make sure we redo the break images when we reconfigure
    set size [font measure src-font "W"]
    makeBreakDot $size [pref get gdb/src/bp_fg] $sImage
    makeBreakDot $size [pref get gdb/src/temp_bp_fg] $tImage
    makeBreakDot $size [pref get gdb/src/disabled_fg] $dImage
    makeBreakDot $size [pref get gdb/src/trace_fg] $trImage

    # Tags
    $twin tag configure PC_TAG -background [pref get gdb/src/PC_TAG]
    $twin tag configure STACK_TAG -background [pref get gdb/src/STACK_TAG]
    $twin tag configure BROWSE_TAG -background [pref get gdb/src/BROWSE_TAG]
    setTabs $twin

    # Variable Balloons
    if {$ignore_var_balloons} {
      set balloons 0
    } else {
      set balloons [pref get gdb/src/variableBalloons]
    }
    if {$UseVariableBalloons != $balloons} {
      set UseVariableBalloons $balloons
      if {$UseVariableBalloons} {
	if {$current(mode) == "SOURCE"} {
	  # Only do for SOURCE displays now
	  bind $twin <Motion> "$this motion %x %y"
	  bind $twin <Leave> "$this cancelMotion"
	}
	add_hook gdb_idle_hook [list $this updateBalloon]
      } else {
	cancelMotion
	if {$current(mode) == "SOURCE"} {
	  bind $twin <Motion> {}
	  bind $twin <Leave> {}
	  $twin tag remove _show_variable 1.0 end 
	}
	remove_hook gdb_idle_hook [list $this updateBalloon]
      }
    }

    # Popup colors
    if {$Tracing} {
      $twin.bmenu entryconfigure 0 -activebackground [pref get gdb/src/trace_fg]
    } else {
      $twin.bmenu entryconfigure 1 -activebackground [pref get gdb/src/bp_fg]
      $twin.bmenu entryconfigure 2 -activebackground [pref get gdb/src/temp_bp_fg]
    }
  }

  # ------------------------------------------------------------------
  # METHOD: updateBalloon - we have gone idle, update the balloon
  # ------------------------------------------------------------------
  method updateBalloon {} {
    global ${this}_balloon
    if {[info exists ${this}_balloon]} {
      set a [set ${this}_balloon]
      if {[string length $a]} {
	set a   [split $a =]
	set var [lindex $a 0]
	catch {tmp delete}
	Variable tmp tmp $var
	set val [tmp value]
	register_balloon "$var=$val"
      }
    }
  }

  # ------------------------------------------------------------------
  # METHOD: ClearTags - clear all tags
  # ------------------------------------------------------------------
  method ClearTags {} {
    foreach tag {PC_TAG BROWSE_TAG STACK_TAG} {
      $twin tag remove $tag $current(line).2 $current(line).end
      $twin tag remove $tag $pc(line).2 $pc(line).end
      $twin tag remove $tag $current(asm_line).2 $current(asm_line).end
      if {[info exists bwin]} {
	  $bwin tag remove $tag $current(asm_line).2 $current(asm_line).end
      }
    }
  }

  # ------------------------------------------------------------------
  # METHOD: FillSource - fill a window with source
  # ------------------------------------------------------------------
  method FillSource {winname tagname filename funcname line addr pc_addr} {
    global gdb_running
    upvar $winname win
    debug "FillSource $gdb_running $tagname line=$line pc(line)=$pc(line)"
    debug "FillSource current(filename)=$current(filename) filename=$filename"
    if {$filename != ""} {
      # load new file if necessary
      if {$filename != $current(filename) || $mode_changed} {
	if {![LoadFile $winname $filename]} {
	  # failed to find source file
	  set current(line) $line
	  set current(tag) $tagname
	  set current(addr) $addr
	  set current(funcname) $funcname
	  set current(filename) $filename
	  set oldmode SOURCE
	  $parent.stat.mode configure -value ASSEMBLY
	  return
	}
	if {$current(mode) != "SRC+ASM"} {
	  # reset this flag in FillAssembly for SRC+ASM mode
	  set mode_changed 0
	}
      }
      debug "cf=$current(filename) pc=$pc(filename) filename=$filename"
      if {$current(filename) != ""} {
	if {$gdb_running && $pc(filename) == $filename} {
	  # set the PC tag in this file
	  debug "tagging $win line $pc(line) with PC_TAG"
	  $win tag add PC_TAG $pc(line).2 $pc(line).end
	}
	if {$tagname != "PC_TAG"} {
	  if {$gdb_running && $pc(filename) == $filename && $pc(line) == $line} {
	    # if the tag is on the same line as the PC, set a PC tag
	    $win tag add PC_TAG $line.2 $line.end
	  } else {
	    $win tag add $tagname $line.2 $line.end
	  }
	}
	if {$pc(filename) == $filename && $line == 0} {
	  # no line specified, so show line with PC
	  display_line $win $pc(line)
	} else {
	  display_line $win $line
	}
      }
      return
    }
    # no source; switch to assembly
    debug "no source file; switch to assembly"
    set current(line) $line
    set current(tag) $tagname
    set current(addr) $addr
    set current(funcname) $funcname
    set current(filename) $filename
    set oldmode $current(mode)
    $parent.stat.mode configure -value ASSEMBLY
  }

  # ------------------------------------------------------------------
  # METHOD: FillAssembly - fill a window with disassembled code
  # ------------------------------------------------------------------
  method FillAssembly {winname tagname filename funcname line addr pc_addr} {
    global gdb_running
    #set NoRun 1
    #gdbtk_busy	
    upvar $winname win
    set winnamep ${winname}p
    upvar $winnamep winp
    debug "FillAssembly $win $tagname $filename $funcname $line $addr $pc_addr"
    if {$winname == "twin"} {
      set pane "pane1"
    } else {
      set pane "pane2"
    }

    if {$funcname == ""} {
      pack forget $winp
      set winp $Stwc(gdbtk_scratch_widget)
      set win [$winp subwidget text]
      $win delete 0.0 end
      $win insert 0.0 "Select function name to disassemble"
      pack $winp -expand yes -fill both -in [$this.p subwidget $pane]
    } elseif {$funcname != $current(funcname) || $mode_changed} {
      set mode_changed 0
      set oldwinp $winp
      if {[LoadFromCache $winname $winnamep $addr 1]} {
	debug [format "Disassembling at %x" $addr]
	debug "T cf=$current(filename) name=$filename"
	if {[catch {set dis [gdb_disassemble nosource $addr]} mess] } {
	  # print some intelligent error message?
	  debug "Disassemble failed: $mess"
	  UnLoadFromCache $winname $winnamep $oldwinp $addr
	} else {
	  # fill in new text widget
	  set i 1
	  if {[info exists _map]} { unset _map }
	  foreach line [split $dis \n] {
	    if {[string index $line 0] != " "} {
	      set address [lindex [split $line] 4]
	      if {$i == 1} {set asm_lo_addr $address}
	      # Source line
	      regexp {([0-9]*)[ ]*(.*)} $line match srcline srccode
	      if {$srcline != ""} {
		$win insert $i.end "\t$srcline"
		$win insert $i.end "\t$srccode\n" source_tag
		set _map(srcline=$srcline) $i
	      }
	    } else {
	      # assembly line
	      set address [lindex [split $line] 4]
	      if {$i == 1} {set asm_lo_addr $address}
	      set line [split [string trim $line]]
	      set lineNum [lindex $line 0]
	      set offset  [lindex $line 1]
	      set code [join [lrange $line 2 end]]
	      #debug "lineNum = $lineNum\noffset = $offset\n code=$code\n"
	      $win insert $i.0 "-\t$lineNum\t$offset\t$code\n"
	      $win tag add break_tag $i.0 $i.11
	      $win tag add source_tag $i.12 $i.end
	      set asm_hi_addr $address
	      set _map(pc=$address) $i
	      set _map(line=$i) $address
	    }
	    incr i
	  }
	}
      }
      set current(filename) $filename
      set do_display_breaks 1
    }

    # highlight proper line number
    if {[info exists _map(pc=$addr)]} {
      # if current file has PC, highlight that too
      if {$gdb_running && $tagname != "PC_TAG" && $pc(filename) == $filename
	  && $pc(func) == $funcname} {
	set pc(asm_line) $_map(pc=$pc_addr)
	$win tag add PC_TAG $pc(asm_line).2 $pc(asm_line).end
      }
      set current(asm_line) $_map(pc=$addr)
      debug "current(asm_line) = $current(asm_line)"
      # don't set browse tag if it is at PC
      if {$pc_addr != $addr || $tagname == "PC_TAG"} {
	# HACK.  In STACK mode we usually want the previous instruction
	if {$tagname == "STACK_TAG"} {
	  incr current(asm_line) -1
	}
	$win tag add $tagname $current(asm_line).2 $current(asm_line).end
      }
    }
    display_line $win $current(asm_line)
    #set NoRun 0
    #gdbtk_idle
  }


  # ------------------------------------------------------------------
  # METHOD: FillMixed - fill a window with mixed source and assembly
  # ------------------------------------------------------------------
  method FillMixed {winname tagname filename funcname line addr pc_addr} {
    global gdb_running
    upvar $winname win
    set winnamep ${winname}p
    upvar $winnamep winp

    set asm_lo_addr ""
    #    set NoRun 1
    #    gdbtk_busy
    debug "FillMixed $win $tagname $filename $funcname $line $addr $pc_addr"
    if {$winname == "twin"} {
      set pane "pane1"
    } else {
      set pane "pane2"
    }
    
    if {$funcname == ""} {
      pack forget $winp
      # use scratch widget
      set winp $Stwc(gdbtk_scratch_widget)
      set win [$winp subwidget text]
      $win delete 0.0 end
      $win insert 0.0 "Select function name to disassemble"
      pack $winp -expand yes -fill both -in [$this.p subwidget $pane]
    } elseif {$funcname != $current(funcname) || $mode_changed} {
      set mode_changed 0
      set oldwinp $winp
      if {[LoadFromCache $winname $winnamep $funcname 1]} {
	if {[catch {set dis [gdb_disassemble source $addr]} mess] } {
	  # print some intelligent error message
	  debug "Disassemble Failed: $mess"
	  UnLoadFromCache $winname $winnamep $oldwinp $funcname
	  set current(line) $line
	  set current(tag) $tagname
	  set current(addr) $addr
	  set current(funcname) $funcname
	  set current(filename) $filename
	  set oldmode MIXED
	  $parent.stat.mode configure -value ASSEMBLY
	  return
	} else {
	  # Due to the bizarre and perverted ways that output gets
	  # written into buffers that eventually get captured and
	  # passed back to tcl, if the source file can not be read
	  # an error message will be in the first line and the following
	  # lines will just say "in filename.c".  We could disallow
	  # this entirely, but it may actually be useful to see where
	  # the source lines are in the assembly code, even if they
	  # are not displayed.

	  # fill in new widget
	  set i 1
	  if {[info exists _map]} { unset _map }
	  foreach line [split $dis \n] {
	    if {[string index $line 0] != " "} {
	      # Source line
	      regexp {([0-9]*)[ ]*(.*)} $line match srcline srccode
	      if {$srcline != ""} {
		$win insert $i.end "\t$srcline"
		$win insert $i.end "\t\t$srccode\n" source_tag2
		set _map(srcline=$srcline) $i
	      }
	    } else {
	      # Assembly line
	      set address [lindex [split $line] 4]
	      if {$asm_lo_addr == ""} {set asm_lo_addr $address}
	      set line [split [string trim $line]]
	      set lineNum [lindex $line 0]
	      set offset  [lindex $line 1]
	      set code [join [lrange $line 2 end]]
	      #debug "lineNum = $lineNum\noffset = $offset\n code=$code\n"
	      $win insert $i.0 "- " break_tag
	      set len [string length $lineNum]
	      if { $len < 10} {
		set fudge "          "
		set space [string range $fudge 0 [expr {10-$len}]]
	      } else {
		set space ""
	      }
	      $win insert $i.end "$space$lineNum" break_tag
	      set len [string length $offset]
	      if {$len < 14} {
		set fudge "                   "
		set space [string range $fudge 0 [expr {14-$len}]]
	      } else {
		set space ""
	      }
	      $win insert $i.end " $offset$space" source_tag
	      $win insert $i.end " $code\n" source_tag
	      
	      set asm_hi_addr $address
	      set _map(pc=$address) $i
	      set _map(line=$i) $address
	    }
	    incr i
	  }
	}
      }
      set current(filename) $filename
      # now set the breakpoints
      set do_display_breaks 1
    }
    # highlight proper line number
    if {[info exists _map(pc=$addr)]} {
      # if current file has PC, highlight that too
      if {$gdb_running && $tagname != "PC_TAG" && $pc(filename) == $filename
	  && $pc(func) == $funcname} {
	set pc(asm_line) $_map(pc=$pc_addr)
	$win tag add PC_TAG $pc(asm_line).2 $pc(asm_line).end
      }
      set current(asm_line) $_map(pc=$addr)
      debug "current(asm_line) = $current(asm_line)"
      if {$pc_addr != $addr || $tagname == "PC_TAG"} {
	# HACK.  In STACK mode we usually want the previous instruction
	if {$tagname == "STACK_TAG"} {
	  incr current(asm_line) -1
	}
	$win tag add $tagname $current(asm_line).2 $current(asm_line).end
      }
    }
    display_line $win $current(asm_line)
#    gdbtk_idle
#    set NoRun 0
  }

  # ------------------------------------------------------------------
  # METHOD: location - display a location in a file
  # ------------------------------------------------------------------
  method location {tagname filename funcname line addr pc_addr} {
    debug "Tlocation: $tagname $filename $line $addr $pc_addr,  mode=$current(mode) oldmode=$oldmode  cf=$current(filename)"
    
    ClearTags

    if {$tagname == "PC_TAG" && $addr == $pc_addr} {
      set pc(filename) $filename
      set pc(line) $line
      set pc(addr) $addr
      set pc(func) $funcname
    }
    
    if {$oldmode != "" && $filename != $current(filename)} {
      if {[gdb_find_file $filename] != ""} {
	set tmp $oldmode
	set oldmode ""
	$parent mode "" $tmp 0
      }
    }

    switch $current(mode) {
      SOURCE {
	FillSource twin $tagname $filename $funcname $line $addr $pc_addr
      }
      ASSEMBLY {
	FillAssembly twin $tagname $filename $funcname $line $addr $pc_addr
      }
      MIXED {
	FillMixed twin $tagname $filename $funcname $line $addr $pc_addr
      }
      SRC+ASM {
	FillSource twin $tagname $filename $funcname $line $addr $pc_addr
	FillAssembly bwin $tagname $filename $funcname $line $addr $pc_addr	
      }
    }
    set current(line) $line
    set current(tag) $tagname
    set current(addr) $addr
    set current(funcname) $funcname
    set current(filename) $filename
    if {$do_display_breaks} {
      display_breaks
      set do_display_breaks 0
    }
  }

  # ------------------------------------------------------------------
  #  METHOD:  LoadFile - loads in a new source file
  # ------------------------------------------------------------------
  method LoadFile {winname name} {
    debug "LoadFile $name $current(filename) $current(mode)"
    upvar $winname win
    set winnamep ${winname}p
    upvar $winnamep winp

    set oldwinp $winp
    if {[LoadFromCache $winname $winnamep $name 0]} {
      debug "READING $name"
      if {[catch {gdb_loadfile $win $name $Linenums} msg]} {
	debug "Error opening $name:  $msg"
	#if {$msg != ""} {
	#  tk_messageBox -icon error -title "GDB" -type ok \
	#    -modal task -message $msg
	#}
	UnLoadFromCache $winname $winnamep $oldwinp $name
	return 0
      }
    }
    set current(filename) $name
    # Display all breaks/traces
    set do_display_breaks 1
    return 1
  }

  # ------------------------------------------------------------------
  #  METHOD:  display_line - make sure a line is displayed and near the center
  # ------------------------------------------------------------------

  method display_line { win line } {
    @@update idletasks
    # keep line near center of display
    set pixHeight [winfo height $win]
    set topLine [lindex [split [$win index @0,0] .] 0]
    set botLine [lindex [split [$win index @0,${pixHeight}] .] 0]    
    set margin [expr {int(0.2*($botLine - $topLine))}]
    if {$line < [expr {$topLine + $margin}]} {
      set num [expr {($topLine - $botLine) / 2}]
    } elseif {$line > [expr {$botLine - $margin}]} {
      set num [expr {($botLine - $topLine) / 2}]
    } else {
      set num 0
    }
    $win yview scroll $num units
    $win see $line.0
  }

  # ------------------------------------------------------------------
  # METHOD: display_breaks - insert all breakpoints and tracepoints
  # uses current(filename) in SOURCE mode
  # ------------------------------------------------------------------
  protected do_display_breaks 0	;# flag

  method display_breaks {} {
    debug "T display_breaks"

    # clear any previous breakpoints
    # Note: this procedure only works because breakpoints
    # are currently the only embedded images we use...
    foreach {foo bar index} [$twin dump -image 1.0 end] {
      $twin image configure $index -image {}
      $twin delete $index
      $twin insert $index "-" break_tag
      insertBreakTag $twin $index break_tag
    }

    # now do second pane if it exists
    if {[info exists bwin]} {
      foreach {foo bar index} [$bwin dump -image 1.0 end] {
	$bwin image configure $index -image {}
	$bwin delete $index
	$bwin insert $index "-" break_tag
	insertBreakTag $bwin $index break_tag
      }
    }

    # Display any existing breakpoints.
    foreach bpnum [gdb_get_breakpoint_list] {
      set info [gdb_get_breakpoint_info $bpnum]
      set addr [lindex $info 3]
      set line [lindex $info 2]
      set file [lindex $info 0]
      bp create $bpnum $addr $line $file [lindex $info 6]
    }
    # Display any existing tracepoints.
    foreach bpnum [gdb_get_tracepoint_list] {
      set info [gdb_get_tracepoint_info $bpnum]
      set addr [lindex $info 3]
      set line [lindex $info 2]
      set file [lindex $info 0]
      bp create $bpnum $addr $line $file tracepoint
    }
  }
  
  # ------------------------------------------------------------------
  # METHOD: insertBreakTag - insert the right amount of tag chars
  #         into the text window WIN, at index, where index MUST
  #         be the start of a line, for example "42.0".
  # ------------------------------------------------------------------
  method insertBreakTag {win index tag} {
    debug "insertBreakTag $win $index $tag"

    foreach t {bp_tag break_tag} {
      set tline [$win tag nextrange $t $index]
      if {[string compare [lindex $tline 0] $index] == 0} {
	set stop [lindex $tline 1]
	if {$stop != ""} {
	  $win tag remove $t $index $stop
	  $win tag add $tag $index $stop
	}
	break
      }
    }
  }

  # ------------------------------------------------------------------
  #  METHOD:  bp - set and remove breakpoints
  #
  #  if $addr is valid, the breakpoint will be set in the assembly or 
  #  mixed window at that address.  If $line and $file are valid, 
  #  a breakpoint will be set in the source window if appropriate.
  # ------------------------------------------------------------------
  method bp {action bpnum addr {linenum {}} {file {}} {type 0} } {
    debug "bp: $action addr=$addr line=$linenum file=$file type=$type current(filename)=$current(filename)"
    
    switch $current(mode) {
      SOURCE {
	if {$file == $current(filename) && $linenum != {}} {
	  do_bp $twin $action $linenum $type $bpnum
	}
      }

      SRC+ASM {
	if {$addr != {} && [info exists _map(pc=$addr)]} {
	  do_bp $bwin $action $_map(pc=$addr) $type $bpnum
	}
	if {$file == $current(filename) && $linenum != {}} {
	  do_bp $twin $action $linenum $type $bpnum
	}
      }

      ASSEMBLY {
	if {$addr != {} &&[info exists _map(pc=$addr)]} {
	  do_bp $twin $action $_map(pc=$addr) $type $bpnum
	}
      }

      MIXED {
	if {$addr != {} && [info exists _map(pc=$addr)]} {
	  do_bp $twin $action $_map(pc=$addr) $type $bpnum
	}
      }
    }
  }

  # ------------------------------------------------------------------
  #  METHOD:  do_bp - bp helper function
  # ------------------------------------------------------------------
  method do_bp { win action linenum type bpnum } {
    debug "do_bp: $action line=$linenum type=$type"

    if {$action == "modify" || $action == "delete"} {
      catch {$win image configure $linenum.0 -image {}}
      $win delete $linenum.0
      $win insert $linenum.0 "-" break_tag
      insertBreakTag $win $linenum.0 break_tag
    }

    if {$action == "modify" || $action == "create"} {
      if {$type == 0} {
	debug "ERROR:  no bp type specified in create call."
      }
      
      $win delete $linenum.0
      
      if {$type == "tracepoint"} {
	if {[lindex [gdb_get_tracepoint_info $bpnum] 4] == 0} {
	  set type disabled
	}
      } elseif {[lindex [gdb_get_breakpoint_info $bpnum] 5] == "0" } {
	set type disabled
      }
      
      switch $type {
	donttouch {
	  $win image create $linenum.0 -image $sImage
	  insertBreakTag $win $linenum.0 bp_tag
	}
	delete {
	  $win image create $linenum.0 -image $tImage
	  insertBreakTag $win $linenum.0 bp_tag
	}
	disabled {
	  $win image create $linenum.0 -image $dImage
	  insertBreakTag $win $linenum.0 bp_tag
	}
	tracepoint {
	  $win image create $linenum.0 -image $trImage
	  insertBreakTag $win $linenum.0 break_tag
	}
	default {
	  debug "UNKNOWN BP TYPE $action $type"
	  win insert $linenum.0 "X" bp_tag
	  insertBreakTag $win $linenum.0 bp_tag
	}
      }
    }
  }
  
  # ------------------------------------------------------------------
  #  METHOD:  hasBP - see if a line number has a breakpoint set
  # ------------------------------------------------------------------
  method hasBP {win line} {
    if {[$win get $line.0] == "-"} {
      return 1
    }
    return 0
  }
  
  # ------------------------------------------------------------------
  # METHOD: hasBreakpoint - return 1 if line (in source) has a
  #         {break/trace}point set, zero otherwise
  #         Although not eloquent, it is much faster than querying gdb
  # ------------------------------------------------------------------
  method hasBreakpoint {line} {

    set err [catch {$twin image cget $line.0 -image}]
    return [expr {!$err}]
  }
	

  # ------------------------------------------------------------------
  #  METHOD:  bp_line - called when a breakdot is clicked on
  # ------------------------------------------------------------------
  method bp_line {win y {type N}} {
    debug "bp_line: $win $y $type $current(filename) Tracing=$Tracing"

    if {$Running} {
      return
    }
    
    if {$y == -1} {
      set y $saved_arg
    }
    set line [lindex [split [$win index @0,$y] .] 0]
    set name [lindex [@@file split $current(filename)] end]
    if {$win == $twin && $current(mode) != "MIXED" && $current(mode) != "ASSEMBLY"} {
      if {$type == "TC" || [hasBP $win $line]} {
	if {!$Tracing} {
	  switch $type {
	    N {
	      gdb_set_bp $current(filename) $line 3
	    }
	    H {
	      gdb_cmd "hbreak $name:$line"
	    }
	    T {
	      gdb_set_bp $current(filename) $line 0
	    }
	    TC {
	      foreach i [gdb_get_breakpoint_list] {
		set enabled($i) [lindex [gdb_get_breakpoint_info $i] 5]
	      }	    
	      gdb_cmd "disable"
	      gdb_set_bp $current(filename) $line 0
	      gdb_immediate "continue"
	      gdb_cmd "enable"
	      foreach i [gdb_get_breakpoint_list] {
		if {![info exists enabled($i)]} {
		  gdb_cmd "delete $i"
		} elseif {!$enabled($i)} {
		  gdb_cmd "disable $i"
		}
	      }
	    }
	  }
	} else {
	  set_tracepoint $win $y
	}
      } else {
	if {!$Tracing} {
	  gdb_cmd "clear $name:$line"
	} else {
	  set_tracepoint $win $y
	}
      }
    } else {
      set addr $_map(line=$line)
      if { $type == "TC" || [hasBP $win $line]} {
	if {!$Tracing} {
	  switch $type {
	    N {
	      debug "line=$line addr=$addr"
	      gdb_cmd "break *$addr"
	    }
	    H {
	      gdb_cmd "hbreak *$addr"
	    }
	    T {
	      gdb_cmd "tbreak *$addr"
	    }
	    TC {
	      foreach i [gdb_get_breakpoint_list] {
		set enabled($i) [lindex [gdb_get_breakpoint_info $i] 5]
	      }	    
	      gdb_cmd "disable"
	      gdb_cmd "tbreak *$addr"
	      gdb_immediate "continue"
	      gdb_cmd "enable"
	      foreach i [gdb_get_breakpoint_list] {
		if {![info exists enabled($i)]} {
		  gdb_cmd "delete $i"
		} elseif {!$enabled($i)} {
		  gdb_cmd "disable $i"
		}
	      }
	    }
	  }
	} else {
	  set_tracepoint $win $y $addr
	}
      } else { 
	if {!$Tracing} {
	  gdb_cmd "clear *$addr"
	} else {
	  set_tracepoint $win $y $addr
	}
      }
    }
  }


  # ------------------------------------------------------------------
  #  METHOD:  do_popup - convenience function for popups
  # ------------------------------------------------------------------
  protected saved_arg {}	;# static variable for do_popup

  method do_popup { win X Y x y } {
    debug "do_popup $win $X $Y $x $y"
    if {$Running || [winfo ismapped $win]} { return }  
    if {$x == -1} {
      set saved_arg $y
      tk_popup $win $X $Y
      return
    }
    set range 0;			# line range flag
    if {[catch {$twin get sel.first sel.last} saved_arg]} {
      set saved_arg [$twin get "@$x,$y wordstart" "@$x,$y wordend"] 
      debug "saved_arg = $saved_arg"
    }

    # Must make sure we don't get the line number
    # REMOVE THIS WHEN THE SELECTION EXCLUDES THE LINENUMBERS -- !!
    # This is a little overkill -- don't really need to do this for EVERY line
    set lines {}
    set all [split $saved_arg \n]
    foreach line $all {
      if {[regexp -- (^-?)\t(\[0-9\]+)\t $line text]} {
	regsub -- $text $line {} newline
	lappend lines $newline
      } else {
	lappend lines $line
      }
    }
    set saved_arg [join $lines \n]

    if {[regsub -all \n $saved_arg "" dummy] > 1} {
      # We have multiple lines ==> possible line range
      set range 1
      set ranges [$twin tag ranges sel]
      set range_low  [lindex [split [lindex $ranges 0] .] 0]
      set range_high [lindex [split [lindex $ranges 1] .] 0]
    }

    # Get just the variable name with this selection (if any)
    set variable [lindex [getVariable -1 -1 $saved_arg] 0]
    # LAME: check to see if VARIABLE is really a number (constants??)
    set is_var [catch {expr {$variable+1}}]
    
    $twin.menu delete 0 end
    
    if {$variable != ""} {
      if {$is_var} {
	$twin.menu add command -label "Add $variable to Watch" \
	  -command [list $this addToWatch $variable]
      }

      $twin.menu add command -label "Dump Memory at $variable" \
	-command [list manage create mem -addr_exp $saved_arg]      
    }

    if {$range && $Tracing} {
      $twin.menu add command -label "Set Tracepoint Range" \
	-command "$this tracepoint_range $twin $range_low $range_high"
    }
    $twin.menu add separator
    $twin.menu add command -label "Open Another Source Window" \
      -command {manage create src}
    #$twin.menu add command -label "Preferences" -command {manage create pref}
    tk_popup $win $X $Y 
  }

  # ------------------------------------------------------------------
  # METHOD:  addToWatch - add a variable to the watch window
  # ------------------------------------------------------------------
  method addToWatch {var} {
    [manage open watch] add $var
  }

  # ------------------------------------------------------------------
  #  METHOD:  do_key  -- wrapper for all key bindings
  # ------------------------------------------------------------------
  method do_key {key} {    
    if {!$Running} {
      switch $key {
	print        { print }
	download     { download_it }
	run          { run_executable }
	stack        { manage open stack }
	registers    { manage open reg }
	memory       { manage open mem }
	watch        { manage open watch }
	locals       { manage open locals }
	breakpoints  { manage open bp }
	console      { manage open console }
	step         { catch {gdb_immediate step} }
	next         { catch {gdb_immediate next} }
	finish       { catch {gdb_immediate finish} }
	continue     { catch {gdb_immediate continue} }
	stepi        { catch {gdb_immediate stepi} }
	nexti        { catch {gdb_immediate nexti} }
	up           { catch {gdb_cmd up} }
	down         { catch {gdb_cmd down} }
	quit         { gdbtk_quit }
	toggle_debug { toggle_debug_mode }
        tdump        { manage open tdump }
        tracepoints  { manage open tp}
        tfind_next   { catch {gdb_immediate tfind} }
        tfind_prev   { catch {gdb_immediate "tfind -"} }
        tfind_start  { catch {gdb_immediate "tfind start"} }
        tfind_line   { catch {gdb_immediate "tfind line"} }
        tfind_tp     { catch {gdb_immediate "tfind tracepoint"} }
	open         { catch {$this.toolbar _open_file} }
	browser      { catch {manage open browser} }
	thread_list  { catch {manage open process} }

	default      { debug "Unknown key binding: \"$key\"" }
      }
    } else {
      debug "ignoring keypress -- running"
    }
  }

  # ------------------------------------------------------------------
  #  METHOD:  mode_get - get the source mode
  # ------------------------------------------------------------------
  method mode_get {} {
    return $current(mode)
  }
  
  # ------------------------------------------------------------------
  #  METHOD:  mode_set - change the source mode
  # ------------------------------------------------------------------
  method mode_set {new_mode {go 1}} {
    debug "mode_set $new_mode"

    if {$new_mode != $current(mode)} {

      if {$current(mode) == "SRC+ASM"} {
	$this.p delete pane2
	unset bwin bwinp
      } elseif {$new_mode == "SRC+ASM"} {
	$this.p add pane2 -min 100 -expand 1
	set pane2 [$this.p subwidget pane2]
	set bwinp $Stwc(gdbtk_scratch_widget)
	set bwin [$bwinp subwidget text]
	debug "bwin=$bwin"
      }

      set current(mode) $new_mode
      set mode_changed 1

      if {$go} {
	location $current(tag) $current(filename) $current(funcname) $current(line) \
	  $current(addr) $pc(addr)
      }
    }
    #set_execution_status
    #after idle focus $top
  }

  # ------------------------------------------------------------------
  #  METHOD:  do_button1 - handle button 1 double-clicks
  # ------------------------------------------------------------------
  method do_button1 {} {
    set t ""
    if {[catch {selection get -displayof $twin} saved_arg]} {
      set t [$twin get "insert wordstart" "insert wordend"] 
    }
    if {$t == ""} {
      set t [$twin get "insert wordstart" "insert wordend"] 
    }
  }

  # ------------------------------------------------------------------
  # METHOD:  cancelMotion - cancel any pending motion callbacks for
  #          the source window's variable balloons
  # ------------------------------------------------------------------
  protected timeoutID {} ;# The timeout ID for the variable balloon help

  method cancelMotion {} {
    catch {after cancel $timeoutID}
  }

  # ------------------------------------------------------------------
  # METHOD:  motion - callback for mouse motion within the source
  #          window's text widget
  # ------------------------------------------------------------------
  method motion {x y} {
    global gdb_running
    cancelMotion

    # Only show help if we are running
    if {$gdb_running} {
      set timeoutID [after $TimeOut "$this showBalloon $x $y"]
    }
  }

  # ------------------------------------------------------------------
  # METHOD:  showBalloon - (possibly) show a variable's value in
  #          a balloon-help widget
  # ------------------------------------------------------------------
  method showBalloon {x y} {
    global ${this}_balloon

    if {$Running} { return }
    $twin tag remove _show_variable 1.0 end 
    catch {tmp delete}

    set variable [getVariable $x $y]
    if {[llength $variable] != 3} {
      return
    }    
    
    # We get the variable name, and its start and stop indices in the text 
    # widget, so all we need to do is set the tag and register the balloon help
    set varName [lindex $variable 0]
    set start   [lindex $variable 1]
    set stop    [lindex $variable 2]

    # Do some validity check. We don't show functions.
    # (This may include func pointers...)
    Variable tmp tmp $varName
    if {[tmp isFunction]} {
      # more?
      tmp delete
      return
    }

    $twin tag add _show_variable $start $stop

    # Get the variable's value
    set value [tmp value]
    set text "$varName=$value"
    register_balloon $text
#    balloon variable $twin ${this}_balloon
    balloon show $twin _show_variable
  }

  # ------------------------------------------------------------------
  # METHOD:  getVariable - get the name of the 'variable' under the
  #          mouse pointer in the text widget
  # ------------------------------------------------------------------
  method getVariable {x y {line {}}} {
    #debug "getVariable $x $y $line"
    if {$x != -1 && $y != -1} {
      # Since we will only be concerned with this line, get it
      set line [$twin get @${x},${y}linestart @${x},${y}lineend]
      #debug "new line=$line"
      set simple 0
    } else {
      # This is not quite right -- still want constants to appear...
      set simple 1
    }

    # The index into LINE that contains the char at which the pointer hangs
    set a [split [$twin index @$x,$y] .]
    set lineNo [lindex $a 0]
    set index  [lindex $a 1]
    set s [string range $line $index end]
    set last {}
    foreach char [split $s {}] {
      if {[regexp -- {[^->a-zA-Z0-9_.]} $char dummy]} {
	break
      }
      lappend last $char
    }
    set last [string trimright [join $last {}] ->]

    # Decrement index for string -- will need to increment it later
    incr index -1
    set tmp [string range $line 0 $index]
    set s {}
    foreach char [split $tmp {}] {
      set s [linsert $s 0 $char]
    }

    set first {}
    foreach char $s {
      if {[regexp -- {[^->a-zA-Z0-9_.]} $char dummy]} {
	break
      }
      set first [linsert $first 0 $char]
    }
    set first [string trimleft [join $first {}] ->]
    #debug "FIRST=$first\nLAST=$last"

    # Validate the variable
    set variable $first$last
    if {!$simple && ![regexp {^[a-zA-Z_]} $variable dummy]} {
      #debug "Rejecting: $variable"
      return {}
    }

    set err [catch {gdb_cmd "set variable $variable"} txt]
    if {$err} {
      if {$simple} {
	# don't reject constants...
	set err [catch {gdb_cmd "print $variable"}]
	if {$err} {
	  #debug "rejecting: $variable ([string trim $txt \ \r\n])"
	  return {}
	}
      } else {
	#debug "rejecting: $variable ([string trim $txt \ \r\n])"
	return {}
      }
    }

    incr index
    # Find the boundaries of this word in the text box
    set a [string length $first]
    set b [string length $last]

    # Gag! If there is a breakpoint at a line, this is off by one!
    if {[hasBreakpoint $lineNo]} {
      incr a -1
      incr b 1
    }
    set start "$lineNo.[expr {$index - $a}]"
    set end   "$lineNo.[expr {$index + $b}]"
    return [list $variable $start $end]
  }

  # ------------------------------------------------------------------
  #  METHOD:  trace_help - update statusbar with ballon help message
  # ------------------------------------------------------------------
  method trace_help {args} {
    global ${this}_balloon
    set a [set ${this}_balloon] 
    if {$a == ""} {
      $parent set_status
    } else {
      $parent set_status $a 1
    }
  }

  method register_balloon {text} {
    balloon register $twin $text _show_variable
  }

  method line_is_executable {win line} {
    # there should be an image or a "-" on the line
    set res [catch {$win image cget $line.0 -image}]
    if {!$res || [$win get $line.0] == "-"} {
      return 1
    }
    return 0
  }
 

  # ------------------------------------------------------------------
  # METHOD: set_tracepoint - initiate a tracepoint add/edit
  # ------------------------------------------------------------------
  method set_tracepoint {win y {addr {}}} {
    if {$y == -1} {
      set y $saved_arg
    }
    set line [lindex [split [$win index @0,$y] .] 0]
    set name [lindex [@@file split $filename] end]    
    if {$addr == ""} {
      manage create tracedlg -File $name -Lines $line
    } else {
      manage create tracedlg -File $name -Addresses [list $addr]
    }
  }


  # ------------------------------------------------------------------
  # METHOD:   tracepoint_range - create tracepoints at every line in
  #           a range of lines on the screen
  # ------------------------------------------------------------------
  method tracepoint_range {win low high} {
    switch $current(mode) {
      SOURCE {
	set lines {}
	for {set i $low} {$i <= $high} {incr i} {
	  if {[line_is_executable $win $i]} {
	    lappend lines $i
	  }
	}
      }

      ASSEMBLY {
	set addrs {}
	for {set i $low} {$i <= $high} {incr i} {
	  lappend addrs $_map(line=$i)
	}
      }

      MIXED {
	set addrs {}
	for {set i $low} {$i <= $high} {incr i} {
	  if {[line_is_executable $win $i]} {
	    lappend addrs $_map(line=$i)
	  }
	}
      }

      SRC+ASM {
	if {$win == $awin} {
	  # Assembly
	  set addrs {}
	  for {set i $low} {$i <= $high} {incr i} {
	    lappend addrs $_map(line=$i)
	  }
	} else {
	  # Source
	  set lines {}
	  for {set i $low} {$i <= $high} {incr i} {
	    if {[line_is_executable $win $i]} {
	      lappend lines $i
	    }
	  }
	}
      }
    }
    
    if {[info exists lines]} {
      if {[llength $lines]} {
	set name [@@file tail $filename]
	manage create tracedlg -File $name -Lines $lines
      }
    } else {
      if {[llength $addrs]} {
	set name [@@file tail $filename]
	manage create tracedlg -File $name -Addresses $addrs
      }
    }

    # Clear the selection -- it looks a lot better.
    $twin tag remove sel 1.0 end
  }


  # ------------------------------------------------------------------
  #  METHOD:  search - search for text or jump to a specific line
  #           in source window, going in the specified DIRECTION.
  # ------------------------------------------------------------------
  protected SearchIndex 1.0	;# static

  method search {exp direction} {
    if {$exp != ""} {
      set result {}
      if {[regexp {^@([0-9]+)} $exp dummy index]} {
	append index .0
	set end [$twin index "$index lineend"]
      } else {
	set index [$twin search -exact -count len -$direction $exp $SearchIndex]
	
	if {$index != ""} {
	  set end [split $index .]
	  set line [lindex $end 0]
	  set char [lindex $end 1]
	  set char [expr {$char + $len}]
	  set end $line.$char
	  set result "Match of \"$exp\" found on line $line"
	  if {$direction == "forwards"} {
	    set SearchIndex $end
	  } else {
	    set SearchIndex $index
	  }
	}
      }
      if {$index != ""} {
	# Highlight word and save index
	$twin tag remove search 1.0 end
	$twin tag add search $index $end
	$twin see $index
      } else {
	set result "No match for \"$exp\" found"
      }
      return $result
    } else {
      $twin tag remove search 1.0 end
    }
  }

  # ------------------------------------------------------------------
  #  METHOD:  LoadFromCache - load a cached widget or create a new one
  #  winname  - "twin" or "bwin"
  #  winnamep - "twinp" or "bwinp"
  #  name     - name widget is cached under
  #  asm      - argument for config_win
  # ------------------------------------------------------------------
  method LoadFromCache {winname winnamep name asm} {
    debug "LoadFromCache $winname $winnamep $name $asm"
    upvar $winname win
    upvar $winnamep winp
    if {$winname == "twin"} {
      set pane "pane1"
    } else {
      set pane "pane2"
    }
    pack forget $winp
    if {[info exists Stwc($name)]} {
      debug "READING CACHE"
      set winp $Stwc($name)
      set win [$winp subwidget text]
      if {$asm} {
	upvar #0 Mc${name} _map
      }
      set res 0
    } else {
      incr filenum
      set Stwc($name) [tixScrolledText $this.stext${filenum} -scrollbar "auto +y"]
      set winp $Stwc($name)
      set win [$winp subwidget text]
      if {$asm} {
	upvar #0 Mc${name} _map
      }
      set res 1
    }
    # reconfigure in case some preferences have changed
    config_win $win $asm
    # breakpoints might have changed, too
    pack $winp -expand yes -fill both -in [$this.p subwidget $pane]
    return $res
  }

  # ------------------------------------------------------------------
  #  METHOD:  UnLoadFromCache - revert back to previously cached widget
  #  This is used when a new widget is created with LoadFromCache but
  #  there is a problem with filling the widget.  
  # ------------------------------------------------------------------
  method UnLoadFromCache {winname winnamep oldwinp name} {
    debug "UnLoadFromCache $winname $winnamep $oldwinp $name"
    upvar $winname win
    upvar $winnamep winp
    if {$winname == "twin"} {
      set pane "pane1"
    } else {
      set pane "pane2"
    }
    pack forget $winp
    unset Stwc($name)
    pack $oldwinp -expand yes -fill both -in [$this.p subwidget $pane]
    set winp $oldwinp
    set win [$winp subwidget text]
  }

  # ------------------------------------------------------------------
  #  METHOD:  print - print the contents of the text widget
  # ------------------------------------------------------------------
  method print {top} {
    # FIXME
    send_printer -ascii [$twin get 1.0 end] -parent $top
  }

  # ------------------------------------------------------------------
  #  METHOD:  config - used to change public attributes
  # ------------------------------------------------------------------
  method config {config} {}
  
  public Tracing	;# if we are running in trace mode
  public parent {}	;# the parent SrcWin
  public ignore_var_balloons 0;  # ignore all variable balloons

  protected top		;# toplevel window

  protected twinp	;# top scrolled window of pane
  protected twin	;# top text window of pane
  protected bwinp	;# bottom scrolled window of pane
  protected bwin	;# bottom text window of pane

  protected UseVariableBalloons

  protected mode_changed 0
  protected current	;# our current state
  protected pc		;# where the PC is now
  protected oldmode ""	;# remember the mode we want, even if we can't have it

  protected Running 0	;# another way to disable things while target is active
  protected Linenums	;# use linenumbers?

  # needed for assembly support
  protected _map

  # cache is not shared among windows yet.  That could be a later
  # optimization
  protected Stwc	;# Source Text Window Cache
  protected Mc		;# _map cache
  protected filenum 0

  # common variables are shared among all objects of this type
  common sImage
  common tImage
  common dImage
  common trImage
  common TimeOut 100 ;# The timeout value for variable balloon help
}


