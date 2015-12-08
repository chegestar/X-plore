# hooks.tcl - Hook functions.
# Copyright (C) 1997 Cygnus Solutions.
# Written by Tom Tromey <tromey@cygnus.com>.

proc add_hook {hook command} {
  upvar \#0 $hook var
  lappend var $command
}

proc remove_hook {hook command} {
  upvar \#0 $hook var
  set var [lremove $var $command]
}

proc define_hook {hook} {
  upvar \#0 $hook var

  if {! [info exists var]} then {
    set var {}
  }
}

proc run_hooks {hook args} {
  upvar \#0 $hook var
  foreach thunk $var {
    uplevel \#0 $thunk $args
  }
}
