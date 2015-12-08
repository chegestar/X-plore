#
# RegWin
# ----------------------------------------------------------------------
# Implements register window for gdb
#
#   PUBLIC ATTRIBUTES:
#
#
#   METHODS:
#
#     reconfig ....... called when preferences change
#
#
#   X11 OPTION DATABASE ATTRIBUTES
#
#
# ----------------------------------------------------------------------
#   AUTHOR:  Martin M. Hunt <hunt@cygnus.com>
#   Copyright (C) 1997, 1998 Cygnus Solutions
#

itcl_class RegWin {
  # ------------------------------------------------------------------
  #  CONSTRUCTOR - create new register window
  # ------------------------------------------------------------------
  constructor {config} {
    #
    #  Create a window with the same name as this object
    #
    global tixOption
    set class [$this info class]
    @@rename $this $this-tmp-
    @@frame $this -class $class 
    @@rename $this $this-win-
    @@rename $this-tmp- $this

    wm withdraw [winfo toplevel $this]
    gdbtk_busy

    set NormalForeground $tixOption(fg)
    set HighlightForeground [pref get gdb/reg/highlight_fg]

    if {[pref getd gdb/reg/menu] != ""} {
      set mbar 0
    }

    init_reg_display_vars 1
    build_win
    gdbtk_idle
    add_hook gdb_update_hook "$this update"
    add_hook gdb_busy_hook [list $this busy]
    add_hook gdb_idle_hook [list $this idle]
  }

  # ------------------------------------------------------------------
  #  METHOD: init_reg_display_vars - init the list of registers displayed
  # ------------------------------------------------------------------
  method init_reg_display_vars {args} {
    global reg_display max_regs
    set reg_display_list {}
    set regnames [gdb_regnames]
    set i 1
    set rn 0
    foreach r $regnames {
      set reg_display($rn,name) $r
      set format [pref getd gdb/reg/$r-format]
      if {$format == ""} { set format x }
      set reg_display($rn,format) $format
      if {$args != "" && [pref getd gdb/reg/$r] == "no"} {
	set reg_display($rn,line) 0
      } else {
	set reg_display($rn,line) $i
	lappend reg_display_list $rn
	incr i
      }
      incr rn
    }
    set num_regs [expr $i - 1]
    set max_regs $rn
  }

  # ------------------------------------------------------------------
  #  PROC: save_reg_display_vars - save the list of displayed registers
  #  to the preferences file
  # ------------------------------------------------------------------
  proc save_reg_display_vars {} {
    global reg_display max_regs
    set rn 0
    while {$rn < $max_regs} {
      set name $reg_display($rn,name)
      if {$reg_display($rn,line) == 0} {
	pref setd gdb/reg/$name no
      } else {
	pref setd gdb/reg/$name {}
      }
      if {$reg_display($rn,format) != "x"} {
	pref setd gdb/reg/$name-format $reg_display($rn,format)
      } else {
	pref setd gdb/reg/$name-format {}
      }
      incr rn
    }
    pref_save ""
  }

  # ------------------------------------------------------------------
  #  METHOD:  build_win - build the main register window
  # ------------------------------------------------------------------
  method build_win {} {
    global reg_display tixOption tcl_platform
    
    set dim [dimensions]
    set nRows [lindex $dim 0]
    set nCols [lindex $dim 1]
    if {$tcl_platform(platform) == "windows"} {
      tixScrolledWindow $this.scrolled -scrollbar both -sizebox 1
    } else {
      tixScrolledWindow $this.scrolled -scrollbar auto
    }
    set ScrolledWin [$this.scrolled subwidget window]
    # Create labels
    set row 0
    set col 0

    set regMaxLen 0
    foreach r [gdb_regnames] {
      set l [string length $r]
      if {$l > $regMaxLen} {
	set regMaxLen $l
      }
    }

    set vmax 0
    foreach r $reg_display_list {
      if {[catch {gdb_fetch_registers $reg_display($r,format) $r} values($r)]} {
	set values($r) ""
      } else {
	set values($r) [string trim $values($r) \ ]
      }
      set l [string length $values($r)]
      if {$l > $vmax} {
	set vmax $l
      }
    }

    foreach r $reg_display_list {
      if {$row == $nRows} {
	grid columnconfigure $ScrolledWin $col -weight 1
	set row 0
	incr col
      }

      frame $ScrolledWin.$r -takefocus 1
      bind $ScrolledWin.$r <Up> "$this reg_select_up"
      bind $ScrolledWin.$r <Down> "$this reg_select_down"
      bind $ScrolledWin.$r <Tab> "$this reg_select_down"
      bind $ScrolledWin.$r <Left> "$this reg_select_left"
      bind $ScrolledWin.$r <Right> "$this reg_select_right"
      if {![pref get gdb/mode]} {
	bind $ScrolledWin.$r <Return> "$this edit $r"
      }

      label $ScrolledWin.$r.lbl -text [fixLength $reg_display($r,name) $regMaxLen left] \
	-relief solid -bd 1 -font src-font 
      label $ScrolledWin.$r.val -anchor e -text [fixLength $values($r) $vmax right] \
	-relief ridge -bd 1 -font src-font -bg $tixOption(input1_bg)

      grid $ScrolledWin.$r.lbl $ScrolledWin.$r.val -sticky nsew
      grid columnconfigure $ScrolledWin.$r 1 -weight 1
      grid $ScrolledWin.$r -colum $col -row $row -sticky nsew
#      grid rowconfigure $ScrolledWin $row -weight 1
      bind $ScrolledWin.$r.val <1> "$this reg_select $r"
      bind $ScrolledWin.$r.lbl <1> "$this reg_select $r"
      bind $ScrolledWin.$r.val <3> "$this but3 $r %X %Y"
      bind $ScrolledWin.$r.lbl <3> "$this but3 $r %X %Y"
      if {![pref get gdb/mode]} {
	bind $ScrolledWin.$r.lbl <Double-1> "$this edit $r"
	bind $ScrolledWin.$r.val <Double-1> "$this edit $r"
      }
      incr row
    }
    grid columnconfigure $ScrolledWin $col -weight 1


    if { $mbar } {
      menu $this.m -tearoff 0
      [winfo toplevel $this] configure -menu $this.m
      $this.m add cascade -menu $this.m.reg -label "Register" -underline 0
      set m [menu $this.m.reg]
      if {![pref get gdb/mode]} {
	$m add command -label "Edit" -underline 0 -state disabled
      }
      $m add cascade -menu $this.m.reg.format -label "Format" -underline 0
      set f [menu $this.m.reg.format]
      $f add radio -label "Hex" -value x -underline 0 -state disabled \
	-command "$this update"
      $f add radio -label "Decimal" -value d -underline 0 -state disabled \
	-command "$this update"
      $f add radio -label "Natural" -value {} -underline 0 -state disabled \
	-command "$this update"
      $f add radio -label "Binary" -value t -underline 0 -state disabled \
	-command "$this update"
      $f add radio -label "Octal" -value o -underline 0 -state disabled \
	-command "$this update"
      $f add radio -label "Raw" -value r -underline 0 -state disabled \
	-command "$this update"
      $m add command -label "Remove from Display" -underline 0 -state disabled
      $m add separator
      $m add command -label "Display All Registers" -underline 0 -state disabled \
	-command "$this display_all"
    }

    set Menu [menu $ScrolledWin.pop -tearoff 0]
    set disabled_fg [$Menu cget -fg]
    $Menu configure -disabledforeground $disabled_fg

    # Clear gdb's changed list
    catch {gdb_changed_register_list}

    pack $this.scrolled -anchor nw -fill both -expand yes
  }
  # ------------------------------------------------------------------
  #  METHOD:  reg_select_up
  # ------------------------------------------------------------------
  method reg_select_up { } {
    if { $selected == -1 || $Running} {
      return
    }
    set current_index [lsearch -exact $reg_display_list $selected]
    set new_reg [lindex $reg_display_list [expr {$current_index - 1}]]
    if { $new_reg != {} } {
      $this reg_select $new_reg
    }
  }
  # ------------------------------------------------------------------
  #  METHOD:  reg_select_down
  # ------------------------------------------------------------------
  method reg_select_down { } {
    if { $selected == -1 || $Running} {
      return
    }
    set current_index [lsearch -exact $reg_display_list $selected]
    set new_reg [lindex $reg_display_list [expr {$current_index + 1}]]
    if { $new_reg != {} } {
      $this reg_select $new_reg
    }
  }
  # ------------------------------------------------------------------
  #  METHOD:  reg_select_right
  # ------------------------------------------------------------------
  method reg_select_right { } {
    if { $selected == -1 || $Running} {
      return
    }
    set current_index [lsearch -exact $reg_display_list $selected]
    set new_reg [lindex $reg_display_list [expr {$current_index + $nRows}]]
    if { $new_reg != {} } {
      $this reg_select $new_reg
    }
  }
  # ------------------------------------------------------------------
  #  METHOD:  reg_select_left
  # ------------------------------------------------------------------
  method reg_select_left { } {
    if { $selected == -1 || $Running} {
      return
    }
    set current_index [lsearch -exact $reg_display_list $selected]
    set new_reg [lindex $reg_display_list [expr {$current_index - $nRows}]]
    if { $new_reg != {} } {
      $this reg_select $new_reg
    }
  }
  # ------------------------------------------------------------------
  #  METHOD:  reg_select - select a register
  # ------------------------------------------------------------------
  method reg_select { r } {
    global tixOption 
    
    if $Running { return }
    if {$selected != -1} {
      catch {$ScrolledWin.$selected.lbl configure -fg $tixOption(fg) -bg $tixOption(bg)}
      catch {$ScrolledWin.$selected.val configure -fg $tixOption(fg) \
	-bg $tixOption(input1_bg)}
    }

    # if we click on the same line, unselect it and return
    if {$selected == $r} {
      set selected -1
      $this.m.reg entryconfigure 0 -state disabled
      $this.m.reg entryconfigure 2 -state disabled
      for {set i 0} {$i < 6} {incr i} {
	$this.m.reg.format entryconfigure $i -state disabled
      }
      return
    }

    if {$Editing != -1} {
      unedit
    }

    $ScrolledWin.$r.lbl configure -fg $tixOption(select_fg) -bg $tixOption(select_bg)
    $ScrolledWin.$r.val configure -fg $tixOption(fg) -bg $tixOption(bg)

    if {![pref get gdb/mode]} {
      $this.m.reg entryconfigure 0 -state normal -command "$this edit $r"
    }
    $this.m.reg entryconfigure 2 -state normal \
      -command "$this delete_from_display_list $r"
      for {set i 0} {$i < 6} {incr i} {
	$this.m.reg.format entryconfigure $i -state normal \
	  -variable reg_display($r,format)
      }
    focus -force $ScrolledWin.$r
    set selected $r
  }
  # ------------------------------------------------------------------
  # METHOD:  dimensions - determine square-like dimensions for
  #          register window
  # ------------------------------------------------------------------
  method dimensions {} {
 
    set rows 16
#    set rows [expr int(floor(sqrt($num_regs)))]
    set cols [expr {int(ceil(sqrt($num_regs)))}]

    return [list $rows $cols]
  }

  # ------------------------------------------------------------------
  # METHOD:  fixLength - return fixed-length string, SIZE characters,
  #          inserting spaces, as necessary. Blanks will be added
  #          either before (WHERE=left) or after (WHERE=right) of the
  #          string S.
  # ------------------------------------------------------------------
  method fixLength {s size where} {
    set blank "                                                                   "
    set len [string length $s]
    set bl  [expr {$size - $len}]
    set b [string range $blank 0 $bl]

    switch $where {
      left  { set fl "$s$b"}
      right { set fl "$b$s"}
    }

    return $fl
  }

  # ------------------------------------------------------------------
  #  METHOD:  but3 - generate and display a popup window on button 3 
  #  over the register value
  # ------------------------------------------------------------------

  method but3 {rn X Y} {
    global reg_display max_regs

    if !$Running {
      $Menu delete 0 end
      $Menu add command -label $reg_display($rn,name) -state disabled
      $Menu add separator
      $Menu add radio -label "Hex" -command "$this update" \
	-value x -variable reg_display($rn,format)
      $Menu add radio -label "Decimal" -command "$this update" \
	-value d -variable reg_display($rn,format)
      $Menu add radio -label "Natural" -command "$this update" \
	-value {} -variable reg_display($rn,format)
      $Menu add radio -label "Binary" -command "$this update" \
	-value t -variable reg_display($rn,format) -underline 0
      $Menu add radio -label "Octal" -command "$this update" \
	-value o -variable reg_display($rn,format)
      $Menu add radio -label "Raw" -command "$this update" \
	-value r -variable reg_display($rn,format)
      $Menu add separator
      $Menu add command  -command "$this delete_from_display_list $rn" \
	-label "Remove $reg_display($rn,name) from Display"
      if {$max_regs != $num_regs} {
	$Menu add separator
	$Menu add command -command "$this display_all" \
	  -label "Display all registers"
      }
      tk_popup $Menu $X $Y
    }
  }

  # ------------------------------------------------------------------
  #  METHOD:  display_all - add all registers to the display list
  # ------------------------------------------------------------------
  method display_all {} {
    init_reg_display_vars
    $this.m.reg entryconfigure 4 -state disabled
    reconfig
  }

  # ------------------------------------------------------------------
  #  METHOD:  delete_from_display_list - remove a register from the
  #  display list
  # ------------------------------------------------------------------
  method delete_from_display_list {rn} {
    global reg_display max_regs
    set reg_display($rn,line) 0
    set reg_display_list {}
    set rn 0
    set i 0
    while {$rn < $max_regs} {
      if {$reg_display($rn,line) > 0} {
	lappend reg_display_list $rn
	incr i
	set reg_display($rn,line) $i
      }
      incr rn
    }
    set num_regs $i
    reconfig
    $this.m.reg entryconfigure 4 -state normal
  }

  # ------------------------------------------------------------------
  #  METHOD:  edit - edit a cell
  # ------------------------------------------------------------------
  method edit {r} {
    global reg_display
    if $Running { return }
    unedit
    
    set Editing $r
    set txt [$ScrolledWin.$r.val cget -text]
    set len [string length $txt]
    set entry [entry $ScrolledWin.$r.ent -width $len -bd 0 -font src-font]
    $entry insert 0 $txt
    
    grid remove $ScrolledWin.$r.val
    grid $entry -row 0 -col 1
    bind $entry <Return> "$this acceptEdit $r"
    bind $entry <Escape> "$this unedit"
    $entry selection to end
    focus $entry    
  }

  # ------------------------------------------------------------------
  # METHOD:  acceptEdit - callback invoked when enter key pressed
  #          in an editing entry
  # ------------------------------------------------------------------
  method acceptEdit {r} {
    global reg_display

    set value [string trimleft [$ScrolledWin.$r.ent get]]
    debug "value=${value}="
    if {$value == ""} {
      set value 0
    }
    if {[catch {gdb_cmd "set \$$reg_display($r,name)=$value"} result]} {
      tk_messageBox -icon error -type ok -message $result \
	-title "Error in Expression" -parent $this
      focus $ScrolledWin.$r.ent
      $ScrolledWin.$r.ent selection to end
    } else {
      unedit
    }
  }

  # ------------------------------------------------------------------
  # METHOD:  unedit - clear any editing entry on the screen
  # ------------------------------------------------------------------
  method unedit {} {
    if {$Editing != -1} {
      destroy $ScrolledWin.$Editing.ent

      # Fill the entry with the old label, updating value
      grid $ScrolledWin.$Editing.val -column 1 -row 0
      focus -force $ScrolledWin.$Editing
      set Editing -1
      update
    }
  }

  # ------------------------------------------------------------------
  #  METHOD:  update - update widget when PC changes
  # ------------------------------------------------------------------
  method update {} {
    global reg_display
    debug "START REGISTER UPDATE CALLBACK"
    if {$reg_display_list == ""
	|| [catch {eval gdb_changed_register_list $reg_display_list} changed_reg_list]} {
      set changed_reg_list {}
    }

    set vmax 0
    foreach r $reg_display_list {
      if {[catch {gdb_fetch_registers $reg_display($r,format) $r} values($r)]} {
	set values($r) ""
      } else {
	set values($r) [string trim $values($r) \ ]
      }
      set l [string length $values($r)]
      if {$l > $vmax} {
	set vmax $l
      }
    }

    foreach r $reg_display_list {
      if {[lsearch -exact $changed_reg_list $r] != -1} {
	set fg $HighlightForeground
      } else {
	set fg $NormalForeground
      }
      $ScrolledWin.$r.val configure -text [fixLength $values($r) $vmax right] \
	-fg $fg
    }
    debug "END REGISTER UPDATE CALLBACK" 
  }

  method idle {} {
    [winfo toplevel $this] configure -cursor {}
    set Running 0
  }

  # ------------------------------------------------------------------
  #  DESTRUCTOR - destroy window containing widget
  # ------------------------------------------------------------------
  destructor {
    remove_hook gdb_update_hook "$this update"
    remove_hook gdb_busy_hook [list $this busy]
    remove_hook gdb_idle_hook [list $this idle]
    set top [winfo toplevel $this]
    destroy $this
    destroy $top
  }

  # ------------------------------------------------------------------
  #  METHOD:  config - used to change public attributes
  # ------------------------------------------------------------------
  method config {config} {}
    
  # ------------------------------------------------------------------
  #  METHOD:  reconfig - used when preferences change
  # ------------------------------------------------------------------
  method reconfig {} {
    destroy $Menu $this.g $this.scrolled $this.m
    gdbtk_busy
    build_win
    gdbtk_idle
  }
  
  # ------------------------------------------------------------------
  #  METHOD:  busy - gdb_busy_hook
  # ------------------------------------------------------------------
  method busy {} {
    # Cancel edits
    unedit

    # Fencepost
    set Running 1

    # cursor
    [winfo toplevel $this] configure -cursor watch
  }

  #
  #  PROTECTED DATA
  #
  protected reg_display_list {}
  protected num_regs 0
  protected nRows
  protected nCols
  protected changed_reg_list {}
  protected oldValue
  protected ScrolledWin
  protected Menu
  protected Editing -1
  protected selected -1
  protected mbar 1
  protected Running 0

  #
  # COMMON DATA
  #
  common HighlightForeground {}
  common NormalForeground {}
}

#
# RegWinPref
# ----------------------------------------------------------------------
# Implements GDB register preferences dialog
#
#   PUBLIC ATTRIBUTES:
#
#   METHODS:
#
#     config ....... used to change public attributes
#
#     PRIVATE METHODS
#
#   X11 OPTION DATABASE ATTRIBUTES
#
#
# ----------------------------------------------------------------------

itcl_class RegWinPref {
  # ------------------------------------------------------------------
  #  CONSTRUCTOR - create new toolbar preferences dialog
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
    build_win
  }

  # ------------------------------------------------------------------
  #  METHOD:  build_win - build the dialog
  # ------------------------------------------------------------------
  method build_win {} {
    if {$attach != 1} {
      frame $this.f
      frame $this.f.a
      frame $this.f.b
      set f $this.f.a
    } else {
      set f $this
    }

    button $f.but -fg red -text "SAVE" -command "RegWin @@ save_reg_display_vars"
    label $f.lab -text "the list\n\
of displayed registers and their\n display formats"
    pack $f.but $f.lab -side left -pady 10

    if {$attach != 1} {
      button $this.f.b.quit -text Quit -underline 0 -command "$this delete"
      pack $this.f.b.quit -side left -expand yes
      pack $this.f.a $this.f.b $this.f $this
    }
  }

  # ------------------------------------------------------------------
  #  DESTRUCTOR - destroy window containing widget
  # ------------------------------------------------------------------
  destructor {
    set top [winfo toplevel $this]
    manage delete $this 1
    destroy $this
    if {$attach != 1} {
      destroy $top
    }
  }

  # ------------------------------------------------------------------
  #  METHOD:  config - used to change public attributes
  # ------------------------------------------------------------------
  method config {config} {}

  # ------------------------------------------------------------------
  #  METHOD:  reconfig - used when preferences change
  # ------------------------------------------------------------------
  method reconfig {} {
    # for now, just delete and recreate
    destroy $this.f
    build_win
  }

  #
  # PUBLIC DATA
  #
  public attach 0
}
