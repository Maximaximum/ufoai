#!/bin/perl -w
#####################################################################
# map2obj.pl
#	radiant .map file parser and .obj (wavefront) converter
#	2007-04-05 Copyright Werner 'hoehrer' H�hrer bill_spam2 [AT) yahoo (DOT] de
#
# Licence:
#	The content of this file is under the GNU/GPL license v2.
#	http://www.gnu.org/licenses/gpl.txt
#
# Changelog:
#	2007-04-05	Basic parser	-- Werner 'hoehrer' H�hrer
#####################################################################
# usage: TODO
#####################################################################

use strict;
use warnings;

my $entity_open = 0;
my $brush_open = 0;

my $map_filename = 'condor04.map'; # Dummy, never used
my $obj_filename;
my $map = {};


############################
# Initialize the map data.
############################
sub map_init ($) {
	my ($map) = @_;
	
	# Init the material list
	$map->{materials} = {};
	$map->{materialcount} = 0;

	$map->{entities} = [];
	$map->{entitycount} = 0;
	
	# Init the brush-array
	$map->{brushes} = [];
	$map->{brushcount} = 0;
	
	# Init map filename (just for info)
	$map->{map_filename} = '';
	return $map;
}

sub brush_init ($) {
	my ($brush) = @_;

	$brush->{polygons} = [];
	$brush->{textures} = [];
	$brush->{polycount} = 0;
	
	$brush->{data} = {};
	return $brush;
}

sub entity_init ($) {
	my ($entity) = @_;

	# init entity
	$entity = {};
	$entity->{brushes} = [];
	$entity->{brushcount} = 0;
	$entity->{data} = {};
	return $entity;
}

############################
# Parse the text from the map file.
############################
my @lines;
sub map_parse ($) {
	my ($filename) = @_;

	open(MAP_INPUT, "< ".$filename) ||
		die "Failed to open mapfile '", $filename, "' .\n";

	my $entity;
	my $brush;
		
	$map->{map_filename} = $filename;
	my $line;
	
	if ($map->{entitycount} >2) { return };
	while(<MAP_INPUT>) {
		next if /^\s*\/\//;	# Skip comments
		next if /^$/;	# Skip newlines
		next if /^\s*$/;	# Skip line with only whitespaces
		chomp;	# Remove trailing whitespaces
		

		if (!$brush_open) {	# just in case
			$brush = {};
		}
		if (!$entity_open) {	# just in case
			$entity = {};
		}

		$line = $_;
		#print $line,"\n";
		if ($line =~ m/^\s*{\s*$/) {
			# Opening curly bracket found.
			#print "open bracket\n"; #debug
			if ($entity_open) {
				if (!$brush_open) {
					# Brush opening curly-bracket found.
					
					# init brush (TODO: extra function?)
					$brush_open = 1;
					$brush = brush_init($brush);
				} else {
					print "Bad opening bracked found: '",$line,"'\n";
					print "Syntax of file possibly corrupted. Abort.\n";
					return;
				}
			} else {
				print "New enity.\n";
				# Entity opening curly-bracket found.
				$entity_open = 1;
				$entity = entity_init($entity);
		
				# Add brush to map-tree
				push (@{$map->{entities}}, $entity);
				$map->{entitycount}++;
			}
			next;
		} 
		
		if (($line =~ m/^\s*\(([^)]*)\)\s*\(([^)]*)\)\s*\(([^)]*)\)\s*([^\s]*)\s*(\d+\s*\d+\s*\d+\s*\d+\s*\d+\s*\d+\s*\d+\s*\d+)\s*$/) && $brush_open) {
			print "New brush.\n"; #debug
			# Found a polygon with texture.
			print "Parsing brush:", $map->{brushcount}-1,"\n";
			print "      Polygon:", $brush->{polycount},"\n";
			my ($v1, $v2, $v3) = ($1, $2, $3);
			my $tex = $4;
			my $the_others = $5;
			$v1 =~ s/^\s+//;
			$v2 =~ s/^\s+//;
			$v3 =~ s/^\s+//;
			$tex =~ s/^\s+//;
			my @vert1 = split(/\s+/, $v1);
			my @vert2 = split(/\s+/, $v2);
			my @vert3 = split(/\s+/, $v3);
			
			#my $polygon = [${@vert1}, $#{@vert2}, $#{@vert3}];
			my $polygon = [[@vert1],[@vert2], [@vert3]];
			push (@{$brush->{polygons}}, $polygon);
			push (@{$brush->{textures}}, $tex); # store texture

			if (!exists($map->{materials}->{$brush->{textures}->[$brush->{polycount}]})) {
				$map->{materials}->{$brush->{textures}->[$brush->{polycount}]} = 1;
				$map->{materialcount}++;
			}

			$brush->{polycount}++;
			next;
		}
		if ($line =~ m/^\s*"(.*)"\s*"(.*)"\s*$/) {
			if ($brush_open) {
				# Found brush data
				# example: "origin" "416 -620 76"
				# $1 should be: origin
				# $2 should be: 416 -620 76
				$brush->{data}->{$1} = $2;
				#TODO: parse special cases for later export (like position and rotation).
				next;
			}
			if ($entity_open) {
				# Found brush data
				# example: "origin" "416 -620 76"
				# $1 should be: origin
				# $2 should be: 416 -620 76
				$entity->{data}->{$1} = $2;
				#TODO: parse special cases for later export (like position and rotation).
				if ($1 eq  'classname') {
					print "Entityclass: ",$entity->{data}->{$1}, "\n";
				}
				next;
			}
		}
	
		if ($line =~ m/^\s*}\s*$/) {
			# Closing curly bracket found
			if ($entity_open) {
				if ($brush_open) {
					# We are closing this brush
					$brush_open = 0;
					# Add brush to map-tree
					push (@{$map->{brushes}}, $brush);
					$map->{brushcount}++;
					push (@{$entity->{brushes}}, $brush);
					$entity->{brushcount}++;
				} else {
					# We are closing this enity.
					$entity_open = 0;
				}
			} else {
				print "Badclosing bracked found, we haven't even met an opening one: '",$line,"'\n";
				print "Syntax of file possibly corrupted. Abort.\n";
				return;
			}
			next;
		} 
		
	}
} # parse

########################
# MAIN
########################

# parse commandline paarameters (md2-filenames)
if ( $#ARGV < 0 ) {
	die "Usage:\tmap2obj.pl <file.map>\n";
} elsif ( $#ARGV == 0 ) {
	$map_filename = $ARGV[0];
	print "Mapfile= \"". $map_filename, "\"\n";
}

# Generate output filename
if ($map_filename =~ m/.*\.map$/) {
	($obj_filename = $map_filename) =~ s/\.map$/\.obj/;
} else {
	$obj_filename = $map_filename.".obj";
}

$map = map_init($map);

# Open + parse map file
map_parse($map_filename);

use Data::Dumper;
print Dumper($map);

foreach my $mat  (keys %{$map->{materials}}) {
	print $mat,"\n";
}


# TODO: write obj (only types of "classname" that are supported by obj)
# TODO: write mtl