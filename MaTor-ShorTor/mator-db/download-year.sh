#!/bin/bash
echo -n "Enter the year you would like to download Tor data for: "
read YEAR

for MONTH in 01 02 03 04 05 06 07 08 09 10 11 12
do
	./mator-db "$YEAR-$MONTH"
done
echo "Done!"