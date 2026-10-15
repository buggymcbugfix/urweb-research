# The provider answers after the request's timeout: the outcome is unknown
# (MaybeSent), and the order stays open.  The provider did charge it, so the
# second attempt gets the first charge's answer back, by the idempotency key,
# and the order is paid once.
seed "INSERT INTO uw_Curl_io_orders VALUES (4, 70, 'slow', 'new', '');"
start
rpc /Curl_io/pay/4
charges
sleep 3
charges
rpc /Curl_io/pay/4
charges
rows
show_log
