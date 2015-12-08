#
# MemWin - Implements memory dump window for gdb
# ----------------------------------------------------------------------
#
#   PUBLIC ATTRIBUTES:
#
#
#   METHODS:
#
#     reconfig ....... called when preferences change
#     update ......... idle hook
#
#
#   X11 OPTION DATABASE ATTRIBUTES
#
#
# ----------------------------------------------------------------------
#   AUTHOR:  Martin M. Hunt <hunt@cygnus.com>
#   Copyright (C) 1997, 1998 Cygnus Solutions
#

itcl_class MemWin {
  # ------------------------------------------------------------------
  #  CONSTRUCTOR - create new memory dump window
  # ------------------------------------------------------------------
  constructor {config} {
    #
    #  Create a window with the same name as this object
    #
    global _mem tixOption
    set class [$this info class]
    @@rename $this $this-tmp-
    @@frame $this -class $class 
    @@rename $this $this-win-
    @@rename $this-tmp- $this

    set top [winfo toplevel $this]
    wm withdraw $top
    gdbtk_busy

    set _mem($this,enabled) 1
    set bg $tixOption(input1_bg)

    set type(1) char
    set type(2) short
    set type(4) int
    set type(8) "long long"

    if {[pref getd gdb/mem/menu] != ""} {
      set mbar 0
    }
    
    init_addr_exp
    build_win
    gdbtk_idle
    add_hook gdb_update_hook "$this update"
    add_hook gdb_busy_hook [list $this busy]
    add_hook gdb_idle_hook [list $this idle]
  }

  # ------------------------------------------------------------------
  #  METHOD:  build_win - build the main memory window
  # ------------------------------------------------------------------
  method build_win {} {
    global tcl_platform gdb_ImageDir _mem _memval${this}

    set maxlen 0
    set maxalen 0
    set saved_value ""

    if { $mbar } {
      menu $this.m -tearoff 0
      $top configure -menu $this.m
      $this.m add cascade -menu $this.m.addr -label "Addresses" -underline 0
      set m [menu $this.m.addr]
      $m add check -label " Auto Update" -variable _mem($this,enabled) \
	-underline 1 -command "$this toggle_enabled"
      $m add command -label " Update Now" -underline 1 \
	-command "$this update_address" -accelerator {Ctrl+U}
      $m add separator
      $m add command -label " Preferences..." -underline 1 \
	-command "$this create_prefs"
    }

    # Numcols = number of columns of data
    # numcols = number of columns in table (data plus headings plus ASCII)
    # if numbytes are 0, then use window size to determine how many to read
    if {$numbytes == 0} {
      set Numrows 8
    } else {
      set Numrows [expr {$numbytes / $bytes_per_row}]
    }
    set numrows [expr {$Numrows + 1}]
    
    set Numcols [expr {$bytes_per_row / $size}]
    if {$ascii} {
      set numcols [expr {$Numcols + 2}]
    } else {
      set numcols [expr {$Numcols + 1}]
    }

    table $this.t -titlerows 1 -titlecols 1 -variable _memval${this} \
      -roworigin -1 -colorigin -1 -bg $bg \
      -browsecmd "$this changed_cell %s %S" -font src-font\
      -colstretch unset -rowstretch unset -selectmode single \
      -xscrollcommand "$this.sx set" -resizeborders none \
      -cols $numcols -rows $numrows -autoclear 1
  
    if {$numbytes} {
      $this.t configure -yscrollcommand "$this.sy set"
      scrollbar $this.sy -command [list $this.t yview]
    } else {
      $this.t configure -rowstretchmode none
    }
    scrollbar $this.sx -command [list $this.t xview] -orient horizontal
    $this.t tag config sel -bg [$this.t cget -bg] -relief sunken
    $this.t tag config active -bg lightgray -relief sunken -wrap 0

    # rebind all events that use tkTableMoveCell to our local version
    # because we don't want to move into the ASCII column if it exists
    bind $this.t <Up>		"$this memMoveCell %W -1  0; break"
    bind $this.t <Down>		"$this memMoveCell %W  1  0; break"
    bind $this.t <Left>		"$this memMoveCell %W  0 -1; break"
    bind $this.t <Right>	"$this memMoveCell %W  0  1; break"
    bind $this.t <Return>	"$this memMoveCell %W 0 1; break"
    bind $this.t <KP_Enter>	"$this memMoveCell %W 0 1; break"

    # bind button 3 to popup
    bind $this.t <3> "$this do_popup %X %Y"

    # bind Paste and button2 to the paste function
    # this is necessary because we want to not just paste the
    # data into the cell, but we also have to write it
    # out to real memory
    bind $this.t <ButtonRelease-2> [format {after idle %s paste %s %s} $this %x %y]
    bind $this.t <<Paste>> [format {after idle %s paste %s %s} $this %x %y]

    menu $this.t.menu -tearoff 0
    bind_plain_key $top Control-u "$this update_address"

    # bind resize events
    bind $this <Configure> "$this newsize %h"

    frame $this.f
    tixControl $this.f.cntl -label " Address " -value $addr_exp \
      -validatecmd "$this validate" -command "$this update_address_cb" \
      -incrcmd "$this incr_addr -1" -decrcmd "$this incr_addr 1" \
      -options { entry.width 20	entry.font src-font} 

    balloon register [$this.f.cntl subwidget entry] "Address or Address Expression"
    balloon register [$this.f.cntl subwidget incr] "Scroll Up (Decrement Address)"
    balloon register [$this.f.cntl subwidget decr] "Scroll Down (Increment Address)"

    if {!$mbar} {
      button $this.f.upd -command "$this update_address" \
	-image [image create photo -file [@@file join $gdb_ImageDir check.gif]]
      balloon register $this.f.upd "Update Now"
      checkbutton $this.cb -variable _mem($this,enabled) -command "$this toggle_enabled"
      balloon register $this.cb "Toggles Automatic Display Updates"
      grid $this.f.upd $this.f.cntl -sticky ew -padx 5
    } else {
      grid $this.f.cntl x -sticky w
      grid columnconfigure $this.f 1 -weight 1
    }

    # draw top border
    set col 0
    for {set i 0} {$i < $bytes_per_row} { incr i $size} {
      set _memval${this}(-1,$col) [format " %X" $i]
      incr col
    }

    if {$ascii} {
      set _memval${this}(-1,$col) ASCII
    }

    # fill initial display
    if {$nb} {
      update_address
    }

    if {!$mbar} {
      grid $this.f x -row 0 -column 0 -sticky nws
      grid $this.cb -row 0 -column 1 -sticky news
    } else {
      grid $this.f -row 0 -column 0 -sticky news
    }
    grid $this.t -row 1 -column 0 -sticky news
    if {$numbytes} { grid $this.sy -row 1 -column 1 -sticky ns }
    grid $this.sx -sticky ew
    grid columnconfig $this 0 -weight 1
    grid rowconfig $this 1 -weight 1
    focus $this.f.cntl
  }

  # ------------------------------------------------------------------
  #  METHOD:  paste - paste callback. Update cell contents after paste
  # ------------------------------------------------------------------
  method paste {x y} {
    edit [$this.t index @$x,$y]
  }

  # ------------------------------------------------------------------
  #  METHOD:  validate - because the control widget wants this
  # ------------------------------------------------------------------
  method validate {val} {
    return $val
  }

  # ------------------------------------------------------------------
  #  METHOD:  create_prefs - create memory preferences dialog
  # ------------------------------------------------------------------
  method create_prefs {} {
    if {$Running} { return }

    # make sure row height is set
    if {$rheight == ""} {
      set rheight [lindex [$this.t bbox 0,0] 3]
    }

    set prefs_win [manage create mempref -win $this -size $size -format $format \
	      -numbytes $numbytes -bpr $bytes_per_row -ascii $ascii \
	      -ascii_char $ascii_char -color $color]
    wm transient [winfo toplevel $prefs_win] $this
    freeze $prefs_win
  }

  # ------------------------------------------------------------------
  #  METHOD:  changed_cell - called when moving from one cell to another
  # ------------------------------------------------------------------
  method changed_cell {from to} {
    #debug "moved from $from to $to"
    #debug "value = [$this.t get $from]"
    if {$saved_value != ""} {
      if {$saved_value != [$this.t get $from]} {
	edit $from
      }
    }
    set saved_value [$this.t get $to]
  }

  # ------------------------------------------------------------------
  #  METHOD:  edit - edit a cell
  # ------------------------------------------------------------------
  method edit { cell } {
    global _mem _memval${this}

    #debug "edit $cell"
    if {$Running || $cell == ""} { return }
    set rc [split $cell ,]
    set row [lindex $rc 0]
    set col [lindex $rc 1]
    set val [$this.t get $cell]

    if {$col == $Numcols} { 
      # editing the ASCII field
      set addr [expr {$current_addr + $bytes_per_row * $row}]
      set start_addr $addr

      # calculate number of rows to modify
      set len [string length $val]
      set rows 0
      while {$len > 0} { 
	incr rows
	set len [expr {$len - $bytes_per_row}]
      }
      set nb [expr {$rows * $bytes_per_row}]

      # now process each char, one at a time
      foreach c [split $val ""] {
	if {$c != $ascii_char} {
	  if {$c == "'"} {set c "\\'"}
	  catch {gdb_cmd "set *(char *)($addr) = '$c'"}
	}
	incr addr
      }
      set addr $start_addr
      set nextval 0
      # now read back the data and update the widget
      catch {gdb_get_mem $addr $format $size $nb $bytes_per_row $ascii_char} vals
      for {set n 0} {$n < $nb} {incr n $bytes_per_row} {
	set _memval${this}($row,-1) [format "0x%x" $addr]
	for { set col 0 } { $col < [expr {$bytes_per_row / $size}] } { incr col } {
	  set _memval${this}($row,$col) [lindex $vals $nextval]
	  incr nextval
	}
	set _memval${this}($row,$col) [lindex $vals $nextval]
	incr nextval
	incr addr $bytes_per_row
	incr row
      }
      return
    }

    # calculate address based on row and column
    set addr [expr {$current_addr + $bytes_per_row * $row + $size * $col}]
    #debug "  edit $row,$col         [format "%x" $addr] = $val"
    #set memory
    catch {gdb_cmd "set *($type($size) *)($addr) = $val"} res
    # read it back
    catch {gdb_get_mem $addr $format $size $size $size ""} val
    # delete whitespace in response
    set val [string trimright $val]
    set val [string trimleft $val]
    set _memval${this}($row,$col) $val
  }


  # ------------------------------------------------------------------
  #  METHOD:  toggle_enabled - called when enable is toggled
  # ------------------------------------------------------------------
  method toggle_enabled {} {
    global _mem tixOption

    if {$Running} { return }
    if {$_mem($this,enabled)} {
      update_address
      set bg $tixOption(input1_bg)
    } else {
      set bg $tixOption(bg)
    }
    $this.t config -bg $bg
  }

  # ------------------------------------------------------------------
  #  METHOD:  update - update widget after every PC change
  # ------------------------------------------------------------------
  method update {} {
    global _mem
    #debug "START MEMORY UPDATE CALLBACK"
    if {$_mem($this,enabled)} {
      update_address
    }
    #debug "END MEMORY UPDATE CALLBACK"
  }

  # ------------------------------------------------------------------
  #  METHOD:  idle - memory window is idle, so enable menus
  # ------------------------------------------------------------------
  method idle {} {
    # Fencepost
    set Running 0

    # Cursor
    cursor {}

    # Enable menus
    if {$mbar} {
      for {set i 0} {$i <= [$this.m.addr index last]} {incr i} {
	if {[$this.m.addr type $i] != "separator"} {
	  $this.m.addr entryconfigure $i -state normal
	}
      }
    }
  }

  # ------------------------------------------------------------------
  #  DESTRUCTOR - destroy window containing widget
  # ------------------------------------------------------------------
  destructor {
    if {[winfo exists $prefs_win]} {
      $prefs_win delete
    }
    remove_hook gdb_update_hook "$this update"
    remove_hook gdb_busy_hook [list $this busy]
    remove_hook gdb_idle_hook [list $this idle]

    destroy $this
    destroy $top
  }

  # ------------------------------------------------------------------
  #  METHOD: busy - disable menus 'cause we're busy updating things
  # ------------------------------------------------------------------
  method busy {} {
    # Fencepost
    set Running 1

    # cursor
    cursor watch

    # Disable menus
    if {$mbar} {
      for {set i 0} {$i <= [$this.m.addr index last]} {incr i} {
	if {[$this.m.addr type $i] != "separator"} {
	  $this.m.addr entryconfigure $i -state disabled
	}
      }
    }
  }

  # ------------------------------------------------------------------
  #  METHOD: newsize - calculate how many rows to display when the
  #  window is resized.
  # ------------------------------------------------------------------
  method newsize {height} {
    if {$dont_size || $Running} {
      return 
    }
    
    # make sure row height is set
    if {$rheight == ""} {
      set rheight [lindex [$this.t bbox 0,0] 3]
    }

    # only add rows if numbytes is zero
    if {$numbytes == 0} {
      @@update idletasks
      set theight [winfo height $this.t]
      set Numrows [expr {$theight / $rheight}]
      $this.t configure -rows $Numrows
      update_addr
    }
  }

  protected new_entry 0	;# static var

  # ------------------------------------------------------------------
  #  METHOD: update_address_cb - address entry widget callback
  # ------------------------------------------------------------------
  method update_address_cb {ae} {
    set new_entry 1
    update_address $ae
  }

  # ------------------------------------------------------------------
  #  METHOD: update_address - update address and data displayed
  # ------------------------------------------------------------------
  method update_address { {ae ""} } {
    global tixOption
    if {$ae == ""} {
      set addr_exp [string trimleft [$this.f.cntl cget -value]]
    } else {
      set addr_exp $ae
    }

    if {[string match {[a-zA-Z_&0-9\*]*} $addr_exp]} {
      # Looks like an expression
      catch {gdb_eval "$addr_exp"} current_addr
      if {[string match "No symbol*" $current_addr] || \
	    [string match "Invalid *" $current_addr]} {
	BadExpr $current_addr
	return
      }
      if {[string match {\{*} $current_addr]} {
	set current_addr [lindex $current_addr 1]
	if {$current_addr == ""} {
	  return
	}
      }
    } elseif {[string match {\$*} $addr_exp]} {
      # Looks like a local variable
      catch {gdb_eval "$addr_exp"} current_addr
      if {$current_addr == "No registers.\n"} { 
	# we asked for a register value and debugging hasn't started yet
	return 
      }
      if {$current_addr == "void"} {
	BadExpr "No Local Variable Named \"$addr_ex\""
	return
      }
    } else {
      # something really strange, like "0.1" or ""
      BadExpr "Can't Evaluate \"$addr_expr\""
      return
    }

    # Check for spaces
    set index [string first \  $current_addr]
    if {$index != -1} {
      incr index -1
      set current_addr [string range $current_addr 0 $index]
    }
    
    # set table background
    $this.t config -bg $tixOption(input1_bg)
    catch {update_addr}
  }

  # ------------------------------------------------------------------
  #  METHOD:  BadExpr - handle a bad expression
  # ------------------------------------------------------------------
  method BadExpr {errTxt} {
    global tixOption
    if {$new_entry} {
      tk_messageBox -type ok -icon error -message $errTxt
      set new_entry 0
    }
    # set table background to gray
    $this.t config -bg $tixOption(bg)
  }

  # ------------------------------------------------------------------
  #  METHOD:  incr_addr - callback from control widget to increment
  #  the current address.
  # ------------------------------------------------------------------
  method incr_addr {i foo} {
    incr current_addr [expr {$i * $bytes_per_row}]
    return [format "0x%x" $current_addr]
  }


  # ------------------------------------------------------------------
  #  METHOD:  update_addr - read in data starting at $current_addr
  #  This is just a helper function for update_address.
  # ------------------------------------------------------------------
  method update_addr {} {
    global _mem _memval${this}

    gdbtk_busy
    set addr $current_addr
    set row 0

    if {$numbytes == 0} {
      set nb [expr {$Numrows * $bytes_per_row}]
    } else {
      set nb $numbytes
    }
    set nextval 0
    set num [expr {$bytes_per_row / $size}]
    if {$ascii} {
      set asc $ascii_char
    } else {
      set asc ""
    }
    catch {gdb_get_mem $addr $format $size $nb $bytes_per_row $asc} vals
    set mlen 0
    for {set n 0} {$n < $nb} {incr n $bytes_per_row} {
      set x [format "0x%x" $addr]
      if {[string length $x] > $mlen} {
	set mlen [string length $x]
      }
      set _memval${this}($row,-1) $x
      for { set col 0 } { $col < $num } { incr col } {
	set x [lindex $vals $nextval]
	if {[string length $x] > $maxlen} {set maxlen [string length $x]}
	set _memval${this}($row,$col) $x
	incr nextval
      }
      if {$ascii} {
	set x [lindex $vals $nextval]
	if {[string length $x] > $maxalen} {set maxalen [string length $x]}
	set _memval${this}($row,$col) $x
	incr nextval
      }
      incr addr $bytes_per_row
      incr row
    }
    # set default column width to the max in the data columns
    $this.t configure -colwidth [expr {$maxlen + 1}]
    # set border column width
    $this.t width -1 [expr $mlen + 1]
    if {$ascii} {
      # set ascii column width
      $this.t width $Numcols [expr {$maxalen + 1}]
    }

    gdbtk_idle
  }
  

  # ------------------------------------------------------------------
  #  METHOD:  config - used to change public attributes
  # ------------------------------------------------------------------
  method config {config} {
    set addr_exp [string trimright [string trimleft $addr_exp]]
    set wh [winfo height $top]
    set dont_size 1
    reconfig

    # must update here
    @@update

    set dont_size 0
    if {$numbytes == 0} {
      newsize $wh
    }
  }

  # ------------------------------------------------------------------
  #  METHOD:  hidemb - hide the menubar.  NOT CURRENTLY USED
  # ------------------------------------------------------------------
  method hidemb {} {
    set mbar 0
    reconfig
  }

  # ------------------------------------------------------------------
  #  METHOD:  reconfig - used when preferences change
  # ------------------------------------------------------------------
  method reconfig {} {
    if {[winfo exists $this.m]} { destroy $this.m }
    if {[winfo exists $this.cb]} { destroy $this.cb }
    if {[winfo exists $this.f.upd]} { destroy $this.f.upd }
    if {[winfo exists $this.sy]} { destroy $this.sy }  
    destroy $this.f.cntl $this.f $this.t $this.sx 
    build_win
  }

  # ------------------------------------------------------------------
  #  METHOD:  do_popup - Display popup menu
  # ------------------------------------------------------------------
  method do_popup {X Y} {
    if {$Running} { return }
    $this.t.menu delete 0 end
    $this.t.menu add check -label "Auto Update" -variable _mem($this,enabled) \
	-underline 0 -command "$this toggle_enabled"
    $this.t.menu add command -label "Update Now" -underline 0 \
	-command "$this update_address"
    $this.t.menu add command -label "Go To [$this.t curvalue]" -underline 0 \
      -command "$this goto [$this.t curvalue]"
    $this.t.menu add command -label "Open New Window at [$this.t curvalue]" -underline 0 \
      -command [list manage create mem -addr_exp [$this.t curvalue]]
    $this.t.menu add separator
    $this.t.menu add command -label "Preferences..." -underline 0 \
	-command "$this create_prefs"
    tk_popup $this.t.menu $X $Y 
  }

  # ------------------------------------------------------------------
  #  METHOD:  goto - change the address of the current memory window
  # ------------------------------------------------------------------
  method goto { addr } {
    set current_addr $addr
    $this.f.cntl config -value $addr
  }

  # ------------------------------------------------------------------
  #  METHOD:  init_addr_exp - initialize address expression
  #  On startup, if the public variable "addr_exp" was not set,
  #  then set it to the start of ".data" if found, otherwise "$pc"
  # ------------------------------------------------------------------
  method init_addr_exp {} {
    if {$addr_exp == ""} {
      set err [catch {gdb_cmd "info file"} result]
      if {!$err} {
	foreach line [split [string trim $result] \n] { 
	  if {[scan $line {%x - %x is %s} start stop section] == 3} {
	    if {$section == ".data"} {
	      set addr_exp [format "%#08x" $start]
	      break
	    }
	  }
	}
      }
      if {$addr_exp == ""} {
	set addr_exp \$pc
      }
    }
  }

  # ------------------------------------------------------------------
  #  METHOD:  cursor - set the cursor
  # ------------------------------------------------------------------
  method cursor {glyph} {
    # Set cursor for all labels
    # for {set i 0} {$i < $bytes_per_row} {incr i $size} {
    #   $this.t.h.$i configure -cursor $glyph
    # }
    $top configure -cursor $glyph
  }

  # memMoveCell --
  #
  # Moves the location cursor (active element) by the specified number
  # of cells and changes the selection if we're in browse or extended
  # selection mode.
  #
  # Don't allow movement into the ASCII column.
  #
  # Arguments:
  # w - The table widget.
  # x - +1 to move down one cell, -1 to move up one cell.
  # y - +1 to move right one cell, -1 to move left one cell.
  
  method memMoveCell {w x y} {
    if {[catch {$w index active row} r]} return
    set c [$w index active col]
    if {$ascii && ($c == $Numcols)} {
      # we're in the ASCII column so behave differently
      if {$y == 1} {set x 1}
      if {$y == -1} {set x -1}
      incr r $x
    } else {
      incr r $x
      incr c $y
      if { $c < 0 } {
	if {$r == 0} {
	  set c 0
	} else {
	  set c [expr {$Numcols - 1}]
	  incr r -1
	}
      } elseif { $c >= $Numcols } {
	if {$r >= [expr {$Numrows - 1}]} {
	  set c [expr {$Numcols - 1}]
	} else {
	  set c 0
	  incr r
	}
      }
    }
    if { $r < 0 } { set r 0 }
    $w activate $r,$c
    $w see active
  }
  
  #
  #  PROTECTED DATA
  #
  protected current_addr ""
  protected dont_size 0
  protected mbar 1
  protected bg
  protected top
  protected nb 128
  protected prefs_win ""
  protected Running 0
  protected Numrows 0
  protected Numcols 0
  protected saved_value
  protected maxlen
  protected maxalen
  protected rheight ""

  # PUBLIC PARAMETERS
  public addr_exp ""
  public size 4
  public format x
  public bytes_per_row 16
  public numbytes 128
  public ascii 1
  public ascii_char "."
  public color green

  common type
}

