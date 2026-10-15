# A payment the way the README of urweb-curl has it: claimed, sent, recorded.
# The provider answers, refuses with a 500, or is not there at all; a paid
# order is not charged again, and one that was not sent may be sent again.
seed "INSERT INTO uw_Curl_io_orders VALUES (1, 50, 'ok', 'new', '');" \
     "INSERT INTO uw_Curl_io_orders VALUES (2, 20, 'error', 'new', '');" \
     "INSERT INTO uw_Curl_io_orders VALUES (3, 30, 'closed', 'new', '');"
start
rpc /Curl_io/pay/1
rpc /Curl_io/pay/1
rpc /Curl_io/pay/2
rpc /Curl_io/pay/2
rpc /Curl_io/pay/3
rpc /Curl_io/pay/3
rpc /Curl_io/pay/9
charges
rows
show_log
