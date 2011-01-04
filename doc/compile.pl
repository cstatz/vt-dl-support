#!/usr/bin/perl

my $file_prefix = "UserManual";
my $tex_file = "$file_prefix.tex";
my $pdf_file = "$file_prefix.pdf";
my $html_file = "$file_prefix.html";
my $html_title = "VampirTrace - User Manual";
my $tmp_css_file = "html/$file_prefix.css";
my $tmp_html_file = "html/$file_prefix.html";

my $cmd;

# call pdflatex to have the toc, aux, etc. files there
#
for( $i = 0; $i < 2; $i++ ) { # 2x to get the table of contents
  $cmd = "pdflatex $tex_file";
  print "$0: executing command '$cmd'...\n";
  if( system( $cmd ) != 0 ) {
    print "$0: error: could not execute command '$cmd'\n";
    exit(1);
  }
}
print "$0: created '$pdf_file'.\n";

# create HTML from LaTex using latex2html
#
$cmd = "latex2html -t '$html_title' -split 0  -nonavigation -dir html -mkdir -noinfo -noaddress -no_images $tex_file";
print "$0: executing command '$cmd'...\n";
if( system( $cmd ) != 0 ) {
  print "$0: error: could not execute command '$cmd'\n";
  exit(1);
}

# read CSS content
#
print "$0: reading CSS file '$tmp_css_file'...\n";
my @css_content = "";
open(CSS, "<$tmp_css_file") || die "$0: error: could not open '$tmp_css_file' for reading";
@css_content = <CSS>;
close(CSS);

# open input HTML file
open(HTMLIN, "<$tmp_html_file") || die "$0: error: could not open '$tmp_html_file' for reading";
# open output HTML file
open(HTMLOUT, ">$html_file") || die "$0: error: could not open '$html_file' for writing";

# read lines from input HTML
#
print "$0: post-processing '$tmp_html_file'...\n";
while(<HTMLIN>) {
  my $line = $_;

  # replace <LINK ...> by the CSS content
  #
  if( $line eq "<LINK REL=\"STYLESHEET\" HREF=\"$file_prefix.css\">\n" ) {
    print HTMLOUT "<STYLE>\n";
    print HTMLOUT @css_content;
    print HTMLOUT "\nBODY\t\t\t{ font-family: sans-serif; }\n";
    print HTMLOUT "</STYLE>\n";
  # further replacements
  #
  } else {
    $line =~ s/&ge;/&#8805;/gi;
    $line =~ s/&le;/&#8804;/gi;
    $line =~ s/&rArr;/&#8658;/gi;
    $line =~ s/&times;/&#215;/gi;
    print HTMLOUT $line;
  }
}

close(HTMLOUT);
close(HTMLIN);

print "$0: created '$html_file'.\n";

# clean-up
print "$0: cleaning up...\n";
system("rm -rf html $file_prefix.out $file_prefix.log $file_prefix.toc $file_prefix.aux");

print "$0: done.\n";
exit;

