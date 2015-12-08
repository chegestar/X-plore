#
# Globalpref
# ----------------------------------------------------------------------
# Implements Global preferences dialog
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
#   AUTHOR:  Martin M. Hunt	hunt@cygnus.com     
#            Copyright (C) 1997, 1998 Cygnus Solutions   
#


itcl_class Globalpref {
  # ------------------------------------------------------------------
  #  CONSTRUCTOR - create new toolbar preferences dialog
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
    build_win
    after idle [list wm deiconify $top]
  }

  # ------------------------------------------------------------------
  #  METHOD:  build_win - build the dialog
  # ------------------------------------------------------------------
  method build_win {} {
    global tcl_platform GDBTK_LIBRARY
    if {$attach == 1} {
      set frame $this
    } else {
      frame $this.f
      frame $this.x
      set frame $this.f
    }

    # Icons
    frame $frame.icons
    label $frame.icons.lab -text "Icons "
    combobox::combobox $frame.icons.cb -editable 0 -maxheight 10\
      -command "$this change_icons"

    # get list of icon directories
    set icondirlist ""
    cd $GDBTK_LIBRARY
    foreach foo [glob -- *] {
      if {[file isdirectory $foo] && [file exists [file join $foo "icons.txt"]]} {
	lappend icondirlist $foo
      }
    }

    set width 14
    # load combobox
    set imagedir [pref get gdb/ImageDir]
    foreach dir $icondirlist {
      if {![string compare $dir $imagedir]} {
	set cdir 1
      } else {
	set cdir 0
      }
      set foo [file join $dir "icons.txt"]
      if [catch {open $foo r} fid] {
	# failed
	if {$cdir} {$frame.icons.cb entryset "unknown icons"}
	$frame.icons.cb list insert end "unknown icons"
      } else {
	if {[gets $fid txt] >= 0} {
	  if {$cdir} {$frame.icons.cb entryset $txt}
	  if {[string length $txt] > $width} {set width [string length $txt]}
	  $frame.icons.cb list insert end $txt
	} else {
	if {$cdir} {$frame.icons.cb entryset "unknown icons"}
	  $frame.icons.cb list insert end "unknown icons"
	}
	close $fid
      }
    }
    $frame.icons.cb configure -width $width
    
    # create ComboBox with font name
    lappend Fonts fixed
    set Original(fixed,family) [font actual global/fixed -family]
    set Original(fixed,size) [font actual global/fixed -size]
    font create test-fixed-font -family $Original(fixed,family) \
      -size $Original(fixed,size)
    Labelledframe $frame.d -text "Fonts"
    set f [$frame.d get_frame]
    label $f.fixedx -text "Fixed Font:"

    combobox::combobox $f.fixedn -editable 0 -value $Original(fixed,family) \
      -command "$this wfont_changed family fixed"

    # searching for fixed font families take a long time
    # therefore, we cache the font names.  The font cache
    # can be saved in the init file. A way should be provided
    # to rescan the font list, without deleting the entry from the
    # init file.
    set font_cache [pref get gdb/font_cache]
    if {$font_cache == ""} {
      if {$tcl_platform(platform) == "unix"} {
	toplevel .c
	wm title .c "Scanning for fonts"
	message .c.m -width 3i -text "Scanning system for fonts\n\nPlease wait..." \
	  -relief flat -padx 30 -pady 30 \
	  -bg [pref get gdb/global_prefs/message_bg] \
	  -fg [pref get gdb/global_prefs/message_fg]
	@@update
	pack .c.m
	focus .c
	raise .c
	@@update
      }
      set fam [font families]
      foreach fn $fam {
	if {[font metrics [list $fn] -fixed] == 1} {
	  lappend font_cache $fn
	}
      }
      pref set gdb/font_cache $font_cache
      if {$tcl_platform(platform) == "unix"} { destroy .c }
    }

    foreach fn $font_cache {
      $f.fixedn list insert end $fn
    }

    tixControl $f.fixeds -label Size: -integer true -max 18 -min 6 \
      -value $Original(fixed,size) -command "$this font_changed size fixed" 
    [$f.fixeds subwidget entry] configure -width 2
    label $f.fixedl -text ABCDEFabcdef0123456789 -font test-fixed-font

    if {$tcl_platform(platform) != "windows"} {
      # Cannot change the windows menu font ourselves

      # create menu font names
      lappend Fonts menu
      set Original(menu,family) [font actual global/menu -family]
      set Original(menu,size) [font actual global/menu -size]
      font create test-menu-font -family $Original(menu,family) \
	-size $Original(menu,size)

      label $f.menux -text "Menu Font:"

      combobox::combobox $f.menun -command [list $this wfont_changed family menu]\
	-value $Original(menu,family) -editable 0
      foreach a [lsort [font families]] {
	$f.menun list insert end $a
      }
    
      tixControl $f.menus -label Size: -integer true -max 18 -min 6
      $f.menus config -value $Original(menu,size) \
	-command [list $this font_changed size menu]
      [$f.menus subwidget entry] configure -width 2
      label $f.menul -text ABCDEFabcdef0123456789 -font test-menu-font
    }

    # create default font names
    lappend Fonts default
    set Original(default,family) [font actual global/default -family]
    set Original(default,size) [font actual global/default -size]
    font create test-default-font -family $Original(default,family) \
      -size $Original(default,size)

    label $f.defaultx -text "Default Font:"
    combobox::combobox $f.defaultn -value $Original(default,family) -editable 0\
      -command [list $this wfont_changed family default]
    foreach a [lsort [font families]] {
      $f.defaultn list insert end $a
    }
    
    tixControl $f.defaults -label Size: -integer true -max 18 -min 6 \
      -value $Original(default,size) \
      -command [list $this font_changed size default]
    [$f.defaults subwidget entry] configure -width 2
    label $f.defaultl -text ABCDEFabcdef0123456789 -font test-default-font

    # create status font names
    lappend Fonts status
    set Original(status,family) [font actual global/status -family]
    set Original(status,size) [font actual global/status -size]
    font create test-status-font -family $Original(status,family) \
      -size $Original(status,size)

    label $f.statusx -text "Statusbar Font:"
    combobox::combobox $f.statusn -value $Original(status,family) -editable 0\
      -command [list $this wfont_changed family status] 
    foreach a [lsort [font families]] {
      $f.statusn list insert end $a
    }
    
    tixControl $f.statuss -label Size: -integer true -max 18 -min 6 \
      -value $Original(status,size) \
      -command [list $this font_changed size status]
    [$f.statuss subwidget entry] configure -width 2
    label $f.statusl -text ABCDEFabcdef0123456789 -font test-status-font

    # Map everything
    foreach thunk $Fonts {
      grid $f.${thunk}x $f.${thunk}n $f.${thunk}s $f.${thunk}l -sticky we -padx 5 -pady 5
    }

    pack $frame.icons.lab $frame.icons.cb -side left
    pack $frame.icons -side top -padx 10 -pady 10
    pack $frame.d -side top -fill both -expand yes

    if {$attach != 1} {
      button $this.x.ok -text OK -underline 0 -width 7 -command [list $this ok]
      button $this.x.apply -text Apply -width 7 -underline 0 -command [list $this apply]
      button $this.x.cancel -text Cancel -width 7 -underline 0 -command [list $this cancel]
      pack $this.f -fill both -expand yes -padx 5 -pady 5
      pack $this.x.ok $this.x.apply $this.x.cancel -side left
      standard_button_box $this.x
      pack $this.x -fill x -padx 5 -pady 5
      bind $this.x.ok <Return> "$this ok" 
      focus $this.x.ok
    }
    @@update
  }

  # ------------------------------------------------------------------
  #  DESTRUCTOR - destroy window containing widget
  # ------------------------------------------------------------------
  destructor {
    foreach thunk $Fonts {
      font delete test-$thunk-font
    }
    
    set top [winfo toplevel $this]
    manage delete $this 1
    destroy $this
    if {$attach != 1} {
      destroy $top
    }
  }

  # ------------------------------------------------------------------
  #  METHOD:  config - used to change public attributes
  # ------------------------------------------------------------------
  method config {config} {}

  # ------------------------------------------------------------------
  #  PRIVATE METHOD:  change_icons
  # ------------------------------------------------------------------
  method change_icons {w args} {
    global gdb_ImageDir GDBTK_LIBRARY
    set index [$w list curselection]
    if {$index != ""} {
      set dir [lindex $icondirlist $index]
      pref set gdb/ImageDir $dir
      set gdb_ImageDir [file join $GDBTK_LIBRARY $dir]
      manage restart
    }
  }

  # ------------------------------------------------------------------
  #  PRIVATE METHOD:  wfont_changed - callback from font comboboxes
  #  PRIVATE METHOD:  font_changed - callback from font tixControls
  # ------------------------------------------------------------------
  method wfont_changed {attribute font w val} {
    font_changed $attribute $font $val
  }

  method font_changed {attribute font val} {
    # val will be a size or a font name

    switch $attribute {
      size {
	set oldval [font configure test-$font-font -size]
	font configure test-$font-font -size $val
      }

      family {
	set oldval [font configure test-$font-font -family]
	font configure test-$font-font -family $val
      }
      
      default { debug "Globalpref::font_changed -- invalid change" }
    }
  }

  # ------------------------------------------------------------------
  #  METHOD:  reconfig - used when preferences change
  # ------------------------------------------------------------------
  method reconfig {} {
  }

  # ------------------------------------------------------------------
  #  METHOD:  ok - called to accept settings and close dialog
  # ------------------------------------------------------------------
  method ok {} {
    
    set commands {}
    foreach thunk $Fonts {
      set font   [font configure test-$thunk-font]
      if {[pref get global/font/$thunk] != $font} {
	lappend commands [list pref set global/font/$thunk $font]
      }
    }

    if {[llength $commands] > 0} {

      foreach command $commands {
	eval $command
      }
      manage delete $this
      manage restart
      return
    }
    manage delete $this
  }

  # ------------------------------------------------------------------
  #  METHOD:  cancel - forget current settings -- reset to original
  #                    state and close preferences
  # ------------------------------------------------------------------
  method cancel {} {
 
    # Reset fonts if different
    set commands {}
    foreach thunk $Fonts {
      set family [font configure global/$thunk -family]
      set size   [font configure global/$thunk -size]
      if {$Original($thunk,family) != $family || $Original($thunk,size) != $size} {
	lappend commands [list pref set global/font/$thunk [list -family $Original($thunk,family) -size $Original($thunk,size)]]
      }
    }

    if {[llength $commands] > 0} {
      foreach command $commands {
	eval $command
      }
    }
    manage delete $this

    if {[llength $commands] > 0} {
      manage restart
    }
  }

  # ------------------------------------------------------------------
  #  METHOD:  apply - apply current settings to the screen
  # ------------------------------------------------------------------
  method apply {} {
    
    set commands {}
    foreach thunk $Fonts {
      set font [font configure test-$thunk-font]

      if {[pref get global/font/$thunk] != $font} {
	lappend commands [list pref set global/font/$thunk $font]
      }
    }      

    if {[llength $commands] > 0} {

      foreach command $commands {
	eval $command
      }
      manage restart
    }
  }

  #
  #  PROTECTED DATA
  #
  public attach 0

  protected icondirlist ""

  # Original settings
  protected Original
  
  # List of all available fonts for editing
  protected Fonts
}

