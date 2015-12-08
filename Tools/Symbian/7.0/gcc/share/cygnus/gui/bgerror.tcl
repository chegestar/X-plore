# bgerror.tcl - Send bug report in response to uncaught Tcl error.
# Copyright (C) 1997 Cygnus Solutions.
# Written by Tom Tromey <tromey@cygnus.com>.

# Make sure these preferences are always defined.
pref define global/show_tcl_errors 0
pref define global/show_tcl_stack_trace 0

proc bgerror err {
  global errorInfo errorCode

  set info $errorInfo
  set code $errorCode

  # Don't show errors at all unless the user has requested them.
  if {! [pref get global/show_tcl_errors]} then {
    if {[grab current .] != ""} {
      grab release [grab current .]
    }
    return
  }

  set command [list tk_dialog .bgerrorDialog [gettext "GDB Error"] \
		 [format [gettext "Error: %s"] $err] \
		 error 0 [gettext "OK"]]
  if {[pref get global/show_tcl_errors]} then {
    lappend command [gettext "Stack Trace"]
  }

  set value [eval $command]
  if {$value == 0} then {
    return
  }

  set w .bgerrorTrace
  catch {destroy $w}
  toplevel $w -class ErrorTrace
  wm minsize $w 1 1
  wm title $w "Stack Trace for Error"
  wm iconname $w "Stack Trace"
  button $w.ok -text OK -command "destroy $w" -default active
  text $w.text -relief sunken -bd 2 -yscrollcommand "$w.scroll set" \
    -setgrid true -width 60 -height 20
  scrollbar $w.scroll -relief sunken -command "$w.text yview"
  pack $w.ok -side bottom -padx 3m -pady 2m
  pack $w.scroll -side right -fill y
  pack $w.text -side left -expand yes -fill both
  $w.text insert 0.0 "errorCode is $errorCode"
  $w.text insert 0.0 $info
  $w.text mark set insert 0.0

  bind $w <Return> "destroy $w"
  bind $w.text <Return> "destroy $w; break"

  # Center the window on the screen.

  wm withdraw $w
  update idletasks
  set x [expr [winfo screenwidth $w]/2 - [winfo reqwidth $w]/2 \
	   - [winfo vrootx [winfo parent $w]]]
  set y [expr [winfo screenheight $w]/2 - [winfo reqheight $w]/2 \
	   - [winfo vrooty [winfo parent $w]]]
  wm geom $w +$x+$y
  wm deiconify $w

  # Be sure to release any grabs that might be present on the
  # screen, since they could make it impossible for the user
  # to interact with the stack trace.

  if {[grab current .] != ""} {
    grab release [grab current .]
  }
}
