#
#	perl script to construct skelton implementations of
#	isi functions.
#
# Copyright © 2022 Dialog Semiconductor
#
# Permission is hereby granted, free of charge, to any person obtaining a copy of
# this software and associated documentation files (the "Software"), to deal in 
# the Software without restriction, including without limitation the rights to
# use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
# of the Software, and to permit persons to whom the Software is furnished to do
# so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

use File::Basename;
use strict;

sub MakeAssemblyStub {
	my($file, $forwarder, $forwardee) = @_;

	open HDR, ">$file" or warn "\nError: Cannot create or open debug header file $file\n";
	print HDR ";\t$file implementing $forwarder\n";
	print HDR ";\n";
	print HDR ";\tInternal use only. Do not distribute.\n";
	print HDR ";\tCopyright (C) 2005 Echelon Corporation\n";
	print HDR ";\tRevision 0, July 2005, Bernd Gauweiler\n";
	print HDR ";\n";
	print HDR "\tRADIX\tHEX\n";
	print HDR "\tIMPORT\t$forwardee\n";
	print HDR ";\n";
	print HDR "\tSEG\tCODE\n";
	print HDR "\tORG\n";
	print HDR "%$forwarder\tAPEXP\t; forwarder body\n";
	print HDR "\tBRF\t$forwardee\t; jump to forwardee\n";
	print HDR ";\n";
	print HDR ";\tend of $file\n";
}

sub MakeNeuronCStub {
	my($file, $function) = @_;

	open HDR, ">$file" or warn "\nError: Cannot create or open debug header file $file\n";
	print HDR <<HERE;
//	$file	implementing $function
//
//	Internal use only. Do not distribute.
//	Copyright (C) 2005 Echelon Corporation
//	Revision 0, July 2005, Bernd Gauweiler
//
#include "isi_int.h"

type $function(args) {
	//	###TBD
}

//	end of $file
HERE
}

sub main {
	if (@ARGV != 2) {
		print "Usage: mkfunc <function> <file>\nExample: mkfunc IsiSign sign.c\n";
		print "Notice mkfunc assumes a forwarder if the file extension is .ns,\nand creates an assembly stub thus.\n";
	} else {
		my($function, $file) = @ARGV;
		my($basename, $path, $extension) = fileparse($file, qr/\..*/);

		if (-e $file) {
			die "\nError: $file already exists.\n";
		}

		if ($extension =~ m/\.ns/i) {
			#	assembly
			my($forwardee) = $function;
			$forwardee =~ s/(^I)(.*)/%i$2/;
			MakeAssemblyStub($file, $function, $forwardee);
		} elsif ($extension =~ m/\.c/i) {
			#	c
			MakeNeuronCStub($file, $function);
		} else {
			print "\nError: File extension $extension not recognized. Don't know what to do with $file.\n";
		}
	}
}

main();
