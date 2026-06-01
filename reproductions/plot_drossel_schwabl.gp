set datafile separator comma
set terminal pngcairo size 1200,800
set output "figures/generated/drossel_schwabl_fire_sizes.png"
set title "Drossel-Schwabl fire-size distribution"
set xlabel "fire size"
set ylabel "count"
set logscale xy
set grid
plot "results/drossel_schwabl_fire_sizes.csv" using 1:3 every ::1 with points pointtype 7 pointsize 0.45 title "de-biased n(s), 256x256, 50K measured steps"
