#!/bin/sh
rmmod hi3511_pciv2trans
rmmod hi3511_pciv

insmod hi3511_pciv.ko is_host=1
insmod hi3511_pciv2trans.ko
./pciv_test_host 1
