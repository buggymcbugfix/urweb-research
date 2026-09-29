# The recording transaction fails for good after the provider has charged:
# the request fails, the order stays claimed, and a later payment is refused.
# What the application knows about the charge is in the provider's hands.
seed "INSERT INTO uw_Curl_io_orders VALUES (1, 50, 'ok', 'new', '');"
start
rpc /Curl_io/payThenFail/1
rpc /Curl_io/pay/1
charges
rows
show_log
