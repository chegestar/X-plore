#
# GDBToolBar
# ----------------------------------------------------------------------
# Implements a menu, toolbar, and status window for GDB
#
#   PUBLIC ATTRIBUTES:
#
#
#   METHODS:
#
#     config ....... used to change public attributes
#     insert ....... inserts text in the text widget
#     einsert ...... inserts error text in the text widget
#     command ...... run a command and print output
#
#     PRIVATE METHODS
#
#     previous ..... recalls the previous command
#     next ......... recalls the next command
#     last ......... recalls the last command in the history list
#     first ........ recalls the first command in the history list
#
#   X11 OPTION DATABASE ATTRIBUTES
#
#
# ----------------------------------------------------------------------
#   AUTHOR:  Martin M. Hunt <hunt@cygnus.com>
#   Copyright (C) 1997, 1998 Cygnus Solutions
#


itcl_class GDBToolBar {
  # ------------------------------------------------------------------
  #  CONSTRUCTOR - create new console window
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

    add_hook gdb_idle_hook "$this enable_ui 1"
    add_hook gdb_busy_hook "$this enable_ui 0"
    add_hook gdb_no_inferior_hook "$this enable_ui 2"
  }

  # ------------------------------------------------------------------
  #  METHOD:  build_win - build the main toolbar window
  # ------------------------------------------------------------------
  method build_win {} {

    set OtherMenus {}
    set ControlMenus {}
    set OtherButtons {}
    set ContorlButtons {}

    set Menu [menu $this.m -tearoff 0]
    if {! [virtual create_menu_items $this.m]} {
      destroy $Menu
      set Menu {}
    } else {
      [winfo toplevel $this] configure -menu $Menu
    }

    # Make a subframe so that the menu can't accidentally conflict
    # with a name created by some subclass.
    frame $this.t
    set button_list [virtual create_buttons $this.t]
    if {! [llength $button_list]} {
      destroy $this.t
    } else {
      eval standard_toolbar $this.t $button_list
      pack $this.t $this -fill both -expand true
    }
  }

  # ------------------------------------------------------------------
  #  DESTRUCTOR - destroy window containing widget
  # ------------------------------------------------------------------
  destructor {
    remove_hook gdb_idle_hook "$this enable_ui 1"
    remove_hook gdb_busy_hook "$this enable_ui 0"
    remove_hook gdb_no_inferior_hook "$this enable_ui 2"
    destroy $this
  }

  # ------------------------------------------------------------------
  #  METHOD:  configure - used to change public attributes
  # ------------------------------------------------------------------
  method configure {config} {}

  # ------------------------------------------------------------------
  #  METHOD:  reconfig - used when preferences change
  # ------------------------------------------------------------------
  method reconfig {} {
    _load_images 1
  }

  # ------------------------------------------------------------------
  #  METHOD:  create_buttons - Add some buttons to the toolbar.  Returns
  #                         list of buttons in form acceptable to
  #                         standard_toolbar.
  # ------------------------------------------------------------------
  method create_buttons {frame} {
    global IDE_ENABLED

    _load_images

    set button_list {}

    button $frame.reg -command {manage open reg} -image reg_img
    balloon register $frame.reg "Registers (Ctrl+R)"
    lappend button_list $frame.reg
    lappend OtherButtons $frame.reg

    button $frame.mem -command {manage open mem} -image memory_img
    balloon register $frame.mem "Memory (Ctrl+M)"
    lappend button_list $frame.mem
    lappend OtherButtons $frame.mem

    button $frame.stack -command {manage open stack} -image stack_img
    balloon register $frame.stack "Stack (Ctrl+S)"
    lappend button_list $frame.stack
    lappend OtherButtons $frame.stack

    button $frame.watch -command {manage open watch} -image watch_img
    balloon register $frame.watch "Watch Expressions (Ctrl+W)"
    lappend button_list $frame.watch
    lappend OtherButtons $frame.watch

    button $frame.vars -command {manage open locals} -image vars_img
    balloon register $frame.vars "Local Variables (Ctrl+L)"
    lappend button_list $frame.vars
    lappend OtherButtons $frame.vars

    if { ![pref get gdb/mode] } {
    button $frame.bp -command {manage open bp} -image bp_img
    balloon register $frame.bp "Breakpoints (Ctrl+B)"
    lappend button_list $frame.bp
    lappend OtherButtons $frame.bp
    } else {
      button $frame.tp -command {manage open tp -tracepoints 1} -image bp_img
      balloon register $frame.tp "Tracepoints (Ctrl+T)"
      lappend button_list $frame.tp
      lappend OtherButtons $frame.tp
 
      button $frame.tdump -command  {manage open tdump} -text "Td"
      balloon register $frame.tdump "Tdump (Ctrl+D)"
      lappend button_list $frame.tdump
      lappend OtherButtons $frame.tdump
    }

    button $frame.con -command {manage open console} -image console_img
    balloon register $frame.con "Console (Ctrl+N)"
    lappend button_list $frame.con
    lappend OtherButtons $frame.con

    lappend button_list -

    if {$IDE_ENABLED} {
      button $frame.vmake -image vmake_img \
	-command {idewindow_activate_by_name "Foundry _Project"}
      balloon register $frame.vmake "Foundry Project"
      lappend button_list $frame.vmake
      lappend OtherButtons $frame.vmake
    }
    
    # Random bits of obscurity...
    bind $frame.reg <Button-3> "manage create reg"
    bind $frame.mem <Button-3> "manage create mem"
    bind $frame.watch <Button-3> "manage create watch"
    bind $frame.vars <Button-3> "manage create locals"
    # multiple stack and breakpoint windows don't work and
    # aren't very useful anyway
    #bind $frame.stack <Button-3> "manage create stack"
    #if [winfo exists $frame.bp] {
    #  bind $frame.bp <Button-3> "manage create bp"
    #}
    #if [winfo exists $frame.tp] {
    #  bind $frame.tp <Button-3> "manage create tp -tracepoints 1"
    #  bind $frame.tdump <Button-3> "manage create tdump"
    #}

    return $button_list
  }

  # ------------------------------------------------------------------
  #  METHOD:  create_menu_items - Add some menu items to the menubar.
  #                               Returns 1 if any items added.
  # num = number of last menu entry
  # ------------------------------------------------------------------
  method create_menu_items {menu {num 0}} {
    global IDE_ENABLED Paths

    $menu add cascade -menu $menu.file -label "File" -underline 0
    lappend OtherMenus $num
    set m [menu $menu.file]
    $m add separator
    if {$IDE_ENABLED} {
      $m add command -label "Debugger Preferences" -underline 0 \
	      -command {idewindow_activate_by_name [gettext "@@@_Debugger..."]}
      $m add separator
      $m add command -label "Close Debugger" -underline 0 \
	      -command gdbtk_quit      
    } else {
      $m add command -label "Target Settings..." -underline 0 -command {set_target_name ""}
      $m add separator
      $m add command -label "Exit" -underline 1 -command gdbtk_quit
    }

    $menu add cascade -menu $menu.run -label "Run" -underline 0
    incr num
    lappend OtherMenus $num
    set m [menu $menu.run]

    if { [pref get gdb/mode] } {
      $m add command -label "Connect to target" -underline 0 -accelerator "Alt+C" \
         -command "$this do_async_connect $m"
      $m add separator 
      $m add command -label "Begin collection" -underline 0 -accelerator "Ctrl+B" \
         -command "do_tstart"
      $m add command -label "End collection" -underline 0  -accelerator "Ctrl+E" \
         -command "do_tstop" -state disabled
      $m add separator
      $m add command -label "Disconnect" -underline 0 -accelerator "Alt+D" \
         -command "$this do_async_disconnect $m" -state disabled
    } else {
    $m add command -label Download -underline 0 -accelerator "Ctrl+D" -command download_it
    $m add command -label Run -underline 0 -command run_executable -accelerator R
    }


    $menu add cascade -menu $menu.view -label "View" -underline 0
    incr num
    lappend OtherMenus $num
    set m [menu $menu.view]
    $m add command -label "Stack" -underline 0 -accelerator "Ctrl+S" \
      -command {manage open stack}
    $m add command -label "Registers" -underline 0 -accelerator "Ctrl+R" \
      -command {manage open reg}
    $m add command -label "Memory" -underline 0 -accelerator "Ctrl+M" \
      -command {manage open mem}
    $m add command -label "Watch Expressions" -underline 0 \
      -accelerator "Ctrl+W" -command {manage open watch}
    $m add command -label "Local Variables" -underline 0 \
      -accelerator "Ctrl+L" -command {manage open locals}
    if { ![pref get gdb/mode]} {
    $m add command -label "Breakpoints" -underline 0 -accelerator "Ctrl+B" \
      -command {manage open bp -tracepoints 0}
    } else {
      $m add command -label "Tracepoints" -underline 0 -accelerator "Ctrl+T" \
        -command {manage open tp -tracepoints 1}
      $m add command -label "Tdump" -underline 2 -accelerator "Ctrl+U" \
        -command {manage open tdump}
    }
    $m add command -label "Console" -underline 2 -accelerator "Ctrl+N" \
      -command {manage open console}
    $m add command -label "Function Browser" -underline 1 -accelerator "Ctrl+F" \
      -command {manage open browser}
    $m add command -label "Thread List" -underline 0 -accelerator "Ctrl+H" \
      -command {manage open process}

    if { ![pref get gdb/mode]} {
      $menu add cascade -menu $menu.cntrl -label "Control" -underline 0
      incr num
      lappend ControlMenus $num
      set m [menu $menu.cntrl]
      $m add command -label "Step" -underline 0 -accelerator S \
        -command [list catch {gdb_immediate step}]
      $m add command -label "Next" -underline 0 -accelerator N \
        -command [list catch {gdb_immediate next}]
      $m add command -label "Finish" -underline 0 -accelerator F \
        -command [list catch {gdb_immediate finish}]
      $m add command -label "Continue" -underline 0 -accelerator C \
        -command [list catch {gdb_immediate continue}]
      $m add separator
      $m add command -label "Step Asm Inst" -underline 1 -accelerator S \
        -command [list catch {gdb_immediate stepi}]
      $m add command -label "Next Asm Inst" -underline 1 -accelerator N \
        -command [list catch {gdb_immediate nexti}]
      #$m add separator
      #$m add command -label "Automatic Step" -command auto_step
    } else {
      $menu add cascade -menu $menu.trace -label "Trace" -underline 0
      set m [menu $menu.trace]
      $m add command -label "Next Hit" -underline 0 -accelerator N \
	      -command {tfind_cmd tfind}
      $m add command -label "Previous Hit" -underline 0 -accelerator P \
        -command {tfind_cmd "tfind -"}
      $m add command -label "First Hit" -underline 0 -accelerator F \
        -command {tfind_cmd "tfind start"}
      $m add command -label "Next Line Hit" -underline 5 -accelerator L \
        -command {tfind_cmd "tfind line"}
      $m add command -label "Next Hit Here" -underline 9 -accelerator H \
        -command {tfind_cmd "tfind tracepoint"}
      $m add separator
      $m add command -label "Tfind Line..." -underline 9 -accelerator E \
        -command "manage create tfindlinearg -Type LN"
      $m add command -label "Tfind PC..." -underline 7 -accelerator C \
        -command "manage create tfindpcarg -Type PC"
      $m add command -label "Tfind Tracepoint..." -underline 6 -accelerator T \
        -command "manage create tfindtparg -Type TP"
    }

    $menu add cascade -menu $menu.pref -label "Preferences" -underline 0
    incr num
    lappend OtherMenus $num

    if {$IDE_ENABLED} {
      $menu add cascade -menu $menu.win -label "Window" -underline 0
      incr num
      lappend OtherMenus $num
      idewindmenu $menu.win $_ide_title
    } else {
      set m [menu $menu.pref]
      $m add command -label Global -underline 0 -command {manage open globalpref}
#      $m add command -label Download -underline 0 -command {manage open loadpref}
      $m add command -label Source -underline 0 -command {manage open srcpref}
#      $m add command -label Register -underline 0 -command {manage open regpref}
    }


    $menu add cascade -menu $menu.help -label "Help" -underline 0
    incr num
    lappend OtherMenus $num
    set m [menu $menu.help]
    if {$IDE_ENABLED} {
      $m add command -label "Help Topics" -underline 0 -command {ide_help toc}
      $m add command -label "Cygnus Foundry Tour" -underline 15 \
	-command {ide_help display_file [file join $Paths(prefix) help tour.hlp] 1}
      $m add separator
      $m add command -label "Submit a Problem Report" -underline 0 \
	-command {open_url http://www.cygnus.com/product/sendpr.html}
      $m add command -label "Cygnus on the Web" -underline 14 \
	-command {open_url http://www.cygnus.com/product/foundry}
    } else {
      $m add command -label "Help Topics" -underline 0 \
	-command {manage create help}
      $m add command -label "Cygnus on the Web" -underline 14 \
	-command {open_url http://www.cygnus.com/product/gnupro.html}
    }
    $m add separator

    if {$IDE_ENABLED} {
      set label "About Cygnus Foundry..."
    } else {
      set label "About GDB..."
    }
    $m add command -label $label -underline 0 -command {manage open about}
    return 1
  }


  method cmd {args} {
    return [eval $args]
  }

  # ------------------------------------------------------------------
  #  METHOD:  _load_images - Load standard images.  Private method.
  # ------------------------------------------------------------------
  method _load_images { {reconfig 0} } {
    global gdb_ImageDir
    if {!$reconfig && $_loaded_images} {
      return
    }
    set _loaded_images 1

    foreach name {console reg stack vmake vars watch memory bp} {
      image create photo ${name}_img -file [file join $gdb_ImageDir ${name}.gif]
    }
  }

  # ------------------------------------------------------------------
  # METHOD:  enable_ui - enable/disable the appropriate buttons and menus
  # Called from the busy, idle, and no_inferior hooks.
  # ------------------------------------------------------------------
  method enable_ui {on} {
    global tcl_platform
    #debug "Toolbar::enable_ui $on"

    switch $on {
      0 { 
	set ostate disabled
	set cstate disabled
      }
      1 {
	set ostate normal
	set cstate normal
	# set the states of stepi and nexti correctly
	virtual _set_stepi
      }
      2 {
	set ostate normal
	set cstate disabled
      }
    }

    # Disable items
    foreach button $ControlButtons {
      $button configure -state $cstate
    }

    change_menu_state $ControlMenus $cstate

    # Enable items
    foreach button $OtherButtons {
      $button configure -state $ostate
    }

    change_menu_state $OtherMenus $ostate

  }

  # ------------------------------------------------------------------
  # METHOD:  change_menu_state - Does the actual job of enabling menus...
  #          Pass normal or disabled for the state.
  # ------------------------------------------------------------------
  method change_menu_state {menuList state} {
    global tcl_platform

    # Under Unix, it is okay to disable the menus -- they are not selectable.
    # Under Windows, though, it is customary to disable all the menu's entries,
    # not the menu itself.

    foreach menu $menuList {
      if {$tcl_platform(platform) == "windows"} {
	set m [$Menu entrycget $menu -menu]
	set numEntries [$m index last]
	if {[string compare $numEntries "none"] != 0} {
	  for {set i 0} {$i <= $numEntries} {incr i} {
	    if {[$m type $i] != "separator"} {
	      $m entryconfigure $i -state $state
	    }
	  }
        }
      } else {
	$Menu entryconfigure $menu -state $state
      }
    }	
  }

  # ------------------------------------------------------------------
  # METHOD:  do_async_connect: connect to a remote target in asynch mode.
  #                      
  # ------------------------------------------------------------------
  method do_async_connect {menu} {
   debug "do async connect $menu"
    if {[async_connect]} {
     $menu entryconfigure "Connect to target" -state disabled
     $menu entryconfigure "Disconnect" -state normal
   }
  }


  # ------------------------------------------------------------------
  # METHOD:  do_async_disconnect: disconnect from a remote target 
  #                                in asynch mode.                      
  # ------------------------------------------------------------------
  method do_async_disconnect {menu} {
   debug "do async disconnect $menu"
   async_disconnect
   $menu entryconfigure "Connect to target" -state normal
   $menu entryconfigure "Disconnect" -state disabled
  }


  #
  #  PROTECTED DATA
  #

  # This is set if we've already loaded the standard images.
  common _loaded_images 0

  # The title of the window, as known by the IDE.
  protected _ide_title

  # The list of all control buttons (ones which should be disabled when
  # not running anything or when inferior is running)
  protected ControlButtons {}

  # The list of all other buttons (which are disabled when inderior is
  # running)
  protected OtherButtons {}

  # The main window's menu
  protected Menu

  # The list of all menu entries which should be disabled when either no exe
  # is loaded/running or whenever the inferior is running
  protected ControlMenus

  # The list of all menu entries which should be disabled when the inferior is
  # running.
  protected OtherMenus
}
