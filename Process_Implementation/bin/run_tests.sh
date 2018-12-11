#!/bin/bash

echo "Creating Server";
./gameserver -p 2 -i inventory.txt -q 15&
sleep 3;
echo "Connecting First Player";
konsole --noclose -e ./player -n First -i p1_inventory.txt localhost&
sleep 4;
echo "Connecting Second Player";
konsole --noclose -e ./player -n Second -i p1_inventory.txt localhost&
sleep 5;
echo "Connecting Third Player";
konsole --noclose -e ./player -n Third -i p1_inventory.txt localhost&