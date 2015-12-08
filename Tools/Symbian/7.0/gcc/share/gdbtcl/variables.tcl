#
# VariableWin
# ----------------------------------------------------------------------
# Implements variable windows for gdb. LocalsWin and WatchWin both
# inheirit from this class. You need only override the method 
# 'getVariablesBlankPath' and a few other things...
# ----------------------------------------------------------------------
#   AUTHOR:  Keith Seitz <keiths@cygnus.com>
#   Copyright (C) 1997, 1998 Cygnus Solutions
#

itcl_class VariableWin {
  protected Sizebox 1

  # ------------------------------------------------------------------
  #  CONSTRUCTOR - create new watch window
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
    virtual build_win $this
    gdbtk_idle

    add_hook gdb_update_hook "$this update"
    add_hook gdb_busy_hook "$this disable_ui"
    add_hook gdb_no_inferior_hook "$this no_inferior"
    add_hook gdb_idle_hook [list $this idle]

    after idle [list wm deiconify $top]
  }

  # ------------------------------------------------------------------
  #  METHOD:  build_win - build the watch window
  # ------------------------------------------------------------------
  method build_win {f} {
    global tixOption tcl_platform
    #    debug "VariableWin::build_win"
    set width [font measure src-font "W"]
    # Choose the default width to be...
    set width [expr {40 * $width}]
    if {$tcl_platform(platform) == "windows"} {
      set scrollmode both
    } else {
      set scrollmode auto
    }
    set Tree [tixTree $f.tree        \
		-opencmd  "$this open"  \
		-closecmd "$this close" \
		-ignoreinvoke 1         \
		-width $width           \
		-browsecmd [list $this selectionChanged] \
		-scrollbar $scrollmode \
		-sizebox $Sizebox]
    if {![pref get gdb/mode]} {
      $Tree configure -command [list $this editEntry]
    }
    set Hlist [$Tree subwidget hlist]

    # FIXME: probably should use columns instead.
    $Hlist configure -header 1

    set l [expr {$EntryLength - $Length - [string length "Name"]}]
    # Ok, this is as hack as it gets
    set blank "                                                                                                                                                             "
    $Hlist header create 0 -itemtype text \
      -text "Name[string range $blank 0 $l]Value"

    # Configure the look of the tree
    set sbg [$Hlist cget -bg]
    set fg [$Hlist cget -fg]
    set bg $tixOption(input1_bg)
    set width [font measure src-font $LengthString]
    $Hlist configure -indent $width -bg $bg \
      -selectforeground $fg -selectbackground $sbg \
      -selectborderwidth 0 -separator @ -font src-font

    # Get display styles
    set normal_fg    [$Hlist cget -fg]
    set highlight_fg [pref get gdb/variable/highlight_fg]
    set disabled_fg  [pref get gdb/variable/disabled_fg]
    set NormalTextStyle [tixDisplayStyle text -refwindow $Hlist \
			   -bg $bg -fg $normal_fg -font src-font]
    set HighlightTextStyle [tixDisplayStyle text -refwindow $Hlist \
			      -bg $bg -fg $highlight_fg -font src-font]
    set DisabledTextStyle [tixDisplayStyle text -refwindow $Hlist \
			     -bg $bg -fg $disabled_fg -font src-font]

    if [catch {gdb_cmd "show output-radix"} msg] {
      set Radix 10
    } else {
      regexp {[0-9]+} $msg Radix
    }


    # Update the tree display
    virtual update
    after idle pack $Tree -expand yes -fill both

    # Create the popup menu for this widget
    bind $Hlist <3> "$this postMenu %X %Y"

    # Do not use the tixPopup widget... 
    set Popup [menu $f.menu -tearoff 0]
    set disabled_foreground [$Popup cget -foreground]
    $Popup configure -disabledforeground $disabled_foreground
    set ViewMenu [menu $Popup.view]
    $Popup add command -label "dummy" -state disabled
    $Popup add separator
    $Popup add cascade -label "Format" -menu $ViewMenu
#    $Popup add checkbutton -label "Auto Update"
#    $Popup add command -label "Update Now"
    if {![pref get gdb/mode]} {
      $Popup add command -label "Edit"
    }

    # Make sure to update menu info.
    selectionChanged ""
  }

  # ------------------------------------------------------------------
  #  DESTRUCTOR - destroy window containing widget
  # ------------------------------------------------------------------
  destructor {
    #    debug "VariableWin::destructor"
    # Make sure to clean up all the variables
    foreach var [array names Variables] {
      $Variables($var) delete
    }
    
    # Delete the display styles used with this window
    destroy $NormalTextStyle
    destroy $HighlightTextStyle
    destroy $DisabledTextStyle

    # Remove this window and all hooks
    remove_hook gdb_update_hook "$this update"
    remove_hook gdb_busy_hook "$this disable_ui"
    remove_hook gdb_no_inferior_hook "$this no_inferior"
    remove_hook gdb_idle_hook [list $this idle]

    set top [winfo toplevel $this]
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
    #    debug "VariableWin::reconfig"
    destroy $this.tree
    destroy $this.menu
    build_win
  }

  # ------------------------------------------------------------------
  #  METHOD:  build_menu_helper - Create the menu for a subclass.
  # ------------------------------------------------------------------
  method build_menu_helper {first} {
    menu $this.mmenu

    $this.mmenu add cascade -label $first -underline 0 -menu $this.mmenu.var

    menu $this.mmenu.var
    if {![pref get gdb/mode]} {
      $this.mmenu.var add command -label Edit -underline 0 -state disabled \
	  -command [format {
	%s editEntry [%s getSelection]
      } $this $this]
      }
    $this.mmenu.var add cascade -label Format -underline 0 \
      -menu $this.mmenu.var.format

    menu $this.mmenu.var.format
    foreach label {Hex Decimal Binary Octal} fmt {x d t o} {
      # The -variable is set when a selection is made in the tree.
      $this.mmenu.var.format add radiobutton -label $label -underline 0 \
	-value $fmt \
	-command [format {
	  %s setDisplay [%s getSelection] %s
	} $this $this $fmt]
    }

#    $this.mmenu add cascade -label Update -underline 0 -menu $this.mmenu.update
#    menu $this.mmenu.update

    # The -variable is set when a selection is made in the tree.
#    $this.mmenu.update add checkbutton -label "Auto Update" -underline 0 \
#      -command [format {
#	%s toggleUpdate [%s getSelection]
#      } $this $this]
#    $this.mmenu.update add command -label "Update Now" -underline 0 \
#      -accelerator "Ctrl+U" -command [format {
#	%s updateNow [%s getSelection]
#      } $this $this]

    set top [winfo toplevel $this]
    $top configure -menu $this.mmenu
    bind_plain_key $top Control-u [format {
      if {!$Running} {
	if {[%s getSelection] != ""} {
	  %s updateNow [%s getSelection]
	}
      }
    } $this $this $this]

    return $this.mmenu.var
  }

  # Return the current selection, or the empty string if none.
  method getSelection {} {
    return [$Hlist info selection]
  }

  # This is called when a selection is made.  It updates the main
  # menu.
  method selectionChanged {name} {

    if $Running {
      # Clear the selection, too
      $Hlist selection clear
      return
    }

    # if something is being edited, cancel it
    if {[info exists EditEntry]} {
      UnEdit
    }

    if {$name == ""} {
      set state disabled
    } else {
      set state normal
    }

    foreach menu [list $this.mmenu.var $this.mmenu.var.format ] {
      set i [$menu index last]
      while {$i >= 0} {
	if {[$menu type $i] != "cascade"} {
	  $menu entryconfigure $i -state $state
	}
	incr i -1
      }
    }

    if {$name != ""
	&& ! [$Variables($name) isOpenable]
	&& ! [$Variables($name) isFunction]} {
      set state normal
    } else {
      set state disabled
    }
    foreach label {Hex Decimal Binary Octal} {
      $this.mmenu.var.format entryconfigure $label \
	-variable Display($this,$name)
      if {$label != "Hex"} {
	$this.mmenu.var.format entryconfigure $label -state $state
      }
    }
#    $this.mmenu.update entryconfigure 0 -variable Update($this,$name)
  }

  method updateNow {entry} {
    global Display
     # debug "VariableWin::updateNow $entry"

    if !$Running {
      set Values($entry) [$Variables($entry) value $Display($this,$entry)]
      set text [virtual label $entry]
      $Hlist entryconfigure $entry -itemtype text -text $text
    }
  }

  method getEntry {x y} {
    set realY [expr {$y - [winfo rooty $Hlist]}]

    # Get the tree entry which we are over
    return [$Hlist nearest $realY]
  }

  method editEntry {entry} {
    if {!$Running} {
      if {$entry != ""} {
	edit $entry
      }
    }
  }

  method postMenu {X Y} {
    global Display
    global Update
    #    debug "VariableWin::postMenu"

    # Quicky for menu posting problems.. How to unpost and post??

    if {[winfo ismapped $Popup] || $Running} {
      return
    }

    set entry [getEntry $X $Y]
    if {[string length $entry] > 0} {
      #      debug "$Display($this,$entry)=$Display($this,$entry)"
      
      # Configure menu items
      # the title is always first..
      #set labelIndex [$Popup index "dummy"]
      set viewIndex [$Popup index "Format"]
#      set autoIndex [$Popup index "Auto Update"]
#      set updateIndex [$Popup index "Update Now"]
      set noEdit [catch {$Popup index "Edit"} editIndex]

      # Retitle and set update commands
      $Popup entryconfigure 0 -label "[virtual label $entry 1]"
#      $Popup entryconfigure $autoIndex -command "$this toggleUpdate \{$entry\}" \
	-variable Update($this,$entry) 
#      $Popup entryconfigure $updateIndex -command "$this updateNow \{$entry\}"

      # Edit pane
      if {[$Variables($entry) isEditable]} {
	if !$noEdit {
	  $Popup delete $editIndex
	}
	if {![pref get gdb/mode]} {
	  $Popup  add command -label Edit -command "$this edit \{$entry\}"
	}
      } else {
	if !$noEdit {
	  $Popup delete $editIndex
	}
      }

      # Erase any existing menu entries
      $ViewMenu delete 0 end

      # Populate the view menu
#      if {![$Variables($entry) isStruct] && ![$Variables($entry) isUnion]} {
	$ViewMenu add radiobutton -label "Hex"       \
	  -variable Display($this,$entry)            \
	  -value x                                   \
	  -command "$this setDisplay \{$entry\} x"
#      }

      # Change to isPointer?
      if {![$Variables($entry) isOpenable]
	  && ![$Variables($entry) isFunction]} {
	$ViewMenu add radiobutton -label "Decimal" \
	  -variable Display($this,$entry)          \
	  -value d                                 \
	  -command "$this setDisplay \{$entry\} d"
	$ViewMenu add radiobutton -label "Binary"  \
	  -variable Display($this,$entry)          \
	  -value t                                 \
	  -command "$this setDisplay \{$entry\} t"
	$ViewMenu add radiobutton -label "Octal"   \
	  -variable Display($this,$entry)          \
	  -value o                                 \
	  -command "$this setDisplay \{$entry\} o"
      }

      tk_popup $Popup $X $Y
    }
  }

  # ------------------------------------------------------------------
  # METHOD edit -- edit a variable
  # ------------------------------------------------------------------
  method edit {entry} {
    global Display
    global Update tixOption

    # disable menus
    selectionChanged ""

    set fg   [$Hlist cget -foreground]
    set bg   [$Hlist cget -background]

    if {"$Editing" == ""} {
      # Must create the frame
      set Editing [frame $Hlist.frame -bg $bg -bd 0 -relief flat]
      set lbl [@@label $Editing.lbl -fg $fg -bg $bg -font src-font]
      set ent [entry $Editing.ent -bg $tixOption(bg) -font src-font]
      pack $lbl $ent -side left
    }

    if {[info exists EditEntry]} {
      # We already are editing something... So reinstall it first
      # I guess we discard any changes?
      UnEdit
    }

    # Update the label/entry widgets for this instance
    set Update($this,$entry) 1
    set EditEntry $entry
    set label [virtual label $entry 1];	# do not append value
    $Editing.lbl configure -text "$label  "
    $Editing.ent delete 0 end

    # Strip the pointer type, text, etc, from pointers, and such
    set val [$Variables($entry) value $Display($this,$entry) 1]
    $Editing.ent insert 0 $val

    # Find out what the previous entry is
    set prev [$Hlist info prev $entry]
    set data [$Hlist info data $entry]
    set parent [$Hlist info parent $entry]
    close $entry
    $Hlist delete entry $entry
    if {"$prev" != ""} {
      set previous "-after $prev"
    } else {
      # this is the first!
      set previous "-at 0"
    }
    
    if {"$prev" == "$parent"} {
      # This is the topmost-member of a sub-grouping..
      set previous "-at 0"
    }

    set cmd [format { \
      %s add {%s} %s -itemtype window -window %s -data {%s} \
    } $Hlist $entry $previous $Editing $data]
    eval $cmd

    if {[$Variables($entry) isOpenable]} {
      $Tree setmode $entry open
    }

    # Set focus to entry
    focus $Editing.ent
    $Editing.ent selection to end

    # Setup key bindings
    bind $Editing.ent <Return> "$this changeValue"
    bind $Hlist <Return> "$this changeValue"
    bind $Editing.ent <Escape> "$this UnEdit"
    bind $Hlist <Escape> "$this UnEdit"
  }
  
  method UnEdit {} {
    set prev [$Hlist info prev $EditEntry]
    set data [$Hlist info data $EditEntry]
    set parent [$Hlist info parent $EditEntry]
    
    if {"$prev" != ""} {
      set previous "-after $prev"
    } else {
      set previous "-at 0"
    }
    if {"$prev" == "$parent"} {
      # This is the topmost-member of a sub-grouping..
      set previous "-at 0"
    }
    
    $Hlist delete entry $EditEntry
    set cmd [format {\
       %s add {%s} %s -itemtype text -text {%s} -data {%s} \
    } $Hlist $EditEntry $previous [virtual label $EditEntry] $data]
    eval $cmd
    if {[$Variables($EditEntry) isOpenable]} {
      $Tree setmode $EditEntry open
    }
    
    # Unbind
    bind $Hlist <Return> {}
    bind $Hlist <Escape> {}
    if {$Editing != ""} {
      bind $Editing.ent <Return> {}
      bind $Editing.ent <Escape> {}
    }
    
    unset EditEntry
    selectionChanged ""
  }

  method changeValue {} {
    global Display
    # Get the old value
    #   set old [$Variables($EditEntry) value $Display($this,$EditEntry) 1]
    set new [string trim [$Editing.ent get] \ \r\n]
    if {$new == ""} {
      UnEdit
      return
    }

    if {[catch {gdb_cmd "set variable [$Hlist info data $EditEntry]=$new"} errTxt]} {
      tk_messageBox -icon error -type ok -message $errTxt \
	-title "Error in Expression" -parent $this
      focus $Editing.ent
      $Editing.ent selection to end
    } else {
      set Values($EditEntry) [$Variables($EditEntry) value $Display($this,$EditEntry)]
      UnEdit
      
      # Get rid of entry... and replace it with new value
      focus $Tree
    }
  }

  method toggleUpdate {entry} {
    global Update Display

    if $Update($this,$entry) {
      # Must update value
      set Values($entry) [$Variables($entry) value $Display($this,$entry)]
      $Hlist entryconfigure $entry \
	-style $NormalTextStyle    \
	-text [virtual label $entry]
    } else {
      $Hlist entryconfigure $entry \
	-style $DisabledTextStyle
    }
    @@update
  }

  method setDisplay {entry type} {
    global Display
        # debug "VariableWin::setDisplay $entry $type"
    if !$Running {
      set Display($this,$entry) $type
      set Values($entry) [$Variables($entry) value $Display($this,$entry)]
      $Hlist entryconfigure $entry -text [virtual label $entry]
    }
  }

  # ------------------------------------------------------------------
  # METHOD:   label - used to label the entries in the tree
  # ------------------------------------------------------------------
  method label {name {noValue 0}} {
    #    debug "VariableWin::label"

    # Ok, this is as hack as it gets
    set blank "                                                                                                                                                             "
    # Use protected data Length to determine how big variable
    # name should be. This should clean the display up a little
    set n [$Variables($name) name]
    set indent [llength [split $name @]]
    set indent [expr {$indent * $Length}]
    set len [string length $n]
    set l [expr {$EntryLength - $len - $indent}]
    set label "$n[string range $blank 0 $l]"
    set val " $Values($name)"
    #debug "label=$label"
    if $noValue {
      return $label
    }
    return "$label $val"
  }

  # ------------------------------------------------------------------
  # METHOD:   open - used to open an entry in the variable tree
  # ------------------------------------------------------------------
  method open {path} {
    global Update
    # We must lookup all the variables for this struct
    #    debug "VariableWin::open $path"

    if !$Running {
      # Do not open disabled paths
      if $Update($this,$path) {
	populate $path
      }
    } else {
      $Tree setmode $path open
    }
  }

  # ------------------------------------------------------------------
  # METHOD:   close - used to close an entry in the variable tree
  # ------------------------------------------------------------------
  method close {path} {
    global Display Update
    #    debug "VariableWin::close"
    # Close the path and destroy all the entry widgets
    #    debug "CLOSE: $path"

    if {!$Running} {
      # Only update when we we are not disabled
      if $Update($this,$path) {
	set variables [displayedVariables $path]
	#      debug "variables=$variables"
	
	# Delete the offspring of this entry
	$Hlist delete offspring $path
	
	# Delete protected/global data
	foreach var $variables {
	  catch {
	    $Variables($var) delete
	    unset Variables($var)
	    unset Types($var)
	    unset Values($var)
	    unset Display($this,$var)
	    unset Update($this,$var)
	  }
	  #      debug "destroyed $var"
	}
      }
    } else {
      $Tree setmode $path close
    }
  }

  # ------------------------------------------------------------------
  # METHOD:   getVariables - get the variable list for a given path
  # ------------------------------------------------------------------
  method getVariables {path} {
    global Display
    global Update
    #    debug "VariableWin::getVariables"
    set variables [getPath $path]
    #    debug "getVariables got: $variables"

    # We need to setup the protected members...
    set varList {}
    foreach variable $variables {
      set var [lindex $variable 0]
      set gdb [lindex $variable 1]
      if ![info exists Variables($var)] {
	set err [catch {Variable $this.var.$var $var $gdb} obj]
	if $err {
	  continue
	}
	lappend varList $variable

    switch $Radix {
       8  { set fmt o }
       10 { set fmt d }
       16 { set fmt x }
    }

	if {[$obj displayHex]} {
	  set Display($this,$var) x
	} else {
	  set Display($this,$var) $fmt
	}

	set Variables($var) $obj
	set Values($var)    [$obj value $Display($this,$var)]
	set Types($var)     [$obj type]
	set Update($this,$var) 1
      }
    }

    #    debug "Variables: [array names Variables]"
    return $varList
  }

  method isVariable {var} {

    set err [catch {gdb_cmd "output $var"} msg]
    if {$err 
	|| [regexp -nocase "no symbol|syntax error" $msg]} {
      return 0
    }

    return 1
  }

  # ------------------------------------------------------------------
  # METHOD:  getAllClassMembers - YUCK!
  # ------------------------------------------------------------------
  method getAllClassMembers {entry gdbName} {
    catch {var delete}
    Variable var var $gdbName

    #puts "getAllClassMembers \"$gdbName\""
    set classInfo [var classInfo]
    set first [string first : $classInfo]
    if {$first == -1} {
      # This class does not inherit from any other class
      return {}
    }
    
    incr first
    set classInfo [string range $classInfo $first end]
    set classes [split $classInfo ,]
    #puts "CLASSES=$classes"
    set variables {}
    foreach c $classes {
      set class [lindex $c end]
      set class [string trim $class \ \r\n\t]
      #puts "Class: \"$class\""
      update
      set variables [concat $variables [getPath $entry "(($class)$gdbName)"]]
    }

    #puts "RETURNING: $variables"
    return $variables
  }

  # ------------------------------------------------------------------
  # METHOD:  getPath - WARNING! DANGER, WILL ROBINSON!
  # ------------------------------------------------------------------  
  method getPath {entry {gdbName {}}} {
    #puts "VarialbeWin::getPath \"$entry\" \"$gdbName\""

    if {"$entry" == ""} {
      set variables [virtual getVariablesBlankPath]
    } else {
      #
      # Want either a pointer or a struct/class/union
      #
      set hack 0
      if {"$gdbName" == ""} {
	set gdbName [$Hlist info data $entry]
	set hack 1
      }

      set cmd "ptype $gdbName"
      #debug "VariableWin::getPath cmd = $cmd"
      set err [catch {gdb_cmd $cmd} result]
      if $err {
	#	debug stderr "VariableWin::getPath ERROR: $result"
	return {}
      }
      #debug "RESULT=$result"
      # DO NOT :set result [split $result]
      #
      # The msg will always look like this:
      # lindex    value
      # 0         "type" ALWAYS
      # 1         "="    ALWAYS
      # 2         type (int, double, char) or struct, union, class
      # 3         if not struct/union/class, could be *
      #           if struct/union/class, name of struct/union
      # 4         members of struct/union or none if not struct/union/class
      # 5         *'s (indicating pointer, etc.)
      
      # This whole thing dies when we have an unamed union or struct, i.e.,
      # struct _foo {
      #   union {
      #     int a;
      #     int b;
      #   } u1;
      #   int c;
      # } foo;
      # In this case, this all fails because gdb outputs the members of u1, too.
      
      set varObject $Variables($entry)
      if {[$varObject isClass] || [$varObject isClassPtr]} {
	set class 1
      } else {
	set class 0
      }
      if {[$varObject isStruct]   || [$varObject isStructPtr]
	  || [$varObject isUnion] || $class} {
#	if [$varObject isUnamed] {
	  #debug "UNAMED"
#	  set a 3
#	} else {
#	  set a 4
#	}
#	set members [lindex $result $a]
#	set members [split $members \n]
	set first [string first \{ $result]
	set last [string last \} $result]
	incr first
	incr last -1
	set members [string range $result $first $last]
	set members [split $members \n]
	#debug "members of $gdbName:"
	#foreach a $members {
	#  debug $a
	#}

	set variables {}
	if $class {
	  if {$hack && [$varObject isClassPtr]} {
	    set n "(*$gdbName)"
	  } else {
	    set n $gdbName
	  }
	  set variables [getAllClassMembers $entry $n]
	}

	set len [llength $members]
	# The first member is always blank
	for {set i 1} {$i <= $len} {incr i} {
	  set line [lindex $members $i]
	  set line [string trim $line \ \r\n]

	  # This is a two-part hack (that won't work in some cases)
	  # We check if there is a blank line in the output or
	  # if there is a ~. GDB always puts a space between
	  # data and methods and always lists any destructor first.
	  # If we see any of these two conditions, we stop.
	  if {$class && "$line" == ""} {
	    break
	  } elseif {$class && [regexp ~ $line]} {
	    break
	  }

	  # Skip blanks, "public:" and other C++ labels, and
	  # anything that has a period in it (needed for some C++ output --
	  # virtual tables or something?)
	  if {"$line" == "" || "$line" == "public:" 
	      || "$line" == "protected:" || "$line" == "private:"
	      || [regexp \\. $line]} {
	    continue
	  }

	  #puts "LINE: $line"
	  # The class destructor is always listed first, so look for it
	  # SPECIAL CASE: Unamed structs/unions
	  if {[regexp -- "^struct \{" $line]
	      || [regexp "^union \{" $line]} {
	    if {[regexp "struct" $line]} {
	      set type "struct"
	    } else {
	      set type "union"
	    }
	    set braces 0
	    for {} {$i <= $len} {incr i} {
	      set line [lindex $members $i]
	      #debug "LINE*: $line"
	      set line [string trim $line \ \r\n\;]
	      set open [regsub -all -- \{ $line a foo]
	      set clos [regsub -all -- \} $line a foo]
	      set braces [expr {$braces + $open - $clos}]
	      #debug "Braces = $braces"
	      if {$braces == 0} {
		# We must also strip off things like array subscripts...
		set stop [string last \[ $line]
		if {$stop != -1} {
		  incr stop -1
		  set line [string range $line 0 $stop]
		}
		set n [string trim $line \ \r\n\;\}]
		#debug "UNAMED $type: $n"
		break
	      }
	    }
	  } else {
	    # We must also strip off things like array subscripts...
	    set stop [string first \[ $line]
	    if {$stop != -1} {
	      incr stop -1
	      set line [string range $line 0 $stop]
	    }
	    set line [string trim $line \ \r\n\;]

	    set type [lindex $line 0]
	    if {"$type" == "struct" || "$type" == "union"} {
	      set n [string trim [lindex $line 2] \ \r\n\\*]
	    } else {
	      set na [split $line]
	      set n [lindex $na end]
	      set n [string trim $n \ \r\n\\*]
	    }
	  }
	  
	  if {$hack && [$Variables($entry) isPointer]} {
	    lappend variables [list $entry@$n (*$gdbName).$n]
	  } else {
	    lappend variables [list $entry@$n $gdbName.$n]
	  }
	}
      } else {
	# We have a pointer ? So dereference it..
	set list [split $entry @]
	set name [lindex $list end];	# [expr [llength $list] - 1]
	set variables [list [list "$entry@*$name" "(*($gdbName))"]]
      }
    }
    
    #    debug "getPath returning \"$variables\""
    return $variables
  }

  # OVERRIDE THIS METHOD
  method getVariablesBlankPath {} {
    debug "You forgot to override getVariablesBlankPath!!"
    return {}
  }

  method cmd {cmd} {
    eval $cmd
  }
  
  # ------------------------------------------------------------------
  # METHOD:   populate - populate an entry in the tree
  # ------------------------------------------------------------------
  method populate {path} {
    #    debug "VariableWin::populate"
    # NEED TO GO FROM variables/path to Variables (objects)
    set variables [getVariables $path]

    foreach variable $variables {
      set name [lindex $variable 0]
      set gdbName [lindex $variable 1]

      #      debug "populate: name=\"$name\", gdbName=\"$gdbName\""
      set dotPath  [$Variables($name) pathname .]
      $Hlist add $dotPath           \
	-itemtype text              \
	-text [virtual label $name] \
	-data $gdbName
      if {[$Variables($name) isOpenable]} {
	# Make sure we get this labeled as openable
	$Tree setmode $dotPath open
      }
    }
  }
  

  method changed {var} {
    global Display Update

    if $Update($this,$var) {
      set oldValue $Values($var)
      set newValue [$Variables($var) value $Display($this,$var)]
      set new $newValue
      
      # Ignore the string in char *'s...
      # But what if we want to know when its changed??? The display
      # will not get upadted...
      #      if {"$Types($var)" == "char *"} {
      #	debug "oldValue = $oldValue\nnewValue = $newValue"
      #	set first [string first \) $oldValue]
      #	incr first
      #	set oldValue [string range $oldValue $first end]
      #	set oldValue [lindex $oldValue 0]
      #	
      #	set first [string first \) $newValue]
      #	incr first
      #	set new [string range $newValue $first end]
      #	set new [lindex $new 0]
      #
      #	debug "char *: old=\"$oldValue\", new=\"$new\""
      #      }

      if {"$new" != "$oldValue"} {
	return $newValue
      }
    }

    return {}
  }

  method getLocals {} {
    # Get all current locals
    set pc [lindex [gdb_loc] 4]
    set vars [gdb_get_args *$pc]
    set vars [concat $vars [gdb_get_locals *$pc]]

    set variables {}
    foreach var $vars {
      lappend variables [list $var $var]
    }

    #debug "--getLocals:\n$variables\n--getLocals"
    return $variables
  }

  method old_getLocals {} {
    # Get all current locals
    set err [catch {gdb_cmd "info locals"} r]
    if $err {
      # Append error to console window
      #      debug "ERROR: $r"
      return {}
    }
    if {[regexp -nocase "no locals" $r]} {
      # No locals here
      set r {}
    }
    
    set err [catch {gdb_cmd "info args"} result]
    if $err {
      # Append error to console window
      #      puts "ERROR: $result"
      return {}
    }
    if {[regexp -nocase -- "no arguments" $result]} {
      set result [split "$r" \n]
    } else {
      set result [split "$result$r" \n]
    }
    
    set variables {}
    foreach line $result {
      #debug "VARIABLE=$line"
      if {[regexp -nocase "no symbol" $line]} {
	continue
      } 
      
      set line [split $line =]
      if {[string length $line]} {
	set name  [string trim [lindex $line 0] \ \r\n]
	lappend variables [list $name $name]
      }
    }

    #debug "--getLocals:\n$variables\n--getLocals"
    return $variables
  }

  # OVERRIDE THIS METHOD and call it from there
  method update {} {
    global Update Display
        # debug "VaraibleWin::update"

    # First, reset color on label to black
    foreach w $ChangeList {
      catch {
	$Hlist entryconfigure $w -style $NormalTextStyle
      }
    }

    # Get all the values of the variables shown in the box
    set variables [displayedVariables {}]

    # We now have a list of all editable variables.
    # Find out which have changed.
    foreach var $variables {
      #      debug "VARIABLE: $var ($Update($this,$var))"
      set newValue [changed $var]
      if {"$newValue" != ""} {
	# Insert newValue into list and highlight it
	set Values($var) $newValue
	lappend ChangeList $var
	$Hlist entryconfigure $var \
	  -style  $HighlightTextStyle   \
	  -text [virtual label $var]
      }
    }
  }

  method idle {} {
    # Re-enable the UI
    enable_ui
  }

  # RECURSION!!
  method displayedVariables {top} {
    #    debug "VariableWin::displayedVariables"
    set variableList {}
    set variables [$Hlist info children $top]
    foreach var $variables {
      set mode [$Tree getmode $var]
      if {"$mode" == "close"} {
	set moreVars [displayedVariables $var]
	lappend variableList [join $moreVars]
      }
      lappend variableList $var
    }

    return [join $variableList]
  }

  method deleteTree {} {
    global Update Display
    #    debug "VariableWin::deleteTree"
    set variables [displayedVariables {}]

    # Delete all HList entries
    $Hlist delete all

    # Delete the variable objects
    foreach i [array names Variables] {
      $Variables($i) delete
      unset Variables($i)
      unset Values($i)
      unset Types($i)
      unset Display($this,$i)
      unset Update($this,$i)
    }

    set Locals {}
    set ChangeList {}
  }

  # ------------------------------------------------------------------
  # METHOD:   enable_ui
  #           Enable all ui elements.
  # ------------------------------------------------------------------
  method enable_ui {} {
    
    # Clear fencepost
    set Running 0
    [winfo toplevel $this] configure -cursor {}
  }

  # ------------------------------------------------------------------
  # METHOD:   disable_ui
  #           Disable all ui elements that could affect gdb's state
  # ------------------------------------------------------------------
  method disable_ui {} {

    # Set fencepost
    set Running 1

    # Cancel any edits
    if {[info exists EditEntry]} {
      UnEdit
    }

    # Change cursor
    [winfo toplevel $this] configure -cursor watch
  }

  # ------------------------------------------------------------------
  # METHOD:   no_inferior
  #           Reset this object.
  # ------------------------------------------------------------------
  method no_inferior {} {

    # Clear out the Hlist
    deleteTree

    # Clear fencepost
    set Running 0
    [winfo toplevel $this] configure -cursor {}
  }

  #
  # PUBLIC DATA
  #

  #
  #  PROTECTED DATA
  #

  # the tixTree widget for this class
  protected Tree  {}

  # the hlist of this widget
  protected Hlist {}

  # entry widgets which need to have their color changed back to black
  # when idle (used in conjunction with update)
  protected ChangeList {}

  # array of variable values in tree
  protected Variables
  protected Values
  protected Types
  protected ViewMenu
  protected Popup

  # These are for setting the indent level to an number of characters.
  # This will help clean the tree a little
  common EntryLength 15
  common Length 1
  common LengthString " "

  # These should be common... but deletion?
  # Display styles for HList
  protected HighlightTextStyle
  protected NormalTextStyle
  protected DisabledTextStyle
 
  protected Radix

  protected Locals {}

  protected Editing {}
  protected EditEntry

  # Fencepost for enable/disable_ui and idle/busy hooks.
  protected Running 0
}

#
# Variable
# ----------------------------------------------------------------------
# Implements variable class for browsing
#
itcl_class Variable {

  #
  # Public Methods
  #
  constructor {name gdb config} {
    #    debug "Making Variable \"$this\":"
    
    # Maybe we want to change this?
    # What about duplicates?
    set Name $name
    set gdbName $gdb
    set Type  [setType]

    return $this
  }

  destructor {
    #    debug "Destroying \"$this\""
  }

  #
  # Private Methods
  #

  ######################################################################
  #                                                                    #
  # isOpenable                                                         #
  #                                                                    #
  #                 WARNING! WARNING! DANGER! DANGER!                  #
  #                                                                    #
  # This method controls what is "openable" in the tixTree widget used #
  # by the variable/display windows. Anything that should cause the    #
  # tree to allow a user to show more info (like the members of a      #
  # struct or dereferencing a pointer) should be added to here.        #
  #                                                                    #
  # Right now, though, we only do structs... This should be extended   #
  # to pointers before a demo...                                       #
  ######################################################################
  method isOpenable {} {

    # We want to make sure that all pointers and structs are openable
    if {[isPointer] && "$Type" != "char *"
	|| [isStruct] || [isStructPtr]
	|| [isUnion]
	|| [isClass]  || [isClassPtr]} {
      return 1
    }
    return 0
  }
  
  method isClass {} {
    if {[regexp "^class" $Type] && ![regexp "\\*$" $Type]} {
      return 1
    }
    return 0
  }

  method isClassPtr {} {
    set l [split $Type \ ]
    set base  [lindex $l 0]
    set stars [lindex $l end]
    if {"$base" == "class" && "$stars" == "*"} {
      return 1
    }
    return 0
  }

  # This is needed for those *special* little moments...
  method isUnamed {} {
    if {[isStruct]} {
      if {"${Type}" == "struct"} {
	return 1
      }
    } elseif {[isUnion]} {
      if {"${Type}" == "union"} {
	return 1
      }
    }
    
    return 0
  }

  method isUnion {} {
    if {[regexp "^union" $Type]} {
      return 1
    }
    return 0
  }

  method isStructPtr {} {
    set l [split $Type \ ]
    set base  [lindex $l 0]
    set stars [lindex $l end]
    if {"$base" == "struct" && "$stars" == "\*"} {
      return 1
    }
    return 0
  }

  # return true if type = struct
  method isStruct {} {
    if {[regexp "^struct" $Type] && ![regexp "\\*$" $Type]} {
      return 1
    }
    return 0
  }

  method isFunction {} {
    if ![isStruct] {
      return [regexp \\( "$Type"]
    }
    
    return 0
  }
     
  # Return pathname
  method pathname {sep} {
    regsub -all \\. $Name $sep path
    return $path
  }

  # Return type
  method type  {} {
    return $Type
  }
    
  method classInfo {} {
    return $ClassInformation
  }

  method setType {} {

    # We cannot simply use "whatis" here -- we need to know whether something
    # is a struct or a union, etc...
    set name $gdbName

    if {"[string index $gdbName 0]" == "\$"} {
      # Convenience variable
      set result "\$"
    } else {
      set cmd "ptype $name"
      #debug "setType cmd = \"$cmd\""
      catch {gdb_cmd $cmd} msg
      #    set msg [string trim $msg \ \r\n]
      
      # DO NOT :set msg [split $msg]
      # The msg will always look like this:
      # lindex    value
      # 0         "type" ALWAYS
      # 1         "="    ALWAYS
      # 2         type (int, double, char) or struct/class
      # 3         if not struct, could be *
      #           if struct, name of struct/class
      # 4         members of struct or none if not struct
      # 5         *'s (indicating pointer, etc.)
      # plus more for C++
      set type [string trim [lindex $msg 2] \ \r\n]
      
      #debug "MSG:\n$msg"
      #debug "TYPE=$type"
      
      if {"$type" == "struct" || "$type" == "union" || "$type" == "class"} {
	if {[regexp "^type = union \{" $msg]
	    || [regexp "^type = struct \{" $msg]} {
	  # This is trouble..
	  #debug "Setting $Name to unamed $type"
	  set result $type
	} else {
	  set nm   [string trim [lindex $msg 3] \ \r\n]
	  if {"$type" != "class"} {
	    set mods [string trim [lindex $msg 5] \ \r\n]
	  } else {
	    # C++ class, possibly public this or that...
	    #debug "C++"
	    set mods [lindex $msg end]
	    if ![regexp {^\*+$} $mods] {
	      set mods {}
	    }
	    set line [lindex [split $msg \n] 0]
	    set first [string first = $line]
	    set last  [string first \{ $line]
	    incr first 2
	    incr last -1
	    set line  [string range $line $first $last]
	    set ClassInformation $line
	    #debug "ClassInformation=$ClassInformation"
	  }
	  #debug "nm=$nm"
	  #debug "mods=$mods"
	  #debug "type=$type"
	  set result [string trim "$type $nm $mods" \ \r\n]
	}
      } else {
	#      catch {set mods " [string trim [lindex $msg 3] \ \r\n]"}
	#      set result [string trim "$type$mods" \ \r\n]
	set type [split $msg =]
	set result [lindex $type 1]
      }
      
      #debug "TYPE \"$Name\"=$result"
      return [string trim $result \ \r\n]
    }
  }

  method isConvenience {} {
    
    if {"$Type" == "$"} {
      return 1
    }

    return 0
  }

  method displayHex {} {

    if {[isPointer]  || [isFunction] || [isStruct]
	|| [isUnion] || [isConvenience]} {
      return 1
    }

    return 0
  }

  method isArray {} {
    if {[regexp -- \\\[ \{$Type\} ]} {
       return 1
    }
    return 0
  }
    
  method isEnum {} {
    if {[regexp -- "^enum" $Type]} {
      return 1
    }
    
    return 0
  }

  # Need to return whether the type is a foating-point type or an integer type
  method isFloat {} {
    switch -regexp -- $Type {
      ^float* {
	return 1
      }
      ^double* {
	return 1
      }
      default {
	return 0
      }
    }
  }
    
  method isPointer {} {
    return [regexp -- \\* $Type]
  }

  method isEditable {} {

    # Is this too strict?
    if {![isFunction] && ![isUnamed] && ![isStruct]
	&& ![isUnion] && ![isArray]  && ![isClass]} {
      return 1
    }
    return 0
  }

  method value {{how {}} {noExtra 0}} {

    # Must check if we are doing a pointer dereference...
    set name $gdbName
    

    if {"$how" == ""} {
      # Choose some reasonable default
      if {[isPointer] || [isFunction]} {
	set how x
      } else {
        if [catch {gdb_cmd "show output-radix"} msg] {
          set radix 10
        } else {
	  set radix 10
          regexp {[0-9]+} $msg radix
        }

        switch $radix {
           8  { set how o }
           10 { set how d }
           16 { set how x }
        }
#	set how d
      }
    }

    # Get special text -- this is for chars, char *s and other type where
    # data has more natural presentation than a number
    if {"$Type" == "char"} {
      # If its a char, make sure we get the string associated with it, to
      # This way, we see things like:
      # char a = 'A';  ----->  a          65 'A'
      # or             ----->  a          0x41 'A'
      #      debug "***char"
      set err [catch {gdb_cmd "output $name"} msg]
      if $err {
	#	debug "Variable::value ERROR 1 ($name):\n$msg"
	return [string trim $msg \ \r\n]
      }
      #      debug "TEXT MSG: $msg"
      set start [string first \' $msg]
      if {$start != -1} {
	set text [string range $msg $start end]
      }
    } elseif {"$Type" == "char *"} {
      # We do the same sort of thing for char *'s, so that
      # we can see the whole string associated with it
      #      debug "***char *"
      set err [catch {gdb_cmd "output $name"} msg]
      if $err {
	#	debug "Variable::value ERROR 2 ($name):\n$msg"
	return [string trim $msg \ \r\n]
      }
      #      debug "TEXT MSG: $msg"
      set start [string first \" $msg]
      if {$start != -1} {
	set text [string range $msg $start end]
      } else {
	# Make sure we get any appended error, too
	set msg [split $msg \ ]
	set text [lrange $msg 1 end]
      }
    } elseif {[isEnum]} {
      set err [catch {gdb_cmd "output $name"} msg]
      if $err {
	return [string trim $msg \ \r\n]
      }
      set text "($msg)"
    }

    #    catch {debug "TEXT: $text"}
    # Must make sure that floats set to "decimal" are output as
    # floats
    if {[isFloat] && "$how" == "d"} {
      set how f
    }
    
    if {[isFunction]} {
      set how {}
    } else {
      set how "/$how"
    }
    set cmd "output$how $name"
    #debug "Variable::value: $cmd"
    set err [catch {gdb_cmd $cmd} msg]
    if $err {
      #debug "Variable::value ERROR 3 ($name):\n$msg"
      return [string trim $msg \\r\n]
    }
    
    #debug "MESSAGE: $msg"
    if {[isStruct] || [isUnion] || [isClass]} {
      set value "$Type \{...\}"
    } elseif {[isFunction]} {
      # msg will look like:
      # {int (int, int)} 0xabcde <foo>
      set value [lindex $msg 1]
    } else {
      # if it's not a struct or func, get the value
      # all the ouput should look like
      # 0x8282 "spidj jd"      
      set value $msg
    }
    
    set value [string trim $value \ \r\n]
    if {[info exists text] && !$noExtra} {
      set value [concat $value $text]
    }
    
    set value [string trim $value \ \r\n]
    if {[isPointer]} {
      if $noExtra {
	set value "$value"
      } else {
	set value "($Type) $value"
      }
    }
    
    #debug "getValue: ($Type) $name = $value"
      
    return $value
  }
    
  method name {} {
    set list [split $Name @]
    return [lindex $list end];		# [expr [llength $list]-1]
  }

  method gdbName {} {
    return $gdbName
  }

  #
  # Public Data
  #

  #
  # Protected Data
  #
  protected Type
  protected Name
  protected gdbName
  protected ClassInformation

  #
  # Shared Data
  #
  }
