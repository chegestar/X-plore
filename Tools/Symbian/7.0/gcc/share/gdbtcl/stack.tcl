#
# StackWin
# ----------------------------------------------------------------------
# Implements stack window for gdb
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


itcl_class StackWin {
  # ------------------------------------------------------------------
  #  CONSTRUCTOR - create new stack window
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
    set top [winfo toplevel $this]
    wm withdraw $top
    gdbtk_busy
    build_win

    gdbtk_idle
    # Add hooks for this object
    add_hook gdb_update_hook [list $this update]
    add_hook gdb_busy_hook [list $this busy]
    add_hook gdb_no_inferior_hook [list $this no_inferior]
    add_hook gdb_idle_hook [list $this idle]
    after idle [list wm deiconify $top]
  }


  # ------------------------------------------------------------------
  #  METHOD:  build_win - build the main register window
  # ------------------------------------------------------------------
  method build_win {} {
    global tixOption tcl_platform
    if {$tcl_platform(platform) == "windows"} {
      tixScrolledListBox $this.s -scrollbar both -sizebox 1
    } else {
      tixScrolledListBox $this.s -scrollbar auto
    }
    set lb [$this.s subwidget listbox]
    $lb configure -selectmode single -bg $tixOption(input1_bg) \
      -selectbackground [pref get gdb/src/STACK_TAG] \
      -selectforeground black \
      -font src-font
    update
    $lb configure -width $maxwidth
    balloon register $lb "Click on a frame to select the current frame"

    # bind mouse button 1 to change the stack frame
    bind $lb <ButtonPress-1> "$this change_frame %y"
    bind $lb <ButtonRelease-1> break

    pack $this.s -side left -expand yes -fill both
  }


  # ------------------------------------------------------------------
  #  METHOD:  update - update widget when PC changes
  # ------------------------------------------------------------------
  method update {} {
    if {!$protect_me} {
      global gdb_selected_frame_level

      debug "START STACK IDLE CALLBACK $gdb_selected_frame_level"
    set lb [$this.s subwidget listbox]
      set frames [gdb_stack 0 -1]

      if {[llength $frames] == 0} {
	$lb delete 0 end
	$lb insert end {NO STACK}
	debug "END STACK IDLE CALLBACK"
	return
      }
    
    $lb delete 0 end
      set levels 0
      foreach frame $frames {
	set len [string length $frame]

	if {$len > $maxwidth} {
	  set maxwidth $len
	}
	$lb insert end $frame
	incr levels
    }

    # this next section checks to see if the source
    # window is looking at some location other than the 
    # bottom of the stack.  If so, highlight the stack frame
      set level [expr {$levels - $gdb_selected_frame_level - 1}]
      $lb selection set $level
      $lb see $level

    debug "END STACK IDLE CALLBACK"
  }
  }

  method idle {} {
    set Running 0
    cursor {}
  }

  # ------------------------------------------------------------------
  #  METHOD:  change_frame - change the current frame
  #        This method is currently ONLY called from the mouse binding
  # ------------------------------------------------------------------
  method change_frame {y} {
    set lb [$this.s subwidget listbox]

    if {!$Running && [$lb size] != 0} {
      gdbtk_busy
      set lb [$this.s subwidget listbox] 
      set linenum [$lb nearest $y]
      set size [$lb size]
      set linenum [expr {$size - $linenum - 1}]
      catch {gdb_cmd "frame $linenum"}
      # Run idle hooks and cause all widgets to update
      set protect_me 1
      gdbtk_update
      set protect_me 0
      gdbtk_idle
    }
  }

  # ------------------------------------------------------------------
  #  DESTRUCTOR - destroy window containing widget
  # ------------------------------------------------------------------
  destructor {
    set top [winfo toplevel $this]

    remove_hook gdb_update_hook [list $this update]
    remove_hook gdb_idle_hook [list $this idle]
    remove_hook gdb_busy_hook [list $this busy]
    remove_hook gdb_no_inferior_hook [list $this no_inferior]
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
    destroy $this.s
    build_win
  }

  # ------------------------------------------------------------------
  #  METHOD:  busy - gdb_busy_hook
  #
  #        This method should accomplish blocking
  #        - clicks in the window
  #        - change mouse pointer
  # ------------------------------------------------------------------
  method busy {} {
    set Running 1
    cursor watch
  }

  # ------------------------------------------------------------------
  #  METHOD:  no_inferior - gdb_no_inferior_hook
  # ------------------------------------------------------------------
  method no_inferior {} {
    set Running 0
    cursor {}
  }

  # ------------------------------------------------------------------
  #  METHOD:  cursor - set the window cursor
  #        This is a convenience method which simply sets the mouse
  #        pointer to the given glyph.
  # ------------------------------------------------------------------
  method cursor {glyph} {
    set top [winfo toplevel $this]
    $top configure -cursor $glyph
  }

  #
  #  PROTECTED DATA
  #
  protected maxwidth 40
  protected Running 0
  protected protect_me 0
}

