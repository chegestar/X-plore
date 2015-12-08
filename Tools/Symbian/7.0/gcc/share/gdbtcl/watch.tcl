#
# WatchWin
# ----------------------------------------------------------------------
# Implements watch windows for gdb. Inherits the VariableWin
# class from variables.tcl. 
# ----------------------------------------------------------------------
#   AUTHOR:  Keith Seitz <keiths@cygnus.com>
#   Copyright (C) 1997, 1998 Cygnus Solutions
#

itcl_class WatchWin {
  inherit VariableWin

  # ------------------------------------------------------------------
  #  CONSTRUCTOR - create new locals window
  # ------------------------------------------------------------------
  constructor {config} {
    set Sizebox 0

    # Only allow one watch window for now...
    if $init {
      VariableWin@@constructor
      set init 0
    }
  }

  # ------------------------------------------------------------------
  # METHOD: build_win - build window for watch. This supplants the 
  #         one in VariableWin, so that we can add the entry at the
  #         bottom.
  # ------------------------------------------------------------------
  method build_win {f} {
    global tcl_platform
    #debug "WatchWin::build_win $f"

    set Menu [build_menu_helper Watch]
    $Menu add command -label Remove -underline 0 \
      -command [format {
	%s remove [%s getSelection]
      } $this $this]

    set f [@@frame $f.f]
    set treeFrame  [frame $f.top]
    set entryFrame [frame $f.expr]
    VariableWin@@build_win $treeFrame
    set Entry [entry $entryFrame.ent -font src-font]
    button $entryFrame.but -text "Add Watch" -command "$this validateEntry"
    pack $f -fill both -expand yes
    grid $entryFrame.ent -row 0 -column 0 -sticky news -padx 2
    grid $entryFrame.but -row 0 -column 1 -padx 2
    grid columnconfigure $entryFrame 0 -weight 1
    grid columnconfigure $entryFrame 1

    if {$tcl_platform(platform) == "windows"} {
      grid columnconfigure $entryFrame 1 -pad 20
      ide_sizebox $this.sizebox
      place $this.sizebox -relx 1 -rely 1 -anchor se
    }

    grid $treeFrame -row 0 -column 0 -sticky news
    grid $entryFrame -row 1 -column 0 -padx 5 -pady 5 -sticky news
    grid columnconfigure $f 0 -weight 1
    grid rowconfigure $f 0 -weight 1
    update
    # Binding for the entry
    bind $entryFrame.ent <Return> "$entryFrame.but invoke"
  }

  method selectionChanged {entry} {
    VariableWin@@selectionChanged $entry

    set state disabled
    set entry [getSelection]
    foreach var $Watched {
      set name [lindex $var 0]
      if {"$name" == "$entry"} {
	set state normal
	break
      }
    }

    $Menu entryconfigure last -state $state
  }

  method validateEntry {} {
    if !$Running {
      set variable [$Entry get]
      set ok [add $variable]
      
      $Entry delete 0 end
    }
  }

  # ------------------------------------------------------------------
  # DESTRUCTOR - delete watch window
  # ------------------------------------------------------------------
  destructor {
    VariableWin@@destructor
  }

  method postMenu {X Y} {
#    debug "WatchWin::postMenu $x $y"

    set entry [getEntry $X $Y]
    
    # Disable "Remove" if we are not applying this to the parent
    set found 0
    foreach var $Watched {
      set name [lindex $var 0]
      if {"$name" == "$entry"} {
	set found 1
	break
      }
    }

    # Ok, nasty, but a sad reality...
    set noStop [catch {$Popup index "Remove"} i]
    if !$noStop {
      $Popup delete $i
    }
    if $found {
      $Popup add command -label "Remove" -command "$this remove \{$entry\}"
    }

    VariableWin@@postMenu $X $Y
  }

  method remove {entry} {
    global Display Update

    foreach var $Watched {
      set name [lindex $var 0]
      if {"$name" == "$entry"} {
	set gdbName [lindex $var 1]
	break
      }
    }

    # Remove this entry from the list of watched variables
    set i [lsearch -exact $Watched [list $name $gdbName]]
    if {$i == -1} {
      debug "WHAT HAPPENED?"
    }
    set Watched [lreplace $Watched $i $i]    

    set list [$Hlist info children $entry]
    lappend list $entry
    $Hlist delete entry $entry

    foreach entry $list {
      $Variables($entry) delete
      unset Variables($entry)
      unset Values($entry)
      unset Types($entry)
      unset Display($this,$entry)
      unset Update($this,$entry)
    }
  }

  # ------------------------------------------------------------------
  # METHOD: getVariablesBlankPath
  # Overrides VarialbeWin::getVariablesBlankPath. For a Watch Window,
  # this method returns a list of watched variables.
  #
  # ONLY return items that need to be added to the Watch Tree
  # (or use deleteTree)
  # ------------------------------------------------------------------
  method getVariablesBlankPath {} {
#    debug "WatchWin::getVariablesBlankPath"
    set list {}

    set variables [displayedVariables {}]
    foreach var $variables {
      set name [lindex $var 0]
      set on($name) 1
    }
    
    foreach var $Watched {
      set name [lindex $var 0]
      set gdbName [lindex $var 1]
      if {![info exists on($name)]} {
	lappend list [list $name $gdbName]
      }
    }
    
    return $list
  }

  method update {} {
    global Update Display
    debug "START WATCH UPDATE CALLBACK"
    set locals [getLocals]
    if {"$locals" != "$Locals"} {
      # Context switch, delete the tree and repopulate it
      deleteTree
      set Locals $locals
    }

    populate {}
    previous update

    # Make sure all variables are marked as _not_ Openable?
    debug "END WATCH UPDATE CALLBACK"
  }

  # ------------------------------------------------------------------
  # METHOD:   label - used to label the entries in the tree
  # ------------------------------------------------------------------
  method label {name {noValue 0}} {
    #debug "WatchWin::label \"$name\""

    # Ok, this is as hack as it gets
    set blank "                                                                                                                                                             "
    # Use protected data Length to determine how big variable
    # name should be. This should clean the display up a little
    set displayName [$Variables($name) name]
    regsub -all -- {->} $displayName {~} n
    regsub -all -- - $n . displayName
    regsub -all -- {~} $displayName {->} n
    
    set indent [llength [split $name .]]
    set indent [expr {$indent * $Length}]
    set len [string length $n]
    set l [expr {$EntryLength - $len - $indent}]
    regsub -all % $n \$ displayName
    set label "$displayName[string range $blank 0 $l]"
    set val " $Values($name)"
    #debug "label=$label"
    if $noValue {
      return $label
    }
    return "$label $val"
  }

  method showMe {} {
    debug "Watched: $Watched"
  }

  # ------------------------------------------------------------------
  # METHOD: add - add a variable to the watch window
  # ------------------------------------------------------------------
  method add {name} {

    # We must make sure that we make convert the variable (structs, in
    # particular) to a more acceptable form for the entry. For example,
    # if we want to watch foo.bar, we need to make sure this is entered
    # as foo-bar (or else we will be missing the parent "foo").
#    set var [string trim $name \ \r\n\;]
#    regsub -all \\. $var - name
 
    # Strip all the junk after the first \n
    set var [split $name \n]
    set var [lindex $var 0]
    set var [split $var =]
    set var [lindex $var 0]

    # Strip out leading/trailing +, -, ;, spaces, commas
    set var [string trim $var +-\;\ \r\n,]

    # Make sure that we have a valid variable
    set err [catch {gdb_cmd "set variable $var"} errTxt]
    if $err {
      #debug "ERROR adding variable: $errTxt"
    } else {
      if {"[string index $var 0]" == "\$"} {
	# We must make a special attempt at verifying convenience
	# variables.. Specifically, these are printed as "void"
	# when they are not defined. So if a user type "$_I_made_tbis_up",
	# gdb responds with the value "void" instead of an error
	catch {gdb_cmd "p $var"} msg
	set msg [split $msg =]
	set msg [string trim [lindex $msg 1] \ \r\n]
	if {"$msg" == "void"} {
	  return 0
	}

	regsub -all \\. $var - a
	regsub -all \\$ $a % name
      } else {
	regsub -all \\. $var - name
      }

      # make one last attempt to get errors
      set err [catch {set foo($name) 1}]
      set err [expr {$err + [catch {expr $foo($name) + 1}]}]
      if !$err {
	if {[lsearch -exact $Watched [list $name $var]] == -1} {
	  lappend Watched [list $name $var]
	  update
	  return 1
	}
      }
    }

    return 0
  }

  protected Entry
  protected Watched {}
  protected Menu {}
  common init 1
}
