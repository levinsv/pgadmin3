#!/usr/bin/perl

use strict;
use Data::Dump qw(dump);
my $pathdir_help=".";
my @files;
my @stak;
my %function_help;

my %useref;
my %section;
my %ignorehtml=(
'pgbench.html',

);
my @keys_name=(
''
,'overlaps'
);
my @keys=(
   "<td class=\"func_table_entry\">"
   ,"</td>"
   ,'<p><a id="id-1.5.8.15.7.1"' #overlaps
   ,'ass="computeroutput">true</code></pre>' #overlaps end
);
my %others_func=(
'FUNCTIONS-DATETIME-DELAY'=>'pg_sleep'
,'FUNCTIONS-CASE'=>'case'
,'FUNCTIONS-COMPARISONS-IN-SCALAR'=>'in'
,'id-1.5.8.30.15'=>'not in'
,'FUNCTIONS-NULLIF'=>'nullif'
,'FUNCTIONS-COALESCE-NVL-IFNULL'=>'coalesce'
,'FUNCTIONS-GREATEST-LEAST'=>'greatest'
,'greatest'=>'least'
,'FUNCTIONS-LIKE'=>'like'
,'FUNCTIONS-SIMILARTO-REGEXP'=>'similar to'
,'id-1.5.8.30.16' => 'any'
,'id-1.5.8.30.17' => 'all'
,'FUNCTIONS-SUBQUERY-EXISTS'=>'exists'
,'FUNCTIONS-SUBQUERY-IN'=>'in'
,'FUNCTIONS-SUBQUERY-NOTIN'=>'not in'
,'FUNCTIONS-SUBQUERY-ALL'=>'all'
,'FUNCTIONS-SUBQUERY-ANY-SOME'=>'some'
,'some'=>'any'
,'ROW-WISE-COMPARISON' => 'is distinct from'

);
my %tags=(
'pclass="func_signature"'   => '<span style="font-size: 11pt;">',
'pclass="func_signature"/p' => '</span>',
'codeclass="token"'              => '<span style="fgcolor: #0000ff;">',
'codeclass="token"/code'         => '</span>',
'p'                         => '<blockquote>',
'p/p'                       => '</blockquote>',
'ul'                         => '<ul>',
'ul/ul'                       => '</ul>',
'liclass="listitem"'                         => '<li>',
'liclass="listitem"/li'                       => '</li>',
'divclass="itemizedlist"'                         => '',
'divclass="itemizedlist"/div'                     => '',
'divclass="titlepage"'                            => '',
'divclass="titlepage"/div'                        => '',
'divclass="note"'             => '<pre>',
'divclass="note"/div'             => '</pre>',
'divclass="warning"'             => '<pre>',
'divclass="warning"/div'             => '</pre>',
'divclass="tip"'=>'',
'divclass="tip"/div'=>'',
'div'                         => '',
'div/div'                       => '',
'h3class="title"'               =>'<h3>',
'h3class="title"/h3'               =>'</h3>',
'a'                         => '<a>',
'a/a'                       => '</a>',
'sup'                         => '<sup>',
'sup/sup'                       => '</sup>',
'em'                         => '<em>',
'em/em'                       => '</em>',
'emclass="replaceable"'                         => '<b>',
'emclass="replaceable"/em'                       => '</b>',
'emclass="lineannotation"'                         => '',
'emclass="lineannotation"/em'                       => '',
'spanclass="lineannotation"'                         => '<small>',
'spanclass="lineannotation"/span'                    => '</small>',
'acronymclass="acronym"'                         => '',
'acronymclass="acronym"/acronym'                       => '',

'spanclass="quote"'         => '',
'spanclass="quote"/span'    => '',
'spanclass="productname"'         => '<b>',
'spanclass="productname"/span'    => '</b>',
'spanclass="emphasis"'         => '<b>',
'spanclass="emphasis"/span'    => '</b>',
'spanclass="refentrytitle"'         => '<b>',
'spanclass="refentrytitle"/span'    => '</b>',

'spanclass="symbol_font"'         => '<b>',
'spanclass="symbol_font"/span'    => '</b>',
'code'                      => '',
'code/code'                 => '',
'preclass="programlisting"'                     => '<pre>',
'preclass="programlisting"/pre'                 => '</pre>',
'preclass="screen"'                     => '<pre>',
'preclass="screen"/pre'                 => '</pre>',
'preclass="synopsis"'                     => '',
'preclass="synopsis"/pre'                 => '',
'spanclass="optional"'       => '<i>',
'spanclass="optional"/span'  => '</i>',
'spanclass="systemitem"'       => '<i>',
'spanclass="systemitem"/span'  => '</i>',
'spanclass="application"'       => '<i>',
'spanclass="application"/span'  => '</i>',
'codeclass="filename"'       => '<b>',
'codeclass="filename"/code'  => '</b>',
'codeclass="computeroutput"'       => '<i>',
'codeclass="computeroutput"/code'  => '</i>',

'emclass="parameter"'       => '',
'emclass="parameter"/em'    => '',
'emclass="firstterm"'       => '',
'emclass="firstterm"/em'    => '',

'codeclass="varname"'      => '',
'codeclass="varname"/code' => '',
'codeclass="structname"'      => '<b>',
'codeclass="structname"/code' => '</b>',
'codeclass="structfield"'       => '<b>',
'codeclass="structfield"/code'  => '</b>',
'codeclass="literal"'      => '<b>',
'codeclass="literal"/code' => '</b>',
'codeclass="function"'      => '<b>',
'codeclass="function"/code' => '</b>',
'codeclass="returnvalue"'   => '<b>',
'codeclass="returnvalue"/code'   => '</b>',
'codeclass="type"'          => '<b>',
'codeclass="type"/code'     => '</b>',
'codeclass="command"'          => '<b>',
'codeclass="command"/code'     => '</b>',

);
opendir(DIR, $pathdir_help) or die "can't opendir $pathdir_help: $!";

while (defined(my $file = readdir(DIR))) {
	next if (!($file =~ /html$/));
	push @files, $file;
}
closedir(DIR);
my $TDcount=0;
foreach my $file (sort @files) {
#open my $info, $file or die "Could not open $file: $!";
if (exists $ignorehtml{$file}) {next;}

my $fileContent;
open(my $F, '<', $file) or die $!;
binmode($F);
{
 local $/;
 $fileContent = <$F>;
}
close($F);
my $pos=0;
my $lvl=0;
my $cc=0;
my $val="";
my $se="div";
    #div id=href
     $pos=0;
     while ($pos>-1) {
     my $n1=index($fileContent,"id=\"",$pos);
     if ($n1 > -1) {
       my $n2=index($fileContent,"\"",$n1+4);
       my $id_name=substr($fileContent,$n1+4,$n2-$n1-4);
       my $nn=$n1;
       while (substr($fileContent,$nn,1) ne "<") {
          $nn--;
       }
       $nn++;
       ($se)= substr($fileContent,$nn,40) =~ /(\S+)/;

       #print "se=$se id_name=$id_name tag=\n40 sim=".substr($fileContent,$nn,40);
       $n2++;
       $pos=$n2;
       #if (exists $ref{$id_name})
       if (uc($id_name) eq $id_name || (exists $others_func{$id_name} && $se ne "a"))
       {
          my @st=();
          my $n3=$pos;

          while ($n1 > -1) {
            $n1=$pos;
            $n1=index($fileContent,$se,$pos);
            if ($n1 >-1) {
               #print "$n1\n";
               if (substr($fileContent,$n1-1,1) eq "<") {
               # <div>
               push @st, $n1;
               #print "+".substr($fileContent,$n1,20)."\n";
               $n1=$n1+3;
               $pos=$n1;
               next;
               }
               if (substr($fileContent,$n1-1,1) eq "/" && substr($fileContent,$n1-2,1) eq "<") { ## close tag
                  #print "-".substr($fileContent,$n1,20)."\n";
                  if (scalar @st == 0) {
                  if ($se eq "dt") {
                    $n1=index($fileContent,"</dd>",$n1);
                    $n1=$n1+7;
                    #print "".substr($fileContent,$n3+1,$n1-$n3-length($se)-1);
                  }
                  my $r=substr($fileContent,$n3+1,$n1-$n3-length($se));
                  #if (index($id_name,"FUNCTIONS-",0)==0 && $se ne "a") { my $ee=substr($fileContent,$nn,40); print " ->$id_name\n";}
                  if (exists $others_func{$id_name}) {
                    my $id_index=$others_func{$id_name};
                    print "====> other function $id_name add <=====\n";
                      while (exists $others_func{$id_index}) {
                           $id_index=$others_func{$id_index};
                           parseTag($r,$id_index);
                      }
                    $id_index=$others_func{$id_name};
                    parseTag($r,$id_index);
                  } else {
                    $section{$id_name}=$r;
                  }
                  #print "$r \n$id_name\n";
                  #exit;
                  $n1=-1;
                  next;
                  } #0 level end
                  my $nnn=pop @st;
                  $pos=$n1+length($se);
                  next;
               }
               $pos=$n1+length($se);
            }
          }
          $n2=$n2+1;
          print "";
       }
      $pos=$n2;
     } else { $pos=$n1;}
    }
#td func
#print "file $file\n";
$pos=0;
  while( $pos > -1)  {
    #td function
    my $ist=index($fileContent,$keys[$lvl],$pos);
    if ($ist > -1) {
       my $ien=index($fileContent,$keys[$lvl+1],$ist);
       if ($ien >-1) {
           print "Read file $file .. " if $cc==0;
           my $posval=$ist+length($keys[$lvl]);
           if ($lvl>1) {
             $val=substr($fileContent,$ist,$ien-$ist+length($keys[$lvl+1]));
           } else {
             $val=substr($fileContent,$posval,$ien-$posval);
           }
           
           my $idx_name=$lvl/2;
           my $name_sec=$keys_name[$idx_name];
           parseTag($val,$name_sec);
           $pos=$ien+length($keys[$lvl+1]);
           if ($lvl==2) {
             #print " kes idx==2 name_sec=$name_sec \n";
	    }
           $cc++;
       } else {
          die "Not closed string $keys[$lvl+1] .  File : $file position : $ist\n";
       }
    } else {
     $lvl=$lvl+2;
     if (scalar @keys > $lvl) {
        $pos=0; # new keys

     } else {
     $pos=$ist;
     }
     
    }
  }

print " Ok.\n" if $cc > 0;

}

print " TD count $TDcount\n";
open(F, '>', "_func.txt") or die $!;
my $c0=0;
foreach my $key (sort keys %useref) {

 $function_help{$key}=$section{$key};
 #print "section $key\n";
 $c0++;
}
print "Add section $c0\n";
foreach my $key (sort keys %function_help) {
    #print "$key: $function_help{$key}\n";
    print F "#".$key."\n".$function_help{$key}."\n";

}
close(F);
exit;

sub parseTag
{
my $s=shift;
my $defname=shift;
$TDcount++;
my $text;
my $tagname; my $attr; my $vv;
my $pos=0;
my $repl; my $pc;
my $pre=0;
my $fn=0;
my $flit=0;
my $fname;
my $fliteral;
my $href;
my @t=();
 while ($pos <length($s) && $s ne "") {
   if (substr($s,$pos,1) eq "<") {
    if (substr($s,$pos,2) eq "</") {
     # close tag
     $tagname = pop @t;
     #if (length($defname)>0 ) {print "close $tagname\n";}
     my $tmp=substr($s,$pos);
#     print "CLOSE TAG=$tagname pos=$pos\n";
     my ($tagnameC) = $tmp =~ m/<(\/\w+)>/g;
     if (substr($tagname,0,3) eq "pre") {$pre=0;}
     my $startp=$pos;
     $pos=index($s,'>',$pos);
	    if ($pos<0) {
	      die "$s\n NOT CLOSE SIBOL > for tag ${tagname} last=$tmp\n";
	    }
	    $pos++;
     if (exists $tags{$tagname.$tagnameC}) {
            $repl=$tags{$tagname.$tagnameC};
            push @stak, $text, $repl;
        } else {
           if ($href ne "") {
               push @stak, $text, "</a>";
               $useref{$href}=1;
               $href="";
           } else {
             die "$s\n NOT DEFINE REPALCE CLOSE TAG ${tagname}${tagnameC} pos=$pos\n";
           }
        }
    if ($tagname eq "codeclass=\"function\"" && $fn == 1 ) {
      $fn=0;
      $fname=$text;
    }
    if ($tagname eq "codeclass=\"literal\"" && $flit == 1 ) {
      $fliteral=$text;
    }
    $text="";
    next;
    #end close
    }
    my $tmp=substr($s,$pos);
    ($tagname,$attr) = $tmp =~ m/<(\w+)\s*(.*?)>/g;
#    if (length($defname)>0 ) {print "open $tagname pos=$pos\n";}
    my $pp=$pos;
    $pos=index($s,'>',$pos);
    if ($pos<0) {
      die "$s\n NOT CLOSE TAG ${tagname} pos=$pp\n";
    }
    $pos++;
    $href="";
    if ($tagname eq "a" ) {($href) = $attr =~ m/href=".*?#(.*?)"/; $attr=""; }
    if ($tagname eq "sup" ) {$attr="";}
    if ($tagname eq "ul" ) {$attr="";}
    if ($tagname eq "pre" ) {$pre=1;}
    if ($tagname eq "code" && $attr =~ /literal/ ) {$flit++;}
    if ($attr =~ /func_signature/ ) {$fn=1;}
    die "$s TAG NAME EMPTY\n" if $tagname eq "" ;
    if (exists $tags{$tagname.$attr}) {
        $repl=$tags{$tagname.$attr};
        if ($href ne "") {
           $repl="<a href=\"".$href."\">";
           $useref{$href}=1;
           #print "$repl\n";
        }
        push @stak, $text , $repl;
        $text="";
    } else {
       die "$s\n NOT DEFINE REPALCE OPEN TAG ${tagname}${attr}\n";
    }
    #print substr($s,$pos)."\n =============== tag=$tagname, attr=$attr\n";
    push @t, $tagname.$attr;
#    print "TAG=${tagname}${attr} pos=$pos prev=$pp\n";
    next;
   } else {
    my $c=substr($s,$pos,1);
    if ($pc eq $c && $c eq " " && $pre == 0) {

    } else {
       if ($pre == 1 && $c eq "\n") {$c="<br>";}
       $text.=$c;
       $pc=$c;
    }
   }
   $pos++;
 }
 my $rez;
# dump(@stak);
 foreach my $t (@stak) {
  next if $t eq "";
  $rez.=$t;
 }
 if (!defined $fname && defined $fliteral) {
   $fname=lc($fliteral);
   #print "Literal function define $fname\n";
 }
 if (defined $fname) {
 $fname =~ s/&gt;/>/g;
 $fname =~ s/&lt;/</g;
 $fname =~ s/&amp;/&/g;
 $fname=lc($fname);
}
 if (length($defname)>0) {
    $fname=$defname;
  }
 if (defined $fname &&  exists $function_help{$fname}) {
         #print "Dublicate function define $text .Append help\n";
         my $v=$function_help{$fname};
         $function_help{$fname}=$v."<hr>".$rez;
         if ($fname =~ /\||\[/ && length($fname)>3) {
            print "Bad name $fname \n";
         }
         #print "$s\nAppend REZ: $function_help{$fname}\n";
      }
 if (defined $fname &&  !exists $function_help{$fname}) {
         if ($fname =~ /\||\[/ && length($fname)>3) {
            print "Bad name $fname \n";
         }
   $function_help{$fname}=$rez;
  }
 
# print "$s\nREZ: $rez\n";
 @stak=();

#exit;
}
