#
# MemPref
# ----------------------------------------------------------------------
# Implements GDB memory dump preferences dialog
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
#   AUTHOR:  Martin M. Hunt <hunt@cygnus.com>
#   Copyright (C) 1997, 1998 Cygnus Solutions
#

itcl_class MemPref {
  # ------------------------------------------------------------------
  #  CONSTRUCTOR - create new memory preferences dialog
  # ------------------------------------------------------------------
  constructor {config} {
    #
    #  Create a window with the same name as this object
    #
    global gsize gformat gnumbytes gbpr gascii gascii_char gvar
    
    set class [$this info class]
    @@rename $this $this-tmp-
    @@frame $this -class $class 
    @@rename $this $this-win-
    @@rename $this-tmp- $this

    set gsize $size
    set gformat $format
    set gnumbytes $numbytes
    set gbpr $bpr
    set gascii $ascii
    set gascii_char $ascii_char

    set float_size [gdb_eval sizeof(float)]
    set double_size [gdb_eval sizeof(double)]

    build_win
    wm resizable [winfo toplevel $this] 0 0
  }

  # ------------------------------------------------------------------
  #  METHOD:  build_win - build the dialog
  # ------------------------------------------------------------------
  method build_win {} {
    global gsize gformat gnumbytes gbpr gascii gascii_char gvar

    if {$attach != 1} {
      frame $this.f
      frame $this.f.a
      frame $this.f.b
      set f $this.f.a
    } else {
      set f $this
    }

    # SIZE
    Labelledframe $f.f1 -anchor nw -text Size
    set fr [$f.f1 get_frame]

    set Widgets(rb-Byte) [radiobutton $fr.1 -variable gsize -text Byte -value 1]
    set Widgets(rb-half_word) [radiobutton $fr.2 -variable gsize -text "Half Word" -value 2]
    set Widgets(rb-word) [radiobutton $fr.4 -variable gsize -text Word -value 4]
    set Widgets(rb-d_word) [radiobutton $fr.8 -variable gsize -text "Double Word" -value 8]
    set Widgets(rb-float) [radiobutton $fr.f -variable gsize -text Float -value 3]
    set Widgets(rb-d_float) [radiobutton $fr.d -variable gsize -text "Double Float" -value 5]
    grid $fr.1 $fr.4 $fr.f -sticky w -padx 4
    grid $fr.2 $fr.8 $fr.d -sticky w -padx 4

    # FORMAT
    Labelledframe $f.f2 -anchor nw -text Format
    set fr [$f.f2 get_frame]
    set Widgets(rb-binary) [radiobutton $fr.1 -variable gformat -text Binary -value t]
    set Widgets(rb-octal) [radiobutton $fr.2 -variable gformat -text Octal -value o]
    set Widgets(rb-hex) [radiobutton $fr.3 -variable gformat -text Hex -value x]
    set Widgets(rb-signed_dec) [radiobutton $fr.4 -variable gformat -text "Signed Decimal" -value d]
    set Widgets(rb-unsign_dec) [radiobutton $fr.5 -variable gformat -text "Unsigned Decimal" -value u]

    grid $fr.1 $fr.2 $fr.3 -sticky w -padx 4
    grid $fr.4 $fr.5 x -sticky w -padx 4

    # TOTAL BYTES
    Labelledframe $f.fx -anchor nw -text "Number of Bytes"

   if {$gnumbytes == 0} {
      set gnumbytes $default_numbytes
      set gvar 0
    } else {
      set gvar 1
    }

    set fr [$f.fx get_frame] 
    set Widgets(rb-win_size) [radiobutton $fr.1 -variable gvar -text "Depends on window size" \
				-value 0 -command [list $this toggle_size_control]]
    frame $fr.2
    set Widgets(rb-fixed) [radiobutton $fr.2.b -variable gvar -text Fixed \
			     -value 1 -command [list $this toggle_size_control]]

    set old_numbytes $default_numbytes
    set Widgets(e-numbytes) [entry $fr.2.e -textvariable gnumbytes -width 3]
    set normal_background [$Widgets(e-numbytes) cget -background]

    #
    # Trace gnumbytes so it will always be a +'ve integer...  Have to set this
    # trace AFTER the widget's textvariable is set so this trace will fire
    # BEFORE the widget's trace.  If you change this trace, remember to change
    # the trace deletion in the destructor to match.
    #

    trace variable gnumbytes w [list $this check_numbytes]

    label $fr.2.l -text bytes
    grid $fr.2.b $fr.2.e $fr.2.l -sticky we
    grid $fr.1 x -sticky w -padx 4
    grid $fr.2 x -sticky w -padx 4
    grid columnconfigure $fr 1 -weight 1

    # MISC
    Labelledframe $f.1 -anchor nw -text "Miscellaneous"
    set fr [$f.1 get_frame] 
    frame $fr.1
    label $fr.1.plabel -height 1 -width 1 -bg $color -relief raised  
    set Widgets(b-color) [button $fr.1.pc -text "Change color..."  \
		       -command "$this pick $fr.1.plabel"]
    grid $fr.1.plabel $fr.1.pc
    set Widgets(b-bytes_per_row) [tixComboBox $fr.2 -label "Bytes Per Row " -variable gbpr -options \
		       {entry.width 4 listbox.width 2 listbox.height 4 slistbox.scrollbar none }]
    $fr.2 insert end 4
    $fr.2 insert end 8
    $fr.2 insert end 16
    $fr.2 insert end 32
    $fr.2 insert end 64
    $fr.2 insert end 128
    set Widgets(cb-display_ascii) [checkbutton $fr.3 -variable gascii -text "Display ASCII"]
    frame $fr.4
    set Widgets(e-ascii_char) [entry $fr.4.e -textvariable gascii_char -width 1]
    label $fr.4.l -text "Control Char"
    grid $fr.4.e $fr.4.l -sticky we
    grid $fr.2 x $fr.3 -sticky w -padx 4
    grid $fr.4 -sticky w -padx 4
    grid columnconfigure $fr 1 -weight 1

    grid $f.f1 -padx 5 -pady 6 -sticky news
    grid $f.f2 -padx 5 -pady 6 -sticky news
    grid $f.fx -padx 5 -pady 6 -sticky news
    grid $f.1 -padx 5 -pady 6 -sticky we

    if {$attach != 1} {
      set Widgets(b-ok) [button $this.f.b.ok -text OK -command "$this ok" -width 7 -default active]
      focus $Widgets(b-ok)
      
      # If there is an OK button, set Return in the entry field to invoke it...

      bind $Widgets(e-numbytes) <KeyPress-Return> "$Widgets(b-ok) flash ; $Widgets(b-ok) invoke"

      set Widgets(b-cancel) [button $this.f.b.quit -text Cancel -command "$this delete" -width 7]
      set Widgets(b-apply) [button $this.f.b.apply -text Apply -command "$this apply" -width 7]
      standard_button_box $this.f.b

      grid $this.f.a 
      grid $this.f.b -sticky news
      grid $this.f
    }

    #
    # Set the state of the window size entry here...
    #
    toggle_size_control

  }

  # ------------------------------------------------------------------
  #  DESTRUCTOR - destroy window containing widget
  # ------------------------------------------------------------------
  destructor {
    global gnumbytes

    set top [winfo toplevel $this]
    manage delete $this 1
    destroy $this
    trace vdelete gnumbytes w [list $this check_numbytes]
    if {$attach != 1} {
      destroy $top
    }
  }

  # ------------------------------------------------------------------
  #  METHOD:  config - used to change public attributes
  # ------------------------------------------------------------------
  method config {config} {}

  # ------------------------------------------------------------------
  #  METHOD:  busy - make the widget unusable
  # ------------------------------------------------------------------
  method busy {} {
    set top [winfo toplevel $this]
    $top configure -cursor watch
    
    # Disable all the radiobuttons and what not
    foreach w [array names Widgets] {
      $Widgets($w) configure -state disabled
    }
  }

  # ------------------------------------------------------------------
  #  METHOD:  idle - make the widget useable
  # ------------------------------------------------------------------
  method idle {} {
    set top [winfo toplevel $this]
    $top configure -cursor {}

    # Re-enable all widgets
    foreach w [array names Widgets] {
      $Widgets($w) configure -state normal
    }
  }
  # ------------------------------------------------------------------
  #  METHOD:  ok - apply and quit
  # ------------------------------------------------------------------
  method ok {} {
    apply
    delete
  }

  # ------------------------------------------------------------------
  #  METHOD:  check_numbytes - a trace to make sure gnumbytes is an int > 0
  # ------------------------------------------------------------------
  method check_numbytes {var index mode} {
   upvar \#0 $var true
    if {($true != "") && ([catch {expr {(int($true) != double($true)) || $true <= 0}} val] 
			|| $val)} {
      bell
      set true $old_numbytes
    } else {
      set old_numbytes $true
    }
  }

  # ------------------------------------------------------------------
  #  METHOD:  toggle_size_control - toggle the state of the entry box as the
  #           control method changes
  # ------------------------------------------------------------------
  method toggle_size_control {} {
    global gvar

    if {$gvar} {
      $Widgets(e-numbytes) configure -state normal \
	-background $normal_background
    } else {
      $Widgets(e-numbytes) configure -state disabled -background lightgray
      if {[info exists Widgets(b-ok)]} {
	focus $Widgets(b-ok)
      }
    }
  }

  # ------------------------------------------------------------------
  #  METHOD:  apply - apply changes to the parent window
  # ------------------------------------------------------------------
  method apply {} {
    global gsize gformat gnumbytes gbpr gascii gascii_char gvar

    busy
    gdbtk_busy

    if {$gvar == 0} {
      set numbytes 0
    } elseif {$gnumbytes == "" || $gnumbytes == 0} {
      # Protect against the case where someone sets the
      # entry field to an empty string, or pastes in a 0...
      bell
      set gnumbytes $default_numbytes
      set numbytes $gnumbytes
    } else {
      set numbytes $gnumbytes
    }

    set format $gformat

    switch $gsize {
      3 {
	set size $float_size
	set format f
      }
      5 {
	set size $double_size
	set format f
      }
      default {
	set size $gsize
      }
    }
    debug "Set sizes"

    # pass all the changed values back to parent
    $win config -size $size -numbytes $numbytes -format $format -ascii $gascii \
      -ascii_char $gascii_char -bytes_per_row $gbpr -color $color

    gdbtk_idle
    idle
  }

  # ------------------------------------------------------------------
  #  METHOD:  pick - pick colors
  # ------------------------------------------------------------------
  method pick {lab} {
    set new_color [tk_chooseColor -initialcolor $color -title "Choose color"]
    if {$new_color != $color && $new_color != ""} {
      set color $new_color
      $lab configure -bg $color
    }
  }


  # ------------------------------------------------------------------
  #  METHOD:  reconfig - used when preferences change
  # ------------------------------------------------------------------
  method reconfig {} {
    # for now, just delete and recreate
    if {$attach == 1} {
      destroy $this.1 $this.2 $this.3
    } else {
      destroy $this.f 
    }
    build_win
  }

  #
  #  PROTECTED DATA
  #
  public attach 0
  public win
  public size 
  public format 
  public numbytes 
  public bpr 
  public ascii 
  public ascii_char
  public color

  protected float_size
  protected double_size
  protected Widgets
  protected old_numbytes
  protected normal_background

  common default_numbytes 128
  common KeysToPass

  array set KeysToPass {
    Return 1
    Enter 1
    Delete 1
    BackSpace 1
  }
}
