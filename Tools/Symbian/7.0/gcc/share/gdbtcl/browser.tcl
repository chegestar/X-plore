#
# BrowserWin
# ----------------------------------------------------------------------
# Implements Browser window for gdb
#
#   PUBLIC ATTRIBUTES:
#
#
#   PRIVATE METHODS:
#
#   build_win
#   search_src
#   config
#   reconfig
#   search
#   toggle_mode
#   toggle_all_bp
#   toggle_bp
#   select
#   browse
#   fill_source
#   get_selection
#   mode
#   goto_func
#   fill_funcs
#   
#   PUBLIC METHODS:
#
#   X11 OPTION DATABASE ATTRIBUTES
#
#
# ----------------------------------------------------------------------
#   AUTHOR:  Keith Seitz <keiths@cygnus.com>
#   Copyright (C) 1998 Cygnus Solutions
#

itcl_class BrowserWin {
  # ------------------------------------------------------------------
  #  CONSTRUCTOR - create new tdump window
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

    set Top [winfo toplevel $this]
    wm withdraw $Top

    set Current(filename) {}
    set Current(function) {}
    build_win
    after idle wm deiconify $Top

    # Create the toplevel binding for this class
    bind Configure_Browser_$this <Configure> [list $this resize]
  }


  # ------------------------------------------------------------------
  #  METHOD:  build_win - build the main tdump window
  # ------------------------------------------------------------------
  method build_win {} {
    global PREFS_state gdb_ImageDir

    # Four frames: regexp, listboxes, drop-down pane, and buttonbar
    frame $this.regexpf
    frame $this.browser
    frame $this.more
    frame $this.bbar

    set f $this.regexpf
    # Regexp frame
    frame $f.regexp
    frame $f.regexp.top
    frame $f.regexp.bot
    label $f.regexp.top.lbl -text {Search for:}
    set Regexp [entry $f.regexp.top.ent]
    set var [pref varname gdb/search/static]
    checkbutton $f.regexp.bot.stat -variable $var \
      -text {Only show functions declared 'static'}
    set var [pref varname gdb/search/use_regexp]
    checkbutton $f.regexp.bot.reg -variable $var -text {Use regular expression}
    pack $f.regexp.top.lbl -side left -padx 5 -pady 5
    pack $Regexp -side right -fill x -expand yes -padx 5 -pady 5
    pack $f.regexp.bot.stat $f.regexp.bot.reg -side left -fill x -expand yes
    pack $f.regexp.top $f.regexp.bot -side top -fill x -expand yes
    pack $f.regexp -side top -fill x -expand yes -anchor e

    # Browser frame (with listboxes)
    Labelledframe $this.browser.files -anchor nw -text Files
    set f [$this.browser.files get_frame]
    tixScrolledListBox $f.lb -scrollbar auto
    set FilesLB [$f.lb subwidget listbox]
    $FilesLB configure -width 30 -selectmode extended -exportselection false
    set Select [button $f.sel -text {Select All} -width 12 \
		  -command [list $this select 1]]
    pack $f.lb -side top -fill both -expand yes -padx 5 -pady 5
    pack $f.sel -side left -padx 5 -pady 5 -expand yes

    Labelledframe $this.browser.funcs -anchor nw -text Functions
    set f [$this.browser.funcs get_frame]
    tixScrolledListBox $f.lb -scrollbar auto
    set FuncsLB [$f.lb subwidget listbox]
    $FuncsLB configure -width 30 -exportselection false
    button $f.bp -text {Toggle Breakpoint} -command [list $this toggle_all_bp]
    pack $f.lb -side top -fill both -expand yes -padx 5 -pady 5
    pack $f.bp -side bottom -padx 5 -pady 5

    pack $this.browser.files $this.browser.funcs -side left -fill both \
      -expand yes -padx 5 -pady 5

    # "More" frame for viewing source
    if {[lsearch [image names] _MORE_] == -1} {
      image create photo _MORE_ -file [file join $gdb_ImageDir more.gif]
      image create photo _LESS_ -file [file join $gdb_ImageDir less.gif]
    }
    set f [frame $this.more.visible]
    set MoreButton [button $f.btn -image _MORE_ -relief flat \
		      -command [list $this toggle_more]]
    set MoreLabel  [label $f.lbl -text {View Source}]
    frame $f.frame
    frame $f.frame.frame -relief raised -bd 1
    place $f.frame.frame -relx 0 -relwidth 1.0 -height 2 -rely 0.50
    grid $MoreButton $MoreLabel $f.frame -sticky news -padx 5 -pady 5
    grid columnconfigure $f 2 -weight 1

    #  Hidden frame
    set MoreFrame [frame $this.more.hidden]
    pack $f -side top -fill x -expand yes

    # Button bar
    button $this.bbar.search -text Search -command [list $this search]
    pack $this.bbar.search -anchor e

    # map everything onto the screen
    pack $this.regexpf -fill x
    pack $this.browser -fill both -expand yes
    pack $this.more -fill x
    pack $this.bbar -fill x -padx 5 -pady 5

    # Key bindings for "visible" frames
    bind_plain_key $Regexp Return [list $this search]
    bind $FuncsLB <3> [list $this toggle_bp %y]
    bind $FuncsLB <1> [list $this browse %y]

    # Fill defaults
    $Regexp insert 0 [pref get gdb/search/last_symbol]
    set allFiles [gdb_listfiles]
    set FileCount [llength $allFiles]
    foreach file $allFiles {
      $FilesLB insert end $file
    }

    # Construct hidden frame
    set Source [SrcTextWin $MoreFrame.src -Tracing 0 -parent $this -ignore_var_balloons 1]
    set f [frame $MoreFrame.f]
    set FuncCombo [tixComboBox $f.combo -editable true \
		     -command [list $this goto_func] \
		     -options {listbox.height 0 slistbox.scrollbar auto}]
    set ModeCombo [tixComboBox $f.mode -command [list $this mode] \
		     -options {listbox.height 3 entry.width 10 listbox.width 10 slistbox.scrollbar none}]
    set Search [entry $f.search -bd 3 -font src-font -width 10]
    pack $FuncCombo $ModeCombo -side left -padx 5 -pady 5
    pack $Search -side right -padx 5 -pady 5
    pack $Source -side top -fill both -expand yes
    pack $f -side bottom -fill x
    
    # Fill combo boxes
    $ModeCombo insert end SOURCE
    $ModeCombo insert end ASSEMBLY
    $ModeCombo insert end MIXED
    # don't allow SRC+ASM mode... $ModeCombo insert end SRC+ASM
    tixSetSilent $ModeCombo [$Source mode_get]

    # Key bindings for hidden frame
    bind_plain_key $Search Return [list $this search_src forwards]
    bind_plain_key $Search Shift-Return [list $this search_src backwards]
  }

  # ------------------------------------------------------------------
  #  DESTRUCTOR - destroy window containing widget
  # ------------------------------------------------------------------
  destructor {
    set Top [winfo toplevel $this]
    $Source delete
    destroy $this
    destroy $Top
  }

  # ------------------------------------------------------------------
  #  METHOD:  config - used to change public attributes
  # ------------------------------------------------------------------
  method config {config} {}
  
  # ------------------------------------------------------------------
  #  METHOD:  reconfig - used when preferences change
  # ------------------------------------------------------------------
  method reconfig {} {
  }

  # ------------------------------------------------------------------
  #  METHOD:  search_src
  #           Search for text or jump to a specific line
  #           in source window, going in the specified DIRECTION.
  # ------------------------------------------------------------------
  method search_src {direction} {
    set exp [$Search get]
    $Source search $exp $direction
  }

  # ------------------------------------------------------------------
  #  METHOD:  search
  #           Search for files matching regexp/pattern
  #           in specified files
  # ------------------------------------------------------------------
  method search {} {
    set f [$FilesLB curselection]
    set files {}
    if {[llength $f] != $FileCount} {
      # search all files...
      foreach file $f {
	lappend files [$FilesLB get $file]
      }
    }

    set regexp [$Regexp get]
    pref set gdb/search/last_symbol $regexp
    if {![pref get gdb/search/use_regexp]} {
      set regexp "^$regexp"
    }

    $Top configure -cursor watch
    @@update idletasks
    if {$files == ""} {
      set err [catch {gdb_search functions $regexp \
			-static [pref get gdb/search/static]} matches]
    } else {
      set err [catch {gdb_search functions $regexp \
			-files $files -static [pref get gdb/search/static]} matches]
    }      
    $Top configure -cursor {}
    @@update idletasks
    if {$err} {
      debug "ERROR searching for $regexp: $matches"
      return
    }

    $FuncsLB delete 0 end
    foreach func [lsort $matches] {
      $FuncsLB insert end $func
    }
  }

  # ------------------------------------------------------------------
  #  METHOD:  toggle_more
  #           Toggle display of source listing
  # ------------------------------------------------------------------   
  method toggle_more {} {

    if {!$MoreVisible} {
      $MoreLabel configure -text {Hide Source}
      $MoreButton configure -image _LESS_
      pack $MoreFrame -expand yes -fill both
      set MoreVisible 1

      # If we have a selected function, display it in the window
      set f [$FuncsLB curselection]
      if {$f != ""} {
	fill_source [$FuncsLB get $f]
      }

      @@update idletasks

      # Watch for configure events so that the frame gets resized when
      # necessary.
      pack propagate $MoreFrame 0
      after idle bind_toplevel 1
    } else {
      bind_toplevel 0
      pack propagate $MoreFrame 1
      $MoreLabel configure -text {View Source}
      $MoreButton configure -image _MORE_
      pack forget $MoreFrame
      set MoreVisible 0
    }
  }

  # ------------------------------------------------------------------
  #  METHOD:  bind_toplevel
  #            Setup the bindings for the toplevel.
  # ------------------------------------------------------------------
  method bind_toplevel {install} {

    if {$install} {
      set bindings [bindtags $Top]
      bindtags $Top [linsert $bindings 0 Configure_Browser_$this]
    } else {
      set bindings [bindtags $Top]
      set i [lsearch $bindings Configure_Browser_$this]
      if {$i != -1} {
	bindtags $Top [lreplace $bindings $i $i]
      }
    }
  }

  # ------------------------------------------------------------------
  #  METHOD:  resize
  #            Resize "MoreFrame" after all configure events
  # ------------------------------------------------------------------
  method resize {} {

    pack propagate $MoreFrame 1
    after idle pack propagate $MoreFrame 0
  }

  # ------------------------------------------------------------------
  #  METHOD:  toggle_all_bp
  #           Toggle a bp at every function in FuncLB
  # ------------------------------------------------------------------
  method toggle_all_bp {} {

    set funcs [$FuncsLB get 0 end]
    $Top configure -cursor watch
    @@update idletasks
    foreach f $funcs {
      if {[catch {gdb_loc $f} linespec]} {
	return
      }
      set bpnum [@@bp_exists $linespec]
      if {$bpnum == -1} {
	gdb_cmd [list break $f]
      } else {
	catch {gdb_cmd "delete $bpnum"}
      }
    }
    $Top configure -cursor {}
    @@update idletasks
  }

  # ------------------------------------------------------------------
  #  METHOD:  toggle_bp
  #           Toggle bp at function specified by the given Y
  #           coordinate in the listbox
  # ------------------------------------------------------------------
  method toggle_bp {y} {

    set f [get_selection $FuncsLB $y]
    if {$f != ""} {
      if {[catch {gdb_loc $f} linespec]} {
	return
      }
      set bpnum [@@bp_exists $linespec]
      if {$bpnum == -1} {	
	gdb_cmd [list break $f]
      } else {
	catch {gdb_cmd "delete $bpnum"}
      }
    }
  }
  
  # ------------------------------------------------------------------  
  #  METHOD:  select
  #           (Un/Highlight all files in the files list
  # ------------------------------------------------------------------  
  method select {highlight} {

    if {$highlight} {
      $FilesLB select set 0 end
      $Select configure -text {Select None} -command [list $this select 0]
    } else {
      $FilesLB select clear 0 end
      $Select configure -text {Select All}  -command [list $this select 1]
    }
  }

  # ------------------------------------------------------------------
  #  METHOD:  browse
  #           Fill the srctextwin with the selected function
  # ------------------------------------------------------------------
  method browse {y} {
    set f [get_selection $FuncsLB $y]

    # Fill source pane if it is open
    if {$MoreVisible} {
      fill_source $f
    }
  }

  # ------------------------------------------------------------------
  #  METHOD:  fill_source
  #           Helper function to fill the srctextwin
  #           when needed.
  # ------------------------------------------------------------------
  method fill_source {f} {
    if {$f != $Current(function)} {
      if {[catch {gdb_loc $f} linespec]} {
	return
      }
      lassign $linespec foo funcname name line addr pc_addr
      # fill srctextwin
      $Source location BROWSE_TAG $name $funcname $line $addr $pc_addr
      set Current(function) $f
      # fill func combo
      if {$Current(filename) != $name} {
	set Current(filename) $name
	fill_funcs $name
      }
      # Set current function in combo box
      tixSetSilent $FuncCombo $f
    }
  }
  
  # ------------------------------------------------------------------
  #  METHOD:  get_selection
  #           Helper function to return the text of
  #           an element given th listbox (LB) and position in the
  #           listbox (Y)
  # ------------------------------------------------------------------
  method get_selection {lb y} {

    set len [$lb size]
    if {$len == 0} { return }
    for {set i $len} {$i >= 0} {incr i -1} {
      set bbox [$lb bbox $i]
      if {$bbox != ""} {
	break
      }
    }

    set bottom [lindex $bbox 1]
    set height [lindex $bbox 3]
    incr bottom $height
    if {$y > $bottom} {
      after idle $lb selection clear 0 end
      return ""
    }

    set x [$lb nearest $y]
    return [$lb get $x]
  }

  # ------------------------------------------------------------------
  #  METHOD:  mode
  #           Function called by srctextwin when the display
  #           mode changes
  # ------------------------------------------------------------------
  method mode {mode {go 1}} {

    $Source mode_set $mode $go
    tixSetSilent $ModeCombo $mode
  }

  # ------------------------------------------------------------------
  # METHOD:  goto_func
  #          Callback for the function combo box which
  #          sets the srctextwin looking at the given function (VAL)
  # ------------------------------------------------------------------
  method goto_func {val} {

    set mang 0
    if {[info exists _mangled_func($val)]} {
      set mang $_mangled_func($val)
    }
    if {$mang} {
      set loc $val
    } else {
      set fn [lindex [@@file split $Current(filename)] end]
      set loc $fn:$val
    }
    debug "BrowserWin::goto_func GOTO \"$loc\""
    if {![catch {gdb_loc $loc} result]} {
      lassign $result foo funcname name line addr pc_addr
      $Source location BROWSE_TAG $name $funcname $line $addr $pc_addr
    } else {
      debug "BrowserWin::goto_func: gdb_loc returned \"$result\""
    }
  }

  # ------------------------------------------------------------------
  #  METHOD:  fill_funcs
  #           This private method fills the functions combo box
  #           with all the functions in NAME.
  # ------------------------------------------------------------------
  method fill_funcs {name} {

    set lb [$FuncCombo subwidget listbox]
    $lb delete 0 end
    if {$name != ""} {
      set maxlen 10
      if {[catch {gdb_listfuncs $name} listfuncs]} {
	tk_messageBox -icon error -default ok \
	  -title "GDB" -type ok -modal system \
	  -message "This file can not be found or does not contain\ndebugging information."
	return
      }
      foreach f $listfuncs {
	#debug "listfuncs: $f"
	lassign $f func mang
	if {$func == "global constructors keyed to main"} {continue}
	set _mangled_func($func) $mang
	$FuncCombo insert end $func
	if {[string length $func] > $maxlen} {
	  set maxlen [string length $func]
	}
      }
      [$FuncCombo subwidget entry] config -width $maxlen
      [$FuncCombo subwidget listbox] config -width $maxlen
      set height [[$FuncCombo subwidget listbox] size]
      if {$height > 10} {set height 10}
      [$FuncCombo subwidget listbox] config -height $height
    }
  }

  method test_get {var} {
    return [set $var]
  }

  #
  #  PROTECTED DATA
  #
  protected Top; # toplevel
  protected MoreFrame; # hidden frame
  protected MoreLabel; # label for turndown
  protected MoreButton; # little turndown button
  protected Regexp; # expression for which to search
  protected FilesLB; # files listbox
  protected FuncsLB; # functions listbox
  protected FileCount; # #files in project
  protected MoreVisible 0; #whether viewing source
  protected Source {}; #the srctextwin object
  protected Current;
  protected ModeCombo
  protected FuncCombo
  protected _mangled_func
  protected Search; # the search entry
  protected Select; # the select all/none button
}

