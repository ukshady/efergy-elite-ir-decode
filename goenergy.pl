#!/usr/bin/perl
use strict;
use warnings;
use Text::CSV;
my $csv = Text::CSV->new({ sep_char => ','});
use RRDs;

# define location of rrdtool databases
my $rrd = '/root/efergy';
# define location of images
my $img = '/var/www';

my $file = 'energylog.txt';
my @fields;
my $ERROR;

# process data for each interface (add/delete as required)
while (1){
&ProcessData("energy", "Energy consumption");
sleep(300);
}
sub ProcessData
{
# inputs: $_[0]: name (ie, External Temp)
#	  $_[1]: description 


	# if rrdtool database doesn't exist, create it
	if (! -e "$rrd/$_[0].rrd")
	{
		print "creating rrd database for $rrd/$_[0].rrd.\n";
		RRDs::create "$rrd/$_[0].rrd",
			"-s 300",
			"DS:energy:GAUGE:600:U:U",
			"RRA:AVERAGE:0.5:1:576",
			"RRA:AVERAGE:0.5:6:672",
			"RRA:AVERAGE:0.5:24:732",
			"RRA:AVERAGE:0.5:144:1460";
	}
	 if ($ERROR = RRDs::error) {print "$0: unable to generate database: $ERROR\n";}

	#get data
	print "energy: Loading Logfile...\n";
	open(my $data, '<', $file) or die "Could not open file \n";
	while (my $line = <$data>) {
	 chomp $line;
	
	if ($csv->parse($line)) {
		 @fields = $csv->fields();
	} else {
	  warn "Line could not be parsed \n";
	}
	}
	print "@fields\n";
	print "updating rrd file\n";
	# insert values into rrd
	RRDs::update "$rrd/$_[0].rrd", "N:$fields[2]";
	if ($ERROR = RRDs::error) {print "$0: unable to update database: $ERROR\n";}
	# create traffic graphs
	&CreateGraph($_[0], "day", $_[1]);
	&CreateGraph($_[0], "week", $_[1]);
	&CreateGraph($_[0], "month", $_[1]); 
	&CreateGraph($_[0], "year", $_[1]);

}

sub CreateGraph
{
# creates graph
# inputs: $_[0]: name (ie, External Temp)
#	  $_[1]: interval (ie, day, week, month, year)
#	  $_[2]: description 

	RRDs::graph "$img/$_[0]-$_[1]-energy.png",
		"-s -1$_[1]",
		"-t data on $_[0] :: $_[2]",
		"--lazy",
		"-h", "100", "-w", "600",
		"-a", "PNG",
		"-v Watts/hr",
		"-o",
		"-X", "3",
		"--units=si",
		"DEF:energy=$rrd/$_[0].rrd:energy:AVERAGE",
		"AREA:energy#32CD32:energy",
		"GPRINT:energy:MAX:  Max\\: %5.1lf %s",
		"GPRINT:energy:AVERAGE: Avg\\: %5.1lf %S",
		"GPRINT:energy:LAST: Current\\: %5.1lf %S Watts/hr\\n",
		"HRULE:0#000000";
	if ($ERROR = RRDs::error) { print "$0: unable to generate $_[0] $_[1]  graph: $ERROR\n"; }
}
