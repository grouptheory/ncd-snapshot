#/bin/bash

CSVFILE="output-v.csv"
YDATA=$(cat ploticus-options)
PNG="graphic.png"
export PLOTICUS_PREFABS=/usr/local/prefabs

echo pl -prefab lines data=$CSVFILE delim=comma "yrange= 0 1.25" $YDATA xlbl="Iteration" ylbl="NCD Value" title="Company Simulation" -png -o $PNG
pl -prefab lines data=$CSVFILE delim=comma "yrange= 0 1.25" $YDATA xlbl="Iteration" ylbl="NCD Value" title="Company Simulation" -png -o $PNG
