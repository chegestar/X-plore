#
# GDBSrcBar
# ----------------------------------------------------------------------
# Implements a toolbar that is attached to a source window.
#
#   PUBLIC ATTRIBUTES:
#
#
#   METHODS:
#
#     config ....... used to change public attributes
#
#   PRIVATE METHODS
#
#   X11 OPTION DATABASE ATTRIBUTES
#
#
# ----------------------------------------------------------------------
#   AUTHOR:  Tom Tromey                tromey@cygnus.com   
#            Copyright (C) 1997, 1998  Cygnus Solutions   
#


itcl_class GDBSrcBar {
  inherit GDBToolBar

  # ------------------------------------------------------------------
  #  CONSTRUCTOR - create widget
  # ------------------------------------------------------------------
  constructor {config} {
    # FIXME: This is a huge hack: we know the IDE title of our window.
    set _ide_title "Foundry _Debugger"
  }

  # ------------------------------------------------------------------
  #  DESTRUCTOR - destroy window containing widget
  # ------------------------------------------------------------------
  destructor {
    global GDBSrcBar_state
    unset GDBSrcBar_state($this)
  }

  #
  #  PUBLIC DATA
  #

  # This is a handle on our parent source window.
  public source {}

  # This is the command that should be run when the `update'
  # checkbutton is toggled.  The current value of the checkbutton is
  # appended to the command.
  public updatecommand {}

  # This controls whether the `update' checkbutton is turned on or
  # off.
  public updatevalue 0 {
    global GDBSrcBar_state
    @@set GDBSrcBar_state($this) $updatevalue
  }

  # This holds the text that is shown in the address label.
  public address {} {
    if {$_frame != "" && [winfo exists $_frame.addr]} {
      $_frame.addr configure -text $address -font src-font
    }
  }

  # This holds the text that is shown in the line label.
  public line {} {
    if {$_frame != "" && [winfo exists $_frame.line]} {
      $_frame.line configure -text $line
    }
  }

  # This holds the source window's display mode.  Valid values are
  # SOURCE, ASSEMBLY, SRC+ASM, and MIXED.
  public displaymode SOURCE {
    if {$_frame != ""} {
      _set_stepi
    }
  }

  # This is true if the inferior is running, or false if it is
  # stopped.
  public runstop 0 {
    if {$_frame != ""} {
	_set_runstop
    }
  }

  # ------------------------------------------------------------------
  #  METHOD:  create_menu_items - Add some menu items to the menubar.
  #                               Returns 1 if any items added.
  #  This overrides the method in GDBToolBar.
  # ------------------------------------------------------------------
  method create_menu_items {menu} {
    global IDE_ENABLED tcl_platform

    previous create_menu_items $menu
    set file_menu [$menu entrycget "File" -menu]

    if {$tcl_platform(platform) == "windows"} {
      $file_menu insert 0 command -label "Print Source..." -underline 0 \
	-command "$this _apply_source print" -accelerator "Ctrl+P"
      if {!$IDE_ENABLED} {
	$file_menu insert 0 command -label "Page Setup..." -underline 8 \
	  -command [format {
	    set top %s
	    ide_winprint page_setup -parent $top
	  } [winfo toplevel $this]]
      }
    }

    if {$IDE_ENABLED} {
      if {$tcl_platform(platform) == "windows"} {
	$file_menu insert 0 command -label "Page Setup..." -underline 8 \
	  -command [format {
	    set top %s
	    idewindow_freeze $top
	    ide_winprint page_setup -parent $top
	    idewindow_thaw $top
	  } [winfo toplevel $this]]
      }
      $file_menu insert 0 separator
      $file_menu insert 0 command -label "Edit Source" -underline 0 \
	-command [list $this _apply_source edit]
    } else {
      $file_menu insert 0 command -label "Open..." -underline 0 \
        -command "$this _open_file" -accelerator "Ctrl+O"
    }

    return 1
  }
  
  # ------------------------------------------------------------------
  #  METHOD:  create_buttons - Add some buttons to the toolbar.  Returns
  #                         list of buttons in form acceptable to
  #                         standard_toolbar.
  #  This overrides the method in GDBToolBar.
  # ------------------------------------------------------------------
  method create_buttons {frame} {
    global IDE_ENABLED

    _load_src_images

    set _frame $frame
    set button_list {}

    if { ![pref get gdb/mode] } {

      # Synchronous mode (i.e., "normal" breakpoint way)

      button $frame.stop
      lappend button_list $frame.stop
      _set_runstop

      button $frame.step -command [list catch {gdb_immediate step}] \
	-image step_img
      lappend button_list $frame.step
      lappend ControlButtons $frame.step
      balloon register $frame.step "Step (S)" 

      button $frame.next -command [list catch {gdb_immediate next}] \
	-image next_img
      lappend button_list $frame.next
      lappend ControlButtons $frame.next
      balloon register $frame.next "Next (N)"

      button $frame.finish -command [list catch {gdb_immediate finish}] \
	-image finish_img
      lappend button_list $frame.finish
      lappend ControlButtons $frame.finish
      balloon register $frame.finish "Finish (F)"

      button $frame.continue -command [list catch {gdb_immediate continue}] \
	-image continue_img
      lappend button_list $frame.continue
      lappend ControlButtons $frame.continue
      balloon register $frame.continue "Continue (C)"

      # A spacer before the assembly-level items looks good.  It helps
      # to indicate that these are somehow different.
      lappend button_list -
      button $frame.stepi -command [list catch {gdb_immediate stepi}] \
	-image stepi_img
      lappend button_list $frame.stepi
      lappend ControlButtons $frame.stepi
      balloon register $frame.stepi "Step Asm Inst (S)"

      button $frame.nexti -command [list catch {gdb_immediate nexti}] \
	-image nexti_img
      lappend button_list $frame.nexti
      lappend ControlButtons $frame.nexti
      balloon register $frame.nexti "Next Asm Inst (N)"

      _set_stepi
    } else {

      # Asynchronous (tracepoint way)

      button $frame.stop
      lappend button_list $frame.stop
      _set_runstop

      button $frame.tfind -command {tfind_cmd tfind} -text "N"
      lappend button_list $frame.tfind
      balloon register $frame.tfind "Next Hit <N>"

      button $frame.tfindprev -command {tfind_cmd "tfind -"} \
	-text "P"
      lappend button_list $frame.tfindprev
      balloon register $frame.tfindprev "Previous Hit <P>"

      button $frame.tfindstart -command {tfind_cmd "tfind start"} \
	-text "F"
      lappend button_list $frame.tfindstart
      balloon register $frame.tfindstart "First Hit <F>"

      button $frame.tfindline -command {tfind_cmd "tfind line"} \
	-text "L" 
      lappend button_list $frame.tfindline
      balloon register $frame.tfindline "Next Line Hit <L>"

      button $frame.tfindtp -command { tfind_cmd "tfind tracepoint"} \
	-text "H"
      lappend button_list $frame.tfindtp
      balloon register $frame.tfindtp "Next Hit Here <H>"
   }

    lappend button_list -
    eval lappend button_list [previous create_buttons $frame]
    
    if {$IDE_ENABLED} {
      button $frame.edit -command [list $this _apply_source edit] -image edit_img
      balloon register $frame.edit "Open File"
      lappend button_list $frame.edit
      lappend OtherButtons $frame.edit
      lappend button_list -
    }

    label $frame.addr -width 10 -relief sunken -bd 1 -anchor e \
      -text $address -font  src-font
    balloon register $frame.addr "Address"
    lappend button_list $frame.addr

    label $frame.line -width 6 -relief sunken -bd 1 -anchor e \
      -text $line -font  src-font
    balloon register $frame.line "Line Number"
    lappend button_list $frame.line

    lappend button_list --
    button $frame.down -command [list $this _apply_source stack down] -image down_img
    lappend button_list $frame.down
    lappend ControlButtons $frame.down
    balloon register $frame.down "Down Stack Frame"

    button $frame.up -command [list $this _apply_source stack up] -image up_img
    lappend button_list $frame.up
    lappend ControlButtons $frame.up
    balloon register $frame.up "Up Stack Frame"

    button $frame.bottom -command [list $this _apply_source stack bottom] -image bottom_img
    lappend button_list $frame.bottom
    lappend ControlButtons $frame.bottom
    balloon register $frame.bottom "Go to Bottom of Stack"

    # This feature has been disabled for now.
    # checkbutton $frame.upd -command "$this _toggle_updates" \
    #   -variable GDBSrcBar_state($this)
    # lappend button_list $frame.upd
    # global GDBSrcBar_state
    # @@set GDBSrcBar_state($this) $updatevalue
    # balloon register $frame.upd "Toggle Window Updates"

    return $button_list
  }

  # ------------------------------------------------------------------
  #  METHOD:  _load_src_images - Load standard images.  Private method.
  # ------------------------------------------------------------------
  method _load_src_images { {reconf 0} } {
    global gdb_ImageDir

    if {!$reconf && $_loaded_src_images} {
      return
    }
    set _loaded_src_images 1

    foreach name {run stop step next finish continue edit stepi nexti up down bottom} {
      image create photo ${name}_img -file [file join $gdb_ImageDir ${name}.gif]
    }
  }

  # ------------------------------------------------------------------
  #  METHOD:  _toggle_updates - Run when the update checkbutton is
  #                             toggled.  Private method.
  # ------------------------------------------------------------------
  method _toggle_updates {} {
    global GDBSrcBar_state
    if {$updatecommand != ""} {
      uplevel \#0 $updatecommand $GDBSrcBar_state($this)
    }
  }

  # ------------------------------------------------------------------
  #  METHOD:  cancel_download
  # ------------------------------------------------------------------
  method cancel_download {} {
    global download_dialog download_cancel_ok

    if {"$download_dialog" != ""} {
      $download_dialog cancel
    } else {
      set download_cancel_ok 1
    }
  }

  # ------------------------------------------------------------------
  #  METHOD:  _set_runstop - Set state of run/stop button.
  # ------------------------------------------------------------------
  method _set_runstop {} {
    switch $runstop {
      3 {
	$_frame.stop configure -state disabled
      }
      2 {
	# Download
	$_frame.stop configure -state normal -image stop_img -command "$this cancel_download"
	balloon register $_frame.stop "Stop"
      }
      1 {
         if {![pref get gdb/mode]} {
             # sync: running inferior
	     $_frame.stop configure -state normal -image stop_img -command gdb_stop
	     balloon register $_frame.stop "Stop"
         } else {
             # async: tstart inferior
	     $_frame.stop configure -state normal -image stop_img -command do_tstop
	     balloon register $_frame.stop "End Collection"
         }
      }
      0 {
         if {![pref get gdb/mode]} {
	   # sync mode: inferior stopped
	   $_frame.stop configure -state normal -image run_img -command run_executable
           balloon register $_frame.stop "Run (R)"
         } else {
           #async mode : inferior tstopped
           $_frame.stop configure -state normal -image run_img -command run_executable
           balloon register $_frame.stop "Begin Collection"
         }
        }
      default {
	debug "SrcBar::_set_runstop - unknown state $runstop ($running)"
      }
    }
  }


  # ------------------------------------------------------------------
  #  METHOD:  _set_stepi - Set state of stepi/nexti buttons.
  # ------------------------------------------------------------------
  method _set_stepi {} {
    
    # Only do this in synchronous mode
    if {![pref get gdb/mode]} {
      # In source-only mode, disable these buttons.  Otherwise, enable
      # them.
      if {$displaymode == "SOURCE"} {
	set state disabled
      } else {
	set state normal
      }
      $_frame.stepi configure -state $state
      $_frame.nexti configure -state $state
    }
  }

  # ------------------------------------------------------------------
  #  METHOD:  _apply_source - Forward some method call to the source window.
  # ------------------------------------------------------------------
  method _apply_source {args} {
    if {$source != ""} {
      eval $source $args
    }
  }

  # ------------------------------------------------------------------
  #  METHOD: _open_file - Load a file for debugging
  # ------------------------------------------------------------------
  method _open_file {} {
    global gdb_running gdb_downloading tcl_platform

    if {$gdb_running || $gdb_downloading} {
      # We are already running/downloading something..
      if {$gdb_running} {
	set msg "A debugging session is active.\n"
	append msg "Abort session and load new file?"
      } else {
	set msg "A download is in progress.\n"
	append msg "Abort download and load new file?"
      }
      if {![gdbtk_tcl_query $msg no]} {
	return
      }
    }
    set w [winfo toplevel $this]
    set file [tk_getOpenFile -parent $w -title "Load New Executable"]
    
    # Add the base dir for this file to the source search path.
    set root [file dirname $file]
    if {$tcl_platform(platform) == "windows"} {
      set root [ide_cygwin_path to_posix $root]
      set file [ide_cygwin_path to_posix $file]
    }

    gdb_clear_file
    catch {gdb_cmd "cd $root"}
    set_exe_name $file
    if {[set_exe] == "cancel"} {
      gdbtk_update
      gdbtk_idle
      return
    }

    # Show main, if it exists
    foreach src [manage find src] {
      catch {$src location [gdb_loc main]}
    }
  }

  # ------------------------------------------------------------------
  #  METHOD:  reconfig - reconfigure the srcbar
  # ------------------------------------------------------------------
  method reconfig {} {
    _load_src_images 1
    previous reconfig
  }
  
  #
  #  PROTECTED DATA
  #

  # This is set if we've already loaded the standard images.  Private
  # variable.
  common _loaded_src_images 0

  # This is the name of the frame that holds all our buttons.  Private
  # variable.
  protected _frame {}
}

