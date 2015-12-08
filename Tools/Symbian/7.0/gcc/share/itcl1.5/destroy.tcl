# destroy
# ----------------------------------------------------------------------
# Replacement for the usual Tk "destroy" command.
# Recognizes mega-widgets as [incr Tcl] objects and deletes them
# appropriately.  Destroys ordinary Tk windows in the usual manner.
# ----------------------------------------------------------------------
#   AUTHOR:  Michael J. McLennan       Phone: (610)712-2842
#            AT&T Bell Laboratories   E-mail: michael.mclennan@att.com
#
#      RCS:  destroy.tcl,v 1.1 1994/04/22 13:36:07 mmc Exp
# ----------------------------------------------------------------------
#               Copyright (c) 1994  AT&T Bell Laboratories
# ======================================================================
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted,
# provided that the above copyright notice appear in all copies and that
# both that the copyright notice and warranty disclaimer appear in
# supporting documentation, and that the names of AT&T Bell Laboratories
# any of their entities not be used in advertising or publicity
# pertaining to distribution of the software without specific, written
# prior permission.
#
# AT&T disclaims all warranties with regard to this software, including
# all implied warranties of merchantability and fitness.  In no event
# shall AT&T be liable for any special, indirect or consequential
# damages or any damages whatsoever resulting from loss of use, data or
# profits, whether in an action of contract, negligence or other
# tortuous action, arising out of or in connection with the use or
# performance of this software.
# ======================================================================

if {[info commands tk_destroy] == ""} {
	rename destroy tk_destroy
}

# ----------------------------------------------------------------------
#  USAGE:  destroy ?window window...?
#
#  Replacement for the usual Tk "destroy" command.  Destroys both
#  Tk windows and [incr Tcl] objects.
# ----------------------------------------------------------------------
proc destroy {args} {
	global itcl_destroy

	foreach win $args {
		if {[itcl_info objects $win] != "" &&
		    ![info exists itcl_destroy($win)]} {
			set itcl_destroy($win) "in progress"
			$win delete
			unset itcl_destroy($win)
		} else {
			tk_destroy $win
		}
	}
}
