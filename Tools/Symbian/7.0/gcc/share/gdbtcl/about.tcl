#
# About
# ----------------------------------------------------------------------
# Implements About window
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
#            Copyright (C) 1997 Cygnus Solutions   
#


itcl_class About {
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
    build_win
  }

  # ------------------------------------------------------------------
  #  METHOD:  build_win - build the window
  # ------------------------------------------------------------------
  method build_win {} {
    global gdb_ImageDir
    set f [frame $this.f]
    label $f.image1 -bg black -fg white -image \
      [image create photo -file [@@file join $gdb_ImageDir gdbtk.gif]]
    label $f.image2 -bg white -image \
      [image create photo -file [@@file join $gdb_ImageDir cygnus.gif]]
    message $f.m -bg black -fg white -text [gdb_cmd {show version}] -aspect 500 -relief flat
    pack $f.image1 $f.m $f.image2 $this.f -fill both -expand yes
    pack $this
    bind $f.image1 <1> "manage delete $this"
    bind $f.image2 <1> "manage delete $this"
    bind $f.m <1> "manage delete $this"
  }

  # ------------------------------------------------------------------
  #  DESTRUCTOR - destroy window containing widget
  # ------------------------------------------------------------------
  destructor {
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
    # for now, just delete and recreate
    destroy $this.f
    build_win
  }
  #
  #  PROTECTED DATA
  #
}

