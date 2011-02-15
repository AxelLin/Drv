#!/bin/sh
rmmod hi3511_pciv2trans
rmmod hi3511_pciv

insmod hi3511_pciv.ko 
insmod hi3511_pciv2trans.ko
./pciv_test_slave
