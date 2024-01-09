#!/bin/bash
directories=("5200" "5201" "5202" "5203" "5204" "5205")
current_directory=$(pwd)
for dir in "${directories[@]}"; do
    cd "$current_directory/$dir" || exit
    gnome-terminal --tab --working-directory="$current_directory/$dir" --command "./Storage"
done
