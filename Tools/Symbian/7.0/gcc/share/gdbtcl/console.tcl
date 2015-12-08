#
# Console
# ----------------------------------------------------------------------
# Implements a console display widget using primitive widgets as the building
# blocks.  
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


itcl_class Console {
  # ------------------------------------------------------------------
  #  CONSTRUCTOR - create new console window
  # ------------------------------------------------------------------
  constructor {config} {
    #
    #  Create a window with the same name as this object
    #
    global tcl_platform
    set class [$this info class]
    @@rename $this $this-tmp-
    @@frame $this -class $class 
    @@rename $this $this-win-
    @@rename $this-tmp- $this
    @@tixScrolledText $this.stext -scrollbar both -sizebox 1
    set twin [$this.stext subwidget text]

    $twin config -wrap none
    $twin tag configure prompt_tag -foreground [pref get gdb/console/prompt_fg]
    $twin tag configure err_tag -foreground [pref get gdb/console/error_fg]
    $twin configure -font [pref get gdb/console/font]

    #
    # bind editing keys for console window
    #
    bind $twin <Return> "$this invoke; break"

    # History control.
    bind_plain_key $twin Control-p "$this previous; break"
    bind $twin <Up> "$this previous; break"
    bind_plain_key $twin Control-n "$this next; break"
    bind $twin <Down> "$this next; break"
    bind $twin <Meta-less> "$this first; break"
    bind $twin <Home> "$this first; break"
    bind $twin <Meta-greater> "$this last; break"
    bind $twin <End> "$this last; break"

    # Tab completion
    bind_plain_key $twin KeyPress-Tab "$this complete; break"

    # Don't let left arrow or ^B go over the prompt
    bind_plain_key $twin Control-b {
      if {[%W compare insert <= {cmdmark + 1 char}]} {
	break
      }
    }
    bind $twin <Left> [bind $twin <Control-b>]

    # Don't let Control-h, Delete, or Backspace back up over the prompt.
    bind_plain_key $twin Control-h {
      if {[%W compare insert <= {cmdmark + 1 char}]} {
	break
      }
    }
    bind $twin <BackSpace> [bind $twin <Control-h>]
    bind $twin <Delete> {
      if {[pref get gdb/console/deleteLeft]
	  && [%W compare insert <= cmdmark]} {
	break
      }
    }

    # Control-a moves to start of line.
    bind_plain_key $twin Control-a {
      %W mark set insert {cmdmark + 1 char}
      break
    }

    # Control-u deletes to start of line.
    bind_plain_key $twin Control-u {
      %W delete {cmdmark + 1 char} insert
    }
    
    # Control-w deletes previous word.
    bind_plain_key $twin Control-w {
      if {[%W compare {insert -1c wordstart} > cmdmark]} {
	%W delete {insert -1c wordstart} insert
      }
    }

    # Don't allow key motion to move insertion point outside the command
    # area.  This is done by fixing up the insertion point after any key
    # movement.  We only need to do this after events we do not
    # explicitly override.  Note that since the edit line is always the
    # last line, we can't possibly go past it, so we don't bother
    # checking that.
    foreach event [bind Text] {
      if {[string match *Key* $event] && [bind $twin $event] == ""} {
	bind $twin $event {
	  if {[%W compare insert <= {cmdmark + 1 char}]} {
	    %W mark set insert {cmdmark + 1 char}
	  }
	}
      }
    }

    # Don't allow mouse to put cursor outside command line.  For some
    # events we do this by noticing when the cursor is outside the
    # range, and then saving the insertion point.  For others we notice
    # the saved insertion point.
    set pretag pre-$twin
    bind $twin <1> [format {
      if {[%%W compare [tkTextClosestGap %%W %%x %%y] <= cmdmark]} {
	%s _insertion [%%W index insert]
      } else {
	%s _insertion {}
      }
    } $this $this]
    bind $twin <B1-Motion> [format {
      if {[%s _insertion] != ""} {
	%%W mark set insert [%s _insertion]
      }
    } $this $this $this]
    # FIXME: has inside information.
    bind $twin <ButtonRelease-1> [format {
      tkCancelRepeat
      if {[%s _insertion] != ""} {
	%%W mark set insert [%s _insertion]
      }
      %s _insertion {}
      break
    } $this $this $this]

    # Don't allow inserting text outside the command line.  FIXME:
    # requires inside information.
    # Also make it a little easier to paste by making the button
    # drags a little "fuzzy".
    bind $twin <B2-Motion> {
      if {!$tk_strictMotif} {
	if {($tkPriv(x) - 2 < %x < $tkPriv(x) + 2) \
	      || ($tkPriv(y) - 2 < %y < $tkPriv(y) + 2)} {
	  set tkPriv(mouseMoved) 1
	}
	if {$tkPriv(mouseMoved)} {
	  %W scan dragto %x %y
	}
      }
      break
    }
    bind $twin <ButtonRelease-2> {
      if {!$tkPriv(mouseMoved) || $tk_strictMotif} {
	event generate %W <<Paste>>
	break
      }
    }
    bind [winfo toplevel $this] <<Paste>> "$this paste; break"

    setprompt
    pack $this.stext -expand yes -fill both

    # Add hooks
    add_hook gdb_busy_hook [list $this busy]
    add_hook gdb_idle_hook [list $this idle]
    add_hook gdb_no_inferior_hook [list $this idle]

    focus $twin
  }

  # ------------------------------------------------------------------
  #  DESTRUCTOR - destroy window containing widget
  # ------------------------------------------------------------------
  destructor {

    # Remove hooks
    remove_hook gdb_busy_hook [list $this busy]
    remove_hook gdb_idle_hook [list $this idle]
    remove_hook gdb_no_inferior_hook [list $this idle]

    set top [winfo toplevel $this]
    destroy $this
    destroy $top
  }

  method idle {} {
    set Running 0
  }

  method busy {} {
    set Running 1
  }

  # ------------------------------------------------------------------
  #  METHOD:  config - used to change public attributes
  # ------------------------------------------------------------------
  method config {config} {}
    
  # ------------------------------------------------------------------
  #  METHOD:  insert - insert new text in the text widget
  # ------------------------------------------------------------------
  method insert {line} {
    if {$_needNL} {
      $twin insert {insert linestart} "\n"
    }
    # Remove all \r characters from line.
    set line [join [split $line \r] {}]
    $twin insert {insert -1 line lineend} $line

    set nlines [lindex [split [$twin index end] .] 0]
    if {$nlines > $throttle} {
      set delta [expr {$nlines - $throttle}]
      $twin delete 1.0 ${delta}.0
    }

    $twin see insert
    set _needNL 0
    @@update idletasks
  }
  
  #-------------------------------------------------------------------
  #  METHOD:  einsert - insert error text in the text widget
  # ------------------------------------------------------------------
  method einsert {line} {
    debug "einsert"
    if {$_needNL} {
      $twin insert end "\n"
    }
    $twin insert end $line err_tag
    $twin see insert
    set _needNL 0
  }

  #-------------------------------------------------------------------
  #  METHOD:  previous - recall the previous command
  # ------------------------------------------------------------------
  method previous {} {
    if {$_histElement == -1} {
      # Save partial command.
      set _partialCommand [$twin get {cmdmark + 1 char} {cmdmark lineend}]
    }
    incr _histElement
    set text [lindex $_history $_histElement]
    if {$text == ""} {
      # No dice.
      incr _histElement -1
      # FIXME flash window.
    } else {
      $twin delete {cmdmark + 1 char} {cmdmark lineend}
      $twin insert {cmdmark + 1 char} $text
    }
  }

  #-------------------------------------------------------------------
  #  METHOD:  next - recall the next command (scroll forward)
  # ------------------------------------------------------------------
  method next {} {
    if {$_histElement == -1} {
      # FIXME flash window.
      return
    }
    incr _histElement -1
    if {$_histElement == -1} {
      set text $_partialCommand
    } else {
      set text [lindex $_history $_histElement]
    }
    $twin delete {cmdmark + 1 char} {cmdmark lineend}
    $twin insert {cmdmark + 1 char} $text
  }

  #-------------------------------------------------------------------
  #  METHOD:  last - get the last history element
  # ------------------------------------------------------------------
  method last {} {
    set _histElement 0
    next
  }

  #-------------------------------------------------------------------
  #  METHOD:  first - get the first (earliest) history element
  # ------------------------------------------------------------------
  method first {} {
    set _histElement [expr {[llength $_history] - 1}]
    previous
  }



  #-------------------------------------------------------------------
  #  METHOD:  setprompt - put a prompt at the beginning of a line
  # ------------------------------------------------------------------
  method setprompt {{prompt {}}} {
    if {$_invoking} {
      set prompt ""
    } elseif {"$prompt" != ""} {
      # nothing
    } else {
      #set prompt [pref get gdb/console/prompt]
      set prompt [gdb_prompt]
    }

    $twin insert {insert linestart} $prompt prompt_tag
    $twin mark set cmdmark "insert -1 char"
    $twin see insert
  }

  #-------------------------------------------------------------------
  #  METHOD:  activate - run this after a command is run
  # ------------------------------------------------------------------
  method activate {{prompt {}}} {
    if {$_invoking > 0} {
      incr _invoking -1
      setprompt $prompt
    }
  }

  #-------------------------------------------------------------------
  #  METHOD:  invoke - invoke a command
  # ------------------------------------------------------------------
  method invoke {} {
    global gdbtk_state

    incr _invoking
    set text [$twin get {cmdmark + 1 char} {cmdmark lineend}]
    if {$text == ""} {
      set text [lindex $_history 0]
      $twin insert {insert lineend} $text
    }
    $twin mark set insert {insert lineend}
    $twin insert {insert lineend} "\n"

    set ok 0
    if {$Running} {
      if {[string index $text 0] == "!"} {
	set text [string range $text 1 end]
	set ok 1
      }
    }

    # Only push new nonempty history items.
    if {$text != "" && [lindex $_history 0] != $text} {
      lvarpush _history $text
    }
    
    set index [$twin index insert]
    
    # Clear current history element, and current partial element.
    set _histElement -1
    set _partialCommand ""
    
    # Need a newline before next insert.
    set _needNL 1
    
    # run command
    if {$gdbtk_state(readline)} {
      set gdbtk_state(readline_response) $text
      return
    }

    if {!$Running || $ok} {
      set result [catch {gdb_immediate "$text"} message]
    } else {
      set result 1
      set message "The debugger is busy."
    }

    # gdb_immediate may take a while to finish.  Exit if
    # our window has gone away.
    if {![winfo exists $twin]} { return }

    if {$result} {
      global errorInfo
      debug "Error: $errorInfo\n"
      $twin insert end "Error: $message\n" err_tag
    } elseif {$message != ""} {
      $twin insert $index "$message\n"
    }
    
    # Make the prompt visible again.
    activate
    
    # Make sure the insertion point is visible.
    $twin see insert
  }

  #-------------------------------------------------------------------
  #  PRIVATE METHOD:  _insertion - Set or get saved insertion point
  # ------------------------------------------------------------------
  method _insertion {args} {
    if {! [llength $args]} {
      return $_saved_insertion
    } else {
      set _saved_insertion [lindex $args 0]
    }
  }

  # ------------------------------------------------------------------
  #  METHOD:  paste - paste the selection into the console window
  # ------------------------------------------------------------------
  method paste {} {
    set sel {}
    catch {set sel [selection get]}
    if {$sel == "" && [catch {set sel [selection get -selection CLIPBOARD]}]} {
      return
    }

    if {$sel != ""} {
      $twin insert insert $sel
    }
  }

  # ------------------------------------------------------------------
  #  METHOD: reconfig - preferences have changed, update!
  #                     Lucky for us, we are only a font. We need
  #                     not do anything (maybe resize?)
  # ------------------------------------------------------------------
  method reconfig {} {
  }

  method get_text {} {
    return $twin
  }

  # ------------------------------------------------------------------
  #  METHOD:  find_lcp - Return the longest common prefix in SLIST.
  #              Can be empty string.
  # ------------------------------------------------------------------
  method find_lcp {slist} {
    # Handle trivial cases where list is empty or length 1
    if {[llength $slist] <= 1} {return [lindex $slist 0]}

    set prefix [lindex $slist 0]
    set prefixlast [expr [string length $prefix] - 1]

    foreach str [lrange $slist 1 end] {
      set test_str [string range $str 0 $prefixlast]
      while {[string compare $test_str $prefix] != 0} {
	incr prefixlast -1
	set prefix [string range $prefix 0 $prefixlast]
	set test_str [string range $str 0 $prefixlast]
      }
      if {$prefixlast < 0} break
    }
    return $prefix
  }
  
  # ------------------------------------------------------------------
  #  METHOD:  find_completion - Look through COMPLETIONS to generate
  #             the suffix needed to do command
  # ------------------------------------------------------------------
  method find_completion {cmd completions} {
    # Get longest common prefix
    set lcp [find_lcp $completions]
    set cmd_len [string length $cmd]
    # Return suffix beyond end of cmd
    return [string range $lcp $cmd_len end]
  }
 
  # ------------------------------------------------------------------
  #  METHOD: complete - Command line completion
  # ------------------------------------------------------------------
  method complete {} {

    set command_line [$twin get {cmdmark + 1 char} {cmdmark lineend}]
    set choices [gdb_cmd "complete $command_line"]
    set choices [string trimright $choices \n]
    set choices [split $choices \n]

    # Just do completion if this is the first tab
    if {!$saw_tab} {
      set saw_tab 1
      set completion [find_completion $command_line $choices]

      # Here is where the completion is actually done.  If there
      # is one match, complete the command and print a space.
      # If two or more matches, complete the command and beep.
      # If no match, just beep.
      switch {[llength $choices]} {
	0 {}
	1 {
	  twin insert end "$completion "
	  set saw_tab 0
	  return
	}

	default {
	  $twin insert end $completion
	}
      }
      bell
      $twin see end
      bind $twin <KeyPress> "$this reset_tab"
    } else {
      # User hit another consecutive tab.  List the choices.
      # Note that at this point, choices may contain commands
      # with spaces.  We have to lop off everything before (and
      # including) the last space so that the completion list
      # only shows the possibilities for the last token.
      set choices [lsort $choices]
      if {[regexp ".* " $command_line prefix]} {
	regsub -all $prefix $choices {} choices
      }
      if {[llength choices] != 0} {
	insert "\nCompletions:\n[join $choices \ ]\n"
	$twin see end
	bind $twin <KeyPress> "$this reset_tab"
      }
    }
  }

  # ------------------------------------------------------------------
  #  METHOD:  reset_tab - Helper method for tab completion. Used
  #             to reset the tab when a key is pressed.
  # ------------------------------------------------------------------
  method reset_tab {} {
    bind $twin <KeyPress> {}
    set saw_tab 0
  }

  #
  #  PUBLIC DATA
  #    throttle ...... Approximate maximum number of lines allowed in widget
  #
  public throttle 2000

  #
  #  PROTECTED DATA
  #    twin .......... the text subwidget
  #
  protected twin
  protected _invoking 0
  protected _needNL 1
  protected _history {}
  protected _histElement -1
  protected _partialCommand ""
  protected _saved_insertion ""
  protected Running 0
  protected saw_tab 0
}
