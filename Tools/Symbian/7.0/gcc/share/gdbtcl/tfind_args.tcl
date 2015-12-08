#
# TfindArgs
# ----------------------------------------------------------------------
# Implements tfind arguments dialogs
#
#   PUBLIC ATTRIBUTES:
#     
#     Type .........Type of dialog (tfind pc, tfind line, tfind tracepoint)
#
#     config ....... used to change public attributes
#
#     PRIVATE METHODS
#
#   X11 OPTION DATABASE ATTRIBUTES
#
#
# ----------------------------------------------------------------------
#   AUTHOR:  Elena Zannoni     ezannoni@cygnus.com
#            Copyright (C) 1998 Cygnus Solutions
#
 
 
itcl_class TfindArgs {
  # ------------------------------------------------------------------
  #  CONSTRUCTOR - create new tfind arguments dialog
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
  #  METHOD:  build_win - build the dialog
  # ------------------------------------------------------------------
  method build_win {} {

      frame $this.f
      frame $this.f.a
      frame $this.f.b
      set f $this.f.a

    switch $Type {
        LN   { set text "Enter argument: " }
        PC   { set text "Enter PC value: " }
        TP   { set text "Enter Tracepoint No.: " }
    }

    label $f.1 -text $text
    entry $f.2 -textvariable argument -width 10
    grid $f.1 $f.2 -padx 4 -pady 4 -sticky nwe
  
    button $this.f.b.ok -text OK -command "$this ok" -width 7
    button $this.f.b.quit -text Cancel -command "$this delete" -width 7
    grid $this.f.b.ok $this.f.b.quit  -padx 4 -pady 4  -sticky ews

    pack $this.f.a $this.f.b  
    grid $this.f

    focus $f.2
}

  # ------------------------------------------------------------------
  #  DESTRUCTOR - destroy window containing widget
  # ------------------------------------------------------------------
  destructor {
    set top [winfo toplevel $this]
    manage delete $this 1
    destroy $this
      destroy $top
  }
 


  # ------------------------------------------------------------------
  #  METHOD:  ok - do it and quit
  # ------------------------------------------------------------------
  method ok {} {
    do_it 
    delete
  }


  # ------------------------------------------------------------------
  #  METHOD:  do_it - call the gdb command
  # ------------------------------------------------------------------
  method do_it {} {
  global argument


  switch $Type {
      LN  { tfind_cmd "tfind line $argument"} 
      PC  { tfind_cmd "tfind pc $argument"}
      TP  { tfind_cmd "tfind tracepoint $argument"} 
  }
 }


 public Type


}
