# The recording transaction is run again after a retry: the provider was
# called once, before it, and is not called again.
seed "INSERT INTO uw_Curl_io_orders VALUES (1, 50, 'ok', 'new', '');"
start
rpc /Curl_io/payRetried/1
charges
rows
show_log
