#!/bin/bash
#usage:./scripts/auto_build_test.sh at rtos sdk root dir

set -e

#add board and product in the list if there is new building pair
board_product_pair_list=(
	"au401_a213y speaker"
	"ad409_a113l speaker"
	"ad401_a113l speaker"
	"ad403_a113l speaker"
	"am301_t950d4 aocpu"
	"at309_t962d4 aocpu"
	"at301_t962d4 aocpu"
)

#build test for every pair until there is error occurred
len=${#board_product_pair_list[@]}
for((index=0;index<len;index++))
do
	pair=(${board_product_pair_list[index]})
	BOARD=${pair[0]}
	PRODUCT=${pair[1]}
	make distclean
	source scripts/env.sh $BOARD $PRODUCT
	make
done
