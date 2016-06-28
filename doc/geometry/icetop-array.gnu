set encoding utf8
geom="geometry_reduced.txt"

set size ratio -1

set grid

set xrange [-650:650]
set xlabel "x (m)"

set yrange [-550:550]
set ylabel "y (m)"

set key off

set term svg enhanced size 800,600
set output "icetop-array.svg"

#plot geom u (($1<79)*($2==61)?$3:1/0):4:(10) w circles lc rgb "black" title "Base array",\
#     geom u (($1<79)*($2==61)?$3:1/0):4:1 w labels right font ",7" offset -1,0.2 notitle,\
#     geom u (($1>78)*($2==61)?$3:1/0):4:(10) w circles lc rgb "gray" title "Infill array",\
#     geom u (($1>78)*($2==61)?$3:1/0):4:1 w labels left font ",7" offset 1,-.2 textcolor rgb "gray" notitle

plot geom u (($1<79)?$2:1/0):3:(10) w circles lc rgb "black" title "Base array",\
     geom u (($1<79)?$2:1/0):3:1 w labels right font ",7" offset -0.9,0.2 notitle,\
     geom u (($1>78)?$2:1/0):3:(10) w circles lc rgb "gray" title "Infill array",\
     geom u (($1>78)?$2:1/0):3:1 w labels left font ",7" offset 0.9,-.2 textcolor rgb "gray" notitle

set term pngcairo enhanced size 800,600
set output "icetop-array.png"
replot
