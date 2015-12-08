# ventry.tcl - Entry with validation
# Copyright (C) 1997 Cygnus Solutions.
# Written by Tom Tromey <tromey@cygnus.com>.

itcl_class Validated_entry {
  # The validation command.  It is passed the contents of the entry.
  # It should throw an error if there is a problem; the error text
  # will be displayed to the user.
  public command {}

  constructor {config} {
    upvar \#0 $this state

    # The standard widget-making trick.
    set class [$this info class]
    @@rename $this $this-tmp-
    @@frame $this -class $class -borderwidth 0
    @@rename $this $this-win-
    @@rename $this-tmp- $this

    @@set ${this}(value) ""
    @@entry $this.entry -textvariable ${this}(value)
    pack $this.entry -expand 1 -fill both

    bind $this.entry <Map> [list $this _map]
    bind $this.entry <Unmap> [list $this _unmap]
    bind $this.entry <Destroy> [list $this delete]
    # We never want the focus on the frame.
    bind $this <FocusIn> [list focus $this.entry]

    # This window is used when the user enters a bad name for the new
    # executable.  The color here is "plum3".  We use a toplevel here
    # both to get a nice black border and because a frame would be
    # clipped by its parents.
    toplevel $this.badname -borderwidth 1 -background black -relief flat
    wm withdraw $this.badname
    wm overrideredirect $this.badname 1

    @@set state(message) ""

    # FIXME: -textvariable didn't work; I suspect itcl.
    @@label $this.badname.text -anchor w -justify left \
      -background \#cdd29687cdd2 ;# -textvariable ${this}(message)
    pack $this.badname.text -expand 1 -fill both

    # Trace the entry contents.
    uplevel \#0 [list trace variable ${this}(value) w [list $this _trace]]
  }

  destructor {
    upvar \#0 $this state
    catch {destroy $this}
    uplevel \#0 [list trace vdelete ${this}(value) w [list $this _trace]]
    unset state
  }

  method configure {config} {}

  # Return 1 if we're in the error state, 0 otherwise.
  method is_error {} {
    upvar \#0 $this state
    return [expr {$state(message) != ""}]
  }

  # Return error text.
  method error_text {} {
    upvar \#0 $this state
    return $state(message)
  }

  # Some methods to forward messages to the entry.  Add more as
  # required.

  # FIXME: itcl 1.5 won't let us have a `delete' method.  Sigh.
  method delete_hack {args} {
    return [eval $this.entry delete $args]
  }

  method get {} {
    return [$this.entry get]
  }

  method insert {index string} {
    return [$this.entry insert $index $string]
  }


  # This is run to display the label.  Private method.
  method _display {} {
    # FIXME: place above if it would go offscreen.
    set y [expr {[winfo rooty $this.entry] + [winfo height $this.entry] + 1}]
    set x [expr {round ([winfo rootx $this.entry]
			+ 0.12 * [winfo width $this.entry])}]
    wm positionfrom $this.badname user
    wm geometry $this.badname +$x+$y
    # Workaround for Tk 8.0b2 bug on NT.
    update
    wm deiconify $this.badname
    raise $this.badname
  }

  # This is run when the entry widget is mapped.  If we have an error,
  # map our error label.  Private method.
  method _map {} {
    if {[is_error]} then {
      _display
    }
  }

  # This is run when the entry widget is unmapped.  Private method.
  method _unmap {} {
    wm withdraw $this.badname
  }

  # This is called when the entry contents change.  Private method.
  method _trace {args} {
    upvar \#0 $this state

    if {$command != ""} then {
      set cmd $command
      lappend cmd $state(value)
      set cmd [list uplevel \#0 $cmd]
    }
    if {[info exists cmd] && [catch $cmd msg]} then {
      # FIXME: for some reason, the -textvariable on the label doesn't
      # work.  I suspect itcl.
      set state(message) $msg
      $this.badname.text configure -text $msg
      _display
    } else {
      set state(message) ""
      wm withdraw $this.badname
    }
  }
}
