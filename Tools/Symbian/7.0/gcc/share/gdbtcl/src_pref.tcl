#
# SrcPref
# ----------------------------------------------------------------------
# Implements GDB source preferences dialog
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
#   AUTHOR:  Martin M. Hunt	hunt@cygnus.com     
#            Cygnus Solutions   
#


itcl_class SrcPref {
  # ------------------------------------------------------------------
  #  CONSTRUCTOR - create new source preferences dialog
  # ------------------------------------------------------------------
  constructor {args} {
    #
    #  Create a window with the same name as this object
    #
    global PREFS_state
    set attach $args
    set class [$this info class]
    @@rename $this $this-tmp-
    @@frame $this -class $class 
    @@rename $this $this-win-
    @@rename $this-tmp- $this
    build_win
    set s_pc_color     [pref get gdb/src/PC_TAG]
    set s_stack_color  [pref get gdb/src/STACK_TAG]
    set s_browse_color [pref get gdb/src/BROWSE_TAG]
    set s_run(attach)  [pref get gdb/src/run_attach]
    set s_run(load)    [pref get gdb/src/run_load]
    set s_run(run)     [pref get gdb/src/run_run]
    set s_run(cont)    [pref get gdb/src/run_cont]
    set s_bp(norm)     [pref get gdb/src/bp_fg]
    set s_bp(temp)     [pref get gdb/src/temp_bp_fg]
    set s_bp(tp)       [pref get gdb/src/trace_fg]
    set s_bp(dis)      [pref get gdb/src/disabled_fg]
    set s_vars         [pref get gdb/src/variableBalloons]
    set s_source2      [pref get gdb/src/source2_fg]
    set s_tab          [pref get gdb/src/tab_size]
    set s_mode         [pref get gdb/mode]
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

    # Colors frame
    Labelledframe $f.colors -anchor nw -text {Colors}
    set w [$f.colors get_frame]

    set color [pref get gdb/src/PC_TAG]
    label $w.pcl -text {PC}
    button $w.pcb -text {     } -activebackground $color -bg $color \
      -command [list $this pick $color $w.pcb PC_TAG]
    
    set color [pref get gdb/src/STACK_TAG]
    label $w.stl -text {Stack}
    button $w.stb -text {     } -activebackground $color -bg $color \
      -command [list $this pick $color $w.stb STACK_TAG]

    set color [pref get gdb/src/BROWSE_TAG]
    label $w.brl -text {Browse}
    button $w.brb -text {     } -activebackground $color -bg $color\
      -command [list $this pick $color $w.brb BROWSE_TAG]

    set color [pref get gdb/src/source2_fg]
    label $w.s2l -text {Mixed Source}
    button $w.s2b -text {     } -activebackground $color -bg $color \
      -command [list $this pick $color $w.s2b source2_fg]

    set color [pref get gdb/src/bp_fg]
    label $w.nbpl -text {Normal Breakpoint}
    button $w.nbpb -text {     } -activebackground $color -bg $color\
      -command [list $this pick $color $w.nbpb bp_fg]
    
    set color [pref get gdb/src/temp_bp_fg]
    label $w.tbpl -text {Temporary Breakpoint}
    button $w.tbpb -text {     } -activebackground $color -bg $color \
      -command [list $this pick $color $w.tbpb temp_bp_fg]
    
    set color [pref get gdb/src/disabled_fg]
    label $w.dbpl -text {Disabled Breakpoint}
    button $w.dbpb -text {     } -activebackground $color -bg $color \
      -command [list $this pick $color $w.dbpb disabled_fg]

    set color [pref get gdb/src/trace_fg]
    label $w.tpl -text {Tracepoint}
    button $w.tpb -text {     } -activebackground $color -bg $color \
      -command [list $this pick $color $w.tpb trace_fg]

    grid $w.pcl $w.pcb $w.nbpl $w.nbpb -padx 10 -pady 2 -sticky w
    grid $w.stl $w.stb $w.tbpl $w.tbpb -padx 10 -pady 2 -sticky w
    grid $w.brl $w.brb $w.dbpl $w.dbpb -padx 10 -pady 2 -sticky w
    grid $w.s2l $w.s2b $w.tpl  $w.tpb  -padx 10 -pady 2 -sticky w

    frame $f.rmv

    # Debug Mode frame
    Labelledframe $f.rmv.mode -anchor nw -text "Debug Mode"
    set w [$f.rmv.mode get_frame]
    set var [pref varname gdb/mode]
    radiobutton $w.async -text "Tracepoints" -variable $var -value 1 -state disabled
    radiobutton $w.sync  -text "Breakpoints" -variable $var  -value 0 -state disabled
    pack $w.async $w.sync -side top

    # Variable Balloons
    Labelledframe $f.rmv.var -anchor nw -text "Variable Balloons"
    set w [$f.rmv.var get_frame]
    set var [pref varname gdb/src/variableBalloons]
    radiobutton $w.var_on -text "On " -variable $var -value 1
    radiobutton $w.var_off -text "Off" -variable $var -value 0
    pack $w.var_on $w.var_off -side top
    pack $f.rmv.mode $f.rmv.var -side left -fill both -expand yes \
      -padx 5 -pady 5


    frame $f.x
    # Tab size
    tixControl $f.x.size -label "Tab Size" -integer true -max 16 -min 1 \
      -variable [pref varname gdb/src/tab_size] \
      -options { entry.width 2	entry.font src-font}

    # Linenumbers
    checkbutton $f.x.linenum -text "Line Numbers" \
      -variable [pref varname gdb/src/linenums]
    pack $f.x.size $f.x.linenum -side left -padx 5 -pady 5

    pack $f.colors $f.rmv $f.x -side top -fill both -expand yes

    if {$attach != 1} {
      button $this.f.b.ok -text OK -width 7 -underline 0 -command "$this save"
      button $this.f.b.apply -text Apply -width 7 -underline 0 -command "$this apply"
      button $this.f.b.quit -text Cancel -width 7 -underline 0 -command "$this cancel"
      standard_button_box $this.f.b
      pack $this.f.a $this.f.b $this.f -expand yes -fill both -padx 5 -pady 5
      wm resizable [winfo toplevel $this] 0 0
    }
  }

  # ------------------------------------------------------------------
  #  METHOD:  apply - apply changes
  # ------------------------------------------------------------------
  method apply {} {
    manage restart
  }

  # ------------------------------------------------------------------
  #  METHOD:  cancel
  # ------------------------------------------------------------------
  method cancel {} {
    pref set gdb/src/PC_TAG $s_pc_color
    pref set gdb/src/STACK_TAG $s_stack_color
    pref set gdb/src/BROWSE_TAG $s_browse_color
    pref set gdb/src/run_attach $s_run(attach)
    pref set gdb/src/run_load $s_run(load)
    pref set gdb/src/run_run $s_run(run)
    pref set gdb/src/run_cont $s_run(cont)
    pref set gdb/src/bp_fg $s_bp(norm)
    pref set gdb/src/temp_bp_fg $s_bp(temp)
    pref set gdb/src/disabled_fg $s_bp(dis)
    pref set gdb/src/trace_fg $s_bp(tp)
    pref set gdb/src/variableBalloons $s_vars
    pref set gdb/src/source2_fg $s_source2
    pref set gdb/src/tab_size $s_tab
    pref set gdb/mode $s_mode
    save
  }
  
  # ------------------------------------------------------------------
  #  METHOD:  save - apply changes and quit
  # ------------------------------------------------------------------
  method save {} {
    manage delete $this
    manage restart
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
  #  METHOD:  pick - pick colors
  # ------------------------------------------------------------------
  method pick {color win tag} {
    set new_color [tk_chooseColor -initialcolor $color -title "Choose color"]
    if {$new_color != $color && $new_color != {}} {
      pref set gdb/src/$tag $new_color
      $win configure -activebackground $new_color -bg $new_color
    }
  }


  # ------------------------------------------------------------------
  #  METHOD:  reconfig - used when preferences change
  # ------------------------------------------------------------------
  method reconfig {} {
    # for now, just delete and recreate
    #if {$attach == 1} {
    #  destroy $this.1 $this.2 $this.3
    #} else {
    #  destroy $this.f 
    #}
    #build_win
  }

  # ------------------------------------------------------------------
  #  METHOD:  set_run - set the run button. Make sure not both run and
  #                     continue are selected.
  # ------------------------------------------------------------------
  method set_run {check_which} {
    global PREFS_state
    set var [pref varname gdb/src/run_$check_which]
    global $var
    if {[set $var]} {
      set $var 0
    }
  }
	
  #
  #  PROTECTED DATA
  #
  protected f
  protected attach 0
  protected s_pc_color
  protected s_stack_color
  protected s_browse_color
  protected s_run
  protected s_mode
  protected s_bp
  protected s_vars
  protected s_source2
  protected s_tab
}
