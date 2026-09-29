# The provider closes the connection without an answer: the outcome is
# unknown, and nothing was charged.
seed "INSERT INTO uw_Curl_io_orders VALUES (6, 60, 'drop', 'new', '');"
start
rpc /Curl_io/pay/6
charges
rows
show_log
