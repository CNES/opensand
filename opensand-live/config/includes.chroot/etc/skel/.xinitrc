#! /bin/bash

openbox & wmpid=$!
/usr/bin/rox -p Default

/usr/bin/opensand-live &

wait $wmpid
