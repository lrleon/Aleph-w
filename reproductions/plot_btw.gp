set datafile separator comma
set terminal pngcairo size 1200,800
set output "figures/generated/btw_avalanche_sizes.png"
set title "Bak-Tang-Wiesenfeld avalanche-size distribution"
set xlabel "avalanche size"
set ylabel "count"
set logscale xy
set grid
plot "results/btw_avalanche_sizes.csv" using 1:2 every ::1 with points pointtype 7 pointsize 0.45 title "64x64, 100K drops"
