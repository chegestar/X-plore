#
# PrecessWin
# ----------------------------------------------------------------------
# Implements a process window with a list of threads, tasks, and/or
# processes to debug.
#
# ----------------------------------------------------------------------
#   AUTHOR:  Martin M. Hunt <hunt@cygnus.com>
#   Copyright (C) 1998 Cygnus Solutions
#


itcl_class ProcessWin {
  # ------------------------------------------------------------------
  #  CONSTRUCTOR - create new process window
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
    add_hook gdb_no_inferior_hook [list $this idle]
    add_hook gdb_idle_hook [list $this idle]
    after idle [list wm deiconify $top]
  }


  # ------------------------------------------------------------------
  #  METHOD:  build_win - build the main process window
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
      -selectbackground green \
      -selectforeground black \
      -font src-font
    update
    balloon register $lb "Click on a line to change context"

    # bind mouse button 1 to change the current context
    bind $lb <ButtonPress-1> "$this change_context %y"
    bind $lb <ButtonRelease-1> break

    pack $this.s -side left -expand yes -fill both
  }


  # ------------------------------------------------------------------
  #  METHOD:  update - update widget when something changes
  # ------------------------------------------------------------------
  method update {} {
    if {!$protect_me} {

      $lb delete 0 end
      if {[catch {gdb_cmd "info thread"} threads]} {
	# failed.  leave window blank
	return
      }

      #debug "processWin update: \n$threads"
      if {[llength $threads] == 0} {
	# no processes/threads listed.
	return
      }
    
      # insert each line one at a time
      set active -1
      set num_threads 0
      foreach line [split $threads \n] {
	# Active line starts with "*"
	if {[string index $line 0] == "*"} {
	  # strip off leading "*"
	  set line " [string trimleft $line "*"]"
	  set active $num_threads
	}
	# scan for GDB ID number at start of line
	if {[scan $line "%d" id($num_threads)] == 1} {
	  $lb insert end $line
	  incr num_threads
	}
      }
      
      # highlight the active thread
      if {$active >= 0} {
	set active_thread $id($active)
	$lb selection set $active
	$lb see $active
      }
    }
  }

  # ------------------------------------------------------------------
  #  METHOD:  change_context - change the current context (active thread)
  #        This method is currently ONLY called from the mouse binding
  # ------------------------------------------------------------------
  method change_context {y} {
    if {!$Running && [$lb size] != 0} {
      gdbtk_busy
      set linenum [$lb nearest $y]
      set idnum $id($linenum)
      #debug "change_context to line $linenum  id=$idnum"
      catch {gdb_cmd "thread $idnum"}
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
  #  METHOD:  idle - idle hook.  Run when the target is not running
  # ------------------------------------------------------------------
  method idle {} {
    set Running 0
    cursor {}
  }

  # ------------------------------------------------------------------
  #  METHOD:  cursor - set the window cursor
  #        This is a convenience method which simply sets the mouse
  #        pointer to the given glyph.
  # ------------------------------------------------------------------
  method cursor {glyph} {
    $top configure -cursor $glyph
  }

  #
  #  PROTECTED DATA
  #
  protected top
  protected lb
  protected id
  protected Running 0
  protected protect_me 0
}

