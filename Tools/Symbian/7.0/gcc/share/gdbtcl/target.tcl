#
# TargetSelection
# ----------------------------------------------------------------------
# Implements GDB TargetSelection dialog
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

# Target Database
# Set the following members:
# TARGET,pretty-name: Name to display to user
# TARGET,debaud: Default baudrate
# TARGET,baud-rates: Permissible baudrates
# TARGET,cmd: Abstracted command to run for this target (tcpX and com1 are
#          replaced with the real port and host/port in set_target)
# TARGET,runlist: List of preferences for the target: {attach download run cont}

# Exec target
set gdb_target(exec,pretty-name) "Exec"
set gdb_target(exec,defbaud) ""
set gdb_target(exec,baud-rates) {}
set gdb_target(exec,cmd) ""
set gdb_target(exec,runlist) {0 0 1 0}
set gdb_target(exec,options) ""

# ADS board w/SDS protocol
set gdb_target(sds,pretty-name) "SDS"
set gdb_target(sds,defbaud) "38400"
set gdb_target(sds,baud-rates) {9600 38400}
set gdb_target(sds,cmd) "sds com1"
set gdb_target(sds,runlist) {1 0 0 1}

# Simulator
set gdb_target(sim,pretty-name) "Simulator"
set gdb_target(sim,defbaud) ""
set gdb_target(sim,baud-rates) {}
set gdb_target(sim,cmd) "sim"
set gdb_target(sim,runlist) {1 1 1 0}
set gdb_target(sim,options) ""

# Remote
set gdb_target(remote,pretty-name) "Remote/Serial"
set gdb_target(remote,defbaud) "38400"
set gdb_target(remote,baud-rates) {9600 38400}
set gdb_target(remote,cmd) "remote com1"
set gdb_target(remote,runlist) {1 0 0 1}
set gdb_target(remotetcp,pretty-name) "Remote/TCP"
set gdb_target(remotetcp,defbaud) "TCP"
set gdb_target(remotetcp,baud-rates) {}
set gdb_target(remotetcp,cmd) "remote tcpX"
set gdb_target(remotetcp,runlist) {1 0 0 1}

# ARM Angel
set gdb_target(rdi,pretty-name) "ARM Angel"
set gdb_target(rdi,defbaud) "9600"
set gdb_target(rdi,baud-rates) {9600}
set gdb_target(rdi,cmd) "rdi com1"
set gdb_target(rdi,runlist) {1 0 0 1}

# ARM Remote
set gdb_target(rdp,pretty-name) "ARM Remote/Serial"
set gdb_target(rdp,defbaud) "9600"
set gdb_target(rdp,baud-rates) {9600}
set gdb_target(rdp,cmd) "rdp com1"
set gdb_target(rdp,runlist) {1 0 0 1}
set gdb_target(rdptcp,pretty-name) "ARM Remote/TCP"
set gdb_target(rdptcp,defbaud) "TCP"
set gdb_target(rdptcp,baud-rates) {}
set gdb_target(rdptcp,cmd) "rdp tcpX"
set gdb_target(rdptcp,runlist) {1 0 0 1}

# d10v
set gdb_target(d10v,pretty-name) "D10V/Serial"
set gdb_target(d10v,defbaud) "38400"
set gdb_target(d10v,baud-rates) {9600 38400}
set gdb_target(d10v,cmd) "d10v com1"
set gdb_target(d10v,runlist) {1 0 0 1}
set gdb_target(d10vtcp,pretty-name) "D10V/TCP"
set gdb_target(d10vtcp,cmd) "d10v tcpX"
set gdb_target(d10vtcp,defbaud) "TCP"
set gdb_target(d10vtcp,baud-rates) {}
set gdb_target(d10vtcp,runlist) {1 0 0 1}

# m32r
set gdb_target(m32r,pretty-name) "M32R/Serial"
set gdb_target(m32r,defbaud) "38400"
set gdb_target(m32r,baud-rates) {9600 38400}
set gdb_target(m32r,cmd) "m32r com1"
set gdb_target(m32r,runlist) {1 0 0 1}
set gdb_target(m32rtcp,pretty-name) "M32R/TCP"
set gdb_target(m32rtcp,defbaud) "TCP"
set gdb_target(m32rtcp,baud-rates) {}
set gdb_target(m32rtcp,cmd) "m32r tcpX"
set gdb_target(m32rtcp,runlist) {1 0 0 1}

# sparclite
set gdb_target(sparclite,pretty-name) "SPARClite/Serial"
set gdb_target(sparclite,defbaud) "9600"
set gdb_target(sparclite,baud-rates) {9600}
set gdb_target(sparclite,cmd) "sparclite com1"
set gdb_target(sparclite,runlist) {1 0 0 1}
set gdb_target(sparclitetcp,pretty-name) "SPARClite/TCP"
set gdb_target(sparclitetcp,defbaud) "TCP"
set gdb_target(sparclitetcp,baud-rates) {}
set gdb_target(sparclitetcp,cmd) "sparclite tcpX"
set gdb_target(sparclitetcp,runlist) {1 0 0 1}

# these are not target-specific
pref define gdb/load/main 1
pref define gdb/load/exit 1

proc default_port {} {
  global tcl_platform

  switch -regexp $tcl_platform(os) {
    Windows { set port com1 }
    Linux   { set port /dev/ttyS0 }
    SunOS   { set port /dev/ttya }
    AIX     { set port /dev/foo1 }
    ULTRIX  { set port /dev/foo1 }
    IRIX    { set port /dev/foo1 }
    OSF1    { set port /dev/foo1 }
    NetBSD  { set port /dev/foo1 }
    HP-UX   { 
      # Special case...
      switch -regexp $tcl_platform(osVersion) {
	A.09 { set port /dev/tty00 }
	B.10 { set port /dev/tty0p0 }
      }
    }
    default { set port /dev/ttya }
  }
  
  return $port
}

# these are target-specific
# set up the defaults
pref define gdb/load/default-verbose 0
pref define gdb/load/default-port [default_port]
pref define gdb/load/default-hostname ""
pref define gdb/load/check 0

itcl_class TargetSelection {
  # ------------------------------------------------------------------
  #  CONSTRUCTOR - create new toolbar preferences dialog
  # ------------------------------------------------------------------
  constructor {config} {
    global gdb_target gdb_target_name gdb_loaded
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

    set x [expr {[winfo screenwidth $top] / 2 - [winfo reqwidth $top] / 2}]
    set y [expr {[winfo screenheight $top] / 2 - [winfo reqheight $top] / 2}]
    wm geometry $top +$x+$y

    set target_list [get_target_list]
    set target $gdb_target_name
    if {$target == ""} {set target default}
    set defbaud $gdb_target($target,defbaud)
    pref define gdb/load/$target-baud $defbaud
    pref define gdb/load/$target-port [pref get gdb/load/default-port]
    pref define gdb/load/$target-verbose [pref get gdb/load/default-verbose]
    pref define gdb/load/$target-portname 1000
    pref define gdb/load/$target-hostname [pref get gdb/load/default-hostname]
    
    set err [catch {pref get gdb/load/$target-runlist} run_list]
    if {$err} {
      set run_list $gdb_target($target,runlist)
      pref setd gdb/load/$target-runlist $run_list
    }
    pref set gdb/src/run_attach [lindex $run_list 0]
    pref set gdb/src/run_load   [lindex $run_list 1]
    pref set gdb/src/run_run    [lindex $run_list 2]
    pref set gdb/src/run_cont   [lindex $run_list 3]

    set_saved
    
    # Trace all gdb_loaded changes based on target
    trace variable gdb_loaded w target_trace

    build_win
    wm resizable $top 0 0
    after idle [list wm deiconify $top]
  }

  # ------------------------------------------------------------------
  #  METHOD:  build_win - build the dialog
  # ------------------------------------------------------------------
  method build_win {} {
    global tcl_platform gdb_target PREFS_state gdb_ImageDir gdb_target_name

    if {$attach != 1} {
      set f [frame $this.f]
      set opts [frame $this.moreoptions]
      frame $this.moreoptionsframe
      set btns [frame $this.buttons]
    } else {
      set f $this
    }

    # LABELLEDFRAME
    Labelledframe $f.lab -anchor nw -text [gettext "Connection"]
    set fr [$f.lab get_frame]

    # target name
    label $fr.tarl -text [gettext "Target:"]
    combobox::combobox $fr.tar -editable 0 -command [list $this change_target] \
      -width $Width -maxheight 10
    if {[@@valid_target $target]} {
      # We may not have created all the entries/comboboxes yet, so
      # call this when we're idle
      after idle $fr.tar configure -value $gdb_target($target,pretty-name)
    }
    fill_targets

    # baud rate combobox
    label $fr.cbl -text [gettext "Baud Rate:"]
    combobox::combobox $fr.cb -editable 0 -command "$this change_baud" \
      -textvariable [pref varname gdb/load/$target-baud] -width $Width \
      -maxheight 10

    if {[catch {gdb_cmd "show remotebaud"} res]} {
      set baud [pref get gdb/load/$target-baud]
    } else {
      set baud [lindex $res end]
      set baud [string trimright $baud "."]
      # When uninitialized, GDB returns a baud rate of 2^32
      # Detect this and ignore it.
      if {$baud > 4000000000} {
	set baud [pref get gdb/load/$target-baud]
      } else {
	pref setd gdb/load/$target-baud $baud
      }
    }

    # host entry widget
    entry $fr.host -textvariable [pref varname gdb/load/$target-hostname] -width $Width

    # port combobox
    if {$tcl_platform(platform) == "windows"} {
      set editable 0
    } else {
      set editable 1
    }

    label $fr.portl -text [gettext "Port:"]
    combobox::combobox $fr.port -editable $editable \
      -textvariable [pref varname gdb/load/$target-port] \
      -width $Width -maxheight 10

    # load baud rates into combobox
    fill_rates

    # load port combobox
    if {$tcl_platform(platform) == "windows"} {
      foreach val [port_list] {
	$fr.port list insert end $val
      }
    } else {
      # fixme:  how do I find valid values for these????
      switch $tcl_platform(os) {
	Linux { set ports [list /dev/ttyS0 /dev/ttyS1 /dev/ttyS2 /dev/ttyS3] }
	SunOS { set ports [list /dev/ttya /dev/ttyb] }
	AIX   { set ports [list /dev/foo1 /dev/foo2] }
	ULTRIX { set ports [list /dev/foo1 /dev/foo2] }
	IRIX   { set ports [list /dev/foo1 /dev/foo2] }
	OSF1   { set ports [list /dev/foo1 /dev/foo2] }
	NetBSD { set ports [list /dev/foo1 /dev/foo2] }
	HP-UX  { 
	  # Special case...
	  switch -regexp $tcl_platform(osVersion) {
	    A.09 { set ports [list /dev/tty00 /dev/tty01] }
	    B.10 { set ports [list /dev/tty0p0 /dev/tty1p0] }
	  }
	}
	default { set ports [list UNKNOWN UNKNOWN] }
      }
      foreach val $ports {
	$fr.port list insert end $val
      }
    }

    # Port entry widget
    entry $fr.porte -textvariable [pref varname gdb/load/$target-port] -width $Width

    frame $f.fr
    checkbutton $f.fr.main -text [gettext "Run until 'main'"] \
      -variable [pref varname gdb/load/main]
    checkbutton $f.fr.exit -text [gettext "Set breakpoint at 'exit'"] \
      -variable [pref varname gdb/load/exit]
    checkbutton $f.fr.verb -text [gettext "Display Download Dialog"] \
      -variable [pref varname gdb/load/$target-verbose]

    if {[pref get gdb/mode]} {
      $f.fr.main configure -state disabled
      $f.fr.exit configure -state disabled
      $f.fr.verb configure -state disabled
      checkbutton $f.fr.check -text [gettext "Compare to remote executable"] \
         -variable [pref varname gdb/load/check]
      if { $gdb_target_name == "exec" } {
        $f.fr.check configure -state disabled
      }
    }


    grid $fr.tarl $fr.tar -sticky w -padx 5 -pady 5
    grid $fr.cbl $fr.cb -sticky w -padx 5 -pady 5
    grid $fr.portl $fr.port -sticky w -padx 5 -pady 5
    set mapped1 $fr.cb
    set mapped2 $fr.port

    change_target $gdb_target($target,pretty-name)

    grid $f.fr.main -sticky w -padx 5 -pady 5
    grid $f.fr.exit -sticky w -padx 5 -pady 5
    grid $f.fr.verb -sticky w -padx 5 -pady 5
    if {[pref get gdb/mode]} {
      grid $f.fr.check -sticky w -padx 5 -pady 5
    }

    grid $f.lab $f.fr -sticky w -padx 5 -pady 5

    # Create the "More Options" thingy
    if {[lsearch [image names] _MORE_] == -1} {
      image create photo _MORE_ -file [file join $gdb_ImageDir more.gif]
      image create photo _LESS_ -file [file join $gdb_ImageDir less.gif]
    }

    set MoreButton [button $opts.button -image _MORE_ \
		      -relief flat -command [list $this toggle_more_options]]
    set MoreLabel [label $opts.lbl -text {More Options}]
    frame $opts.frame -relief raised -bd 1
    pack $opts.button $opts.lbl -side left
    place $opts.frame -relx 1 -x -10 -rely 0.5 -relwidth 0.73 -height 2 -anchor e

    
    # Create the (hidden) more options frame
    set MoreFrame [Labelledframe $this.moreoptionsframe.frame \
		     -anchor nw -text {Run Options}]
    set frame [$MoreFrame get_frame]

    set var [pref varname gdb/src/run_attach]
    checkbutton $frame.attach -text {Attach to Target} -variable $var

    set var [pref varname gdb/src/run_load]
    checkbutton $frame.load -text {Download Program} -variable $var

    set var [pref varname gdb/src/run_cont]
    checkbutton $frame.cont -text {Continue from Last Stop} -variable $var \
      -command [list $this set_run run]
    
    set var [pref varname gdb/src/run_run]
    checkbutton $frame.run -text {Run Program} -variable $var \
      -command [list $this set_run cont]

    grid $frame.attach $frame.run -sticky w
    grid $frame.load   $frame.cont -sticky w
    grid columnconfigure $frame 0 -weight 1
    grid columnconfigure $frame 1 -weight 1

    # Map everything onto the screen
    # This looks like a possible packing bug -- our topmost frame
    # will not resize itself. So, instead, use the topmost frame.
    #pack $f $opts $this.moreoptionsframe -side top -fill x
    pack $MoreFrame -fill x -expand yes
    pack $f $opts -side top -fill x

    if {$attach != 1} {
      button $btns.ok -text [gettext OK] -width 7 -command "$this save" \
	-default active
      button $btns.cancel -text [gettext Cancel] -width 7 \
	-command "$this cancel"
      button $btns.help -text [gettext Help] -width 7 -command "$this help" \
	-state disabled
      standard_button_box $btns
      bind $btns.ok <Return> "$this save"
      bind $btns.cancel <Return> "$this cancel"
      bind $btns.help <Return> "$this help"

      pack $btns -side bottom -anchor e
      focus $btns.ok
    }
  }

  # ------------------------------------------------------------------
  #  METHOD:  set_saved - set saved values
  # ------------------------------------------------------------------
  method set_saved {} {
    global gdb_target

    set saved_baud [pref get gdb/load/$target-baud]
    set saved_port [pref get gdb/load/$target-port]
    set saved_main [pref get gdb/load/main]
    set saved_exit [pref get gdb/load/exit]
    set saved_check [pref get gdb/load/check]
    set saved_verb [pref get gdb/load/$target-verbose]
    set saved_portname [pref get gdb/load/$target-portname]
    set saved_hostname [pref get gdb/load/$target-hostname]
    set saved_attach [pref get gdb/src/run_attach]
    set saved_load   [pref get gdb/src/run_load]
    set saved_run    [pref get gdb/src/run_run]
    set saved_cont   [pref get gdb/src/run_cont]
    if {[info exists gdb_target($target,options)]} {
      if {[catch {pref get gdb/load/$target-opts} saved_options]} {
	set saved_options ""
      }
    }
  }

  # ------------------------------------------------------------------
  #  METHOD:  write_saved - write saved values to preferences
  # ------------------------------------------------------------------
  method write_saved {} {
    global gdb_target

    pref setd gdb/load/$target-baud $saved_baud
    pref setd gdb/load/$target-port $saved_port
    pref setd gdb/load/main $saved_main
    pref setd gdb/load/exit $saved_exit
    pref setd gdb/load/check $saved_check
    pref setd gdb/load/$target-verbose $saved_verb
    pref setd gdb/load/$target-portname $saved_portname
    pref setd gdb/load/$target-hostname $saved_hostname
    pref setd gdb/load/$target-runlist [list $saved_attach $saved_load $saved_run $saved_cont]
    if {[info exists gdb_target($target,options)]} {
      pref setd gdb/load/$target-opts $saved_options
    }
  }

  # ------------------------------------------------------------------
  #  METHOD:  fill_rates - fill baud rate combobox
  # ------------------------------------------------------------------
  method fill_rates {} {
    global gdb_target
    $fr.cb list delete 0 end

    if {$gdb_target($target,baud-rates) != ""} {
      foreach val $gdb_target($target,baud-rates) {
	$fr.cb list insert end $val
      }
    }
  }

  # ------------------------------------------------------------------
  #  METHOD:  fill_targets - fill target combobox
  # ------------------------------------------------------------------
  method fill_targets {} {
    global gdb_target
    #[$fr.tar subwidget listbox] delete 0 end
    $fr.tar list delete 0 end

    foreach val $target_list {
      if {[info exists gdb_target($val,pretty-name)]} {
	$fr.tar list insert end $gdb_target($val,pretty-name)
	switch $val {
	  remote {
	    # insert fake remotetcp target
	    $fr.tar list insert end $gdb_target(remotetcp,pretty-name)
	  }
	  d10v {
	    # insert fake remotetcp target
	    $fr.tar list insert end $gdb_target(d10vtcp,pretty-name)
	  }
	  m32r {
	    # insert fake remotetcp target
	    $fr.tar list insert end $gdb_target(m32rtcp,pretty-name)
	  }
	  sparclite {
	    # insert fake remotetcp target
	    $fr.tar list insert end $gdb_target(sparclitetcp,pretty-name)
	  }
	  rdp {
	    # insert fake tcp target
	    $fr.tar insert end $gdb_target(rdptcp,pretty-name)
	  }
	}
      }
    }
  }

  # ------------------------------------------------------------------
  #  METHOD:  config_dialog - Convenience method to map/unmap/rename
  #            components onto the screen based on target T.
  # ------------------------------------------------------------------
  method config_dialog {t} {
    global gdb_target

    pref define gdb/load/$t-verbose [pref get gdb/load/verbose]
    $f.fr.verb config -variable [pref varname gdb/load/$t-verbose]

    # Map the correct entries and comboboxes onto the screen
    if {$gdb_target($t,defbaud) == "TCP"} {
      # we have a tcp target
      # map host and porte
      if {$mapped1 != "$fr.host"} {
	grid forget $mapped1
	set mapped1 $fr.host
	grid $mapped1 -row 1 -column 1 -sticky w -padx 5 -pady 5
      }
      $fr.cbl configure -text "Hostname:"
      $fr.host config -textvariable [pref varname gdb/load/$t-hostname]

      if {$mapped2 != "$fr.porte"} {
	grid forget $mapped2
	set mapped2 $fr.porte
	grid $mapped2 -row 2 -column 1 -sticky w -padx 5 -pady 5
      }
      $fr.portl configure -text {Port:}
      $fr.porte config -textvariable [pref varname gdb/load/$t-portname]

      $mapped1 configure -state normal
      $mapped2 configure -state normal
    } elseif {$gdb_target($t,defbaud) != ""} {
      # we have a serial target
      # map port and cb
      if {$mapped1 != "$fr.cb"} {
	grid forget $mapped1
	set mapped1 $fr.cb
	grid $mapped1 -row 1 -column 1 -sticky w -padx 5 -pady 5
      }
      $fr.cbl configure -text "Baud Rate:"
      $fr.cb configure -textvariable [pref varname gdb/load/$t-baud]

      if {$mapped2 != "$fr.port"} {
	grid forget $mapped2
	set mapped2 $fr.port
	grid $mapped2 -row 2 -column 1 -sticky w -padx 5 -pady 5
      }
      $fr.portl configure -text {Port:}
      $fr.port configure -textvariable [pref varname gdb/load/$t-port]

      $mapped1 configure -state normal
      $mapped2 configure -state normal
    } else {
      # we have a non-remote(-like) target
      # disable all (except tar) and check for
      # options
      $mapped1 configure -state disabled
      $mapped2 configure -state disabled
      if {[info exists gdb_target($t,options)]} {
	if {$mapped1 != "$fr.host"} {
	  grid forget $mapped1
	  set mapped1 $fr.host
	  grid $mapped1 -row 1 -column 1 -sticky w -padx 5 -pady 5
	}
	$mapped1 configure -state normal
	$fr.host config -textvariable [pref varname gdb/load/$t-opts]

	# We call options "arguments" for the exec target
	# FIXME: this is really overloaded!!
	if {$t == "exec"} {
	  set text "Arguments:"
	} else {
	  set text "Options:"
	}
	$fr.cbl configure -text $text
      }
    }
  }

  # ------------------------------------------------------------------
  #  METHOD:  change_target - callback for target combobox
  # ------------------------------------------------------------------
  method change_target {w {name ""}} {
    global gdb_target

    if {$name == ""} {return}
    set target [get_target $name]
    debug "new target: $target"
    set defbaud $gdb_target($target,defbaud)
    pref define gdb/load/$target-baud $defbaud
    pref define gdb/load/$target-portname 1000
    pref define gdb/load/$target-hostname [pref get gdb/load/default-hostname]
    if {$defbaud == ""} {
      pref define gdb/load/$target-port ""
    } else {
      pref define gdb/load/$target-port [pref get gdb/load/default-port]
    }

    config_dialog $target
    fill_rates

    # Configure the default run options for this target
    set err [catch {pref get gdb/load/$target-runlist} run_list]
    if {$err} {
      set run_list $gdb_target($target,runlist)
      pref setd gdb/load/$target-runlist $run_list
    }

    pref set gdb/src/run_attach [lindex $run_list 0]
    pref set gdb/src/run_load   [lindex $run_list 1]
    pref set gdb/src/run_run    [lindex $run_list 2]
    pref set gdb/src/run_cont   [lindex $run_list 3]
    set_check_button $name
    set_saved
    set changes 0
  }

  # ------------------------------------------------------------------
  #  PRIVATE METHOD:  change_baud - called when the baud rate is changed.
  #  If GDB is running, set baud rate in GDB and read it back.
  # ------------------------------------------------------------------
  method change_baud {w {baud ""}} {
    if {$baud != ""} {
      if {[string compare $baud "TCP"] != 0} {
	gdb_cmd "set remotebaud $baud"
	if {[catch {gdb_cmd "show remotebaud"} res]} {
	  set newbaud 0
	} else {
	  set newbaud [lindex $res end]
	  set newbaud [string trimright $newbaud "."]
	  if {$newbaud > 4000000} {
	    set newbaud 0
	  }
	}
	if {$newbaud != $baud} {
	  pref set gdb/load/$target-baud $newbaud
	}
      }
    }
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
  #  METHOD:  port_list - return a list of valid ports for Windows
  # ------------------------------------------------------------------
  method port_list {} {
    set plist ""
    # Scan com1 - com8 trying to open each one.
    # If permission is denied that means it is in use,
    # which is OK because we may be using it or the user
    # may be setting up the remote target manually with
    # a terminal program.
    for {set i 1} {$i < 9} { incr i} {
      if {[catch { set fd [open COM$i: RDWR] } msg]} {
	# Failed.  Find out why.
	if {[string first "permission denied" $msg] != -1} {
	  # Port is there, but busy right now. That's OK.
	  lappend plist com$i
	}
      } else {
	# We got it.  Now close it and add to list.
	close $fd
	lappend plist com$i
      }
    }
    return $plist
  }

  # ------------------------------------------------------------------
  #  METHOD:  get_target_list - return a list of targets supported
  #  by this GDB.  Parses the output of "help target"
  # ------------------------------------------------------------------
  method get_target_list {} {
    set native [@@native_debugging]
    set names ""
    set res [gdb_cmd "help target"]
    foreach line [split $res \n] {
      if {![string compare [lindex $line 0] "target"]} {
	set name [lindex $line 1]

	# For cross debuggers, do not allow the target "exec"
	if {$name == "exec" && !$native} {
	  continue
	}
	lappend names $name
      }
    }
    return $names
  }

  # ------------------------------------------------------------------
  #  METHOD:  save - save values
  # ------------------------------------------------------------------
  method save {} {
    global gdb_target_name
    set err [catch {
      set_saved
      write_saved
      set gdb_target_name $target
      pref set gdb/load/target $target
    } errtxt]
    if {$err} {debug "target: $errtxt"}
    if {[valid_target $gdb_target_name]} {
      delete
    }
  }


  # ------------------------------------------------------------------
  #  METHOD:  cancel - restore previous values
  # ------------------------------------------------------------------
  method cancel {} {
    catch {gdb_cmd "set remotebaud $saved_baud"}
    #tixSetSilent $fr.cb $saved_baud
    $fr.cb configure -value $saved_baud
    write_saved
    if {$exportcancel} {
      global gdb_target_name
      set gdb_target_name CANCEL
    }
    delete
  }

  # ------------------------------------------------------------------
  #  METHOD: set_check_button - enable/disable compare-section command 
  # ------------------------------------------------------------------
  method set_check_button {name} {
    if {[winfo exists  $this.f.fr.check]} {
      if { $name == "exec" } {
	$this.f.fr.check configure -state disabled
      } else {
	$this.f.fr.check configure -state normal
      }
    }
  }

  # ------------------------------------------------------------------
  #  METHOD:  help - launches context sensitive help.
  # ------------------------------------------------------------------
  method help {} {
  }

  # ------------------------------------------------------------------
  #  METHOD:  config - used to change public attributes
  # ------------------------------------------------------------------
  method config {config} {}

  # ------------------------------------------------------------------
  #  METHOD:  reconfig - used when preferences change
  # ------------------------------------------------------------------
  method reconfig {} {
    # for now, just delete and recreate
    destroy $this.f
    build_win
  }

  # ------------------------------------------------------------------
  #  METHOD:  get_target - get the internal name of a target from the
  #              pretty-name
  # ------------------------------------------------------------------
  method get_target {name} {
    global gdb_target

    set t {}
    set list [array get gdb_target *,pretty-name]
    debug "list=$list"
    set i [lsearch -exact $list $name]
    if {$i != -1} {
      incr i -1
      set t [lindex [split [lindex $list $i] ,] 0]
    } else {
      debug "TargetSelection::get_target unknown pretty-name \"$name\""
    }

    debug "TARGET IS $t"
    return $t
  }

  # ------------------------------------------------------------------
  #  METHOD: toggle_more_options -- toggle displaying the  More/Fewer
  #                Options pane
  # ------------------------------------------------------------------
  method toggle_more_options {} {
    if {[$MoreLabel cget -text] == "More Options"} {
      $MoreLabel configure -text "Fewer Options"
      $MoreButton configure -image _LESS_
      # Bug in Tk? The top-most frame does not shrink...
      #pack $MoreFrame
      pack $this.moreoptionsframe -after $this.moreoptions -fill both -padx 5 -pady 5
    } else {
      $MoreLabel configure -text "More Options"
      $MoreButton configure -image _MORE_
      #pack forget $MoreFrame
      pack forget $this.moreoptionsframe
    }
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
  # PUBLIC DATA
  # 
  public attach 0
  public exportcancel 0

  #
  #  PROTECTED DATA
  #
  protected f
  protected fr
  protected target
  protected saved_baud
  protected saved_port
  protected saved_main
  protected saved_exit
  protected saved_check
  protected saved_verb
  protected saved_portname
  protected saved_hostname
  protected saved_attach
  protected saved_load
  protected saved_run
  protected saved_cont
  protected saved_options
  protected changes 0
  protected target_list ""

  # The Connection frame has three "sections"; the first contains
  # a combobox with all the targets. The second can either be
  # a combobox listing available baud rates or an entry for specifying
  # the hostname of a TCP connection. The actual widget mapped onto the
  # screen is saved in MAPPED1. The third section contains either a
  # combobox for the serial port or an entry for the portnumber. The
  # widget actually mapped onto the screen is saved in MAPPED2.
  protected mapped1
  protected mapped2

  protected Width 20
  protected MoreButton
  protected MoreFrame
  protected MoreLabel
}

# ------------------------------------------------------------------
#   PROCEDURE: target_trace
#          This procedure is used to configure gdb_loaded
#          and possible more) whenever the value of gdb_loaded
#          is changed based on the current target.
# ------------------------------------------------------------------
proc target_trace {variable index op} {
  global gdb_target_name gdb_loaded

  switch $gdb_target_name {

    exec {
      # The exec target is always loaded.
      set gdb_loaded 1
    }
  }
}

# Returns 1 if TARGET is a _runnable_ target for this gdb.
proc valid_target {target} {
  set err [catch {gdb_cmd "help target $target"}]
  if {$target == "exec" && ![native_debugging]} {
    set err 1
  }

  if {[regexp "tcp$" $target]} {
    # Special case (of course)
    regsub tcp$ $target {} foo
    return [valid_target $foo]
  }

  return [expr {$err == 0}]
}

# Returns 1 if this is not a cross debugger.
proc native_debugging {} {
  global tcl_platform

  if {$tcl_platform(os) == "SunOS" && [regexp "^5" $tcl_platform(osVersion)]} {
    set err [catch {gdb_cmd "help target procfs"}]
  } else {
    set err [catch {gdb_cmd "help target child"}]
  }
  return [expr {$err == 0}]
}
