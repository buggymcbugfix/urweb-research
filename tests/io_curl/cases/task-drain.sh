# A periodic io task drains a queue of jobs: a GET per job outside any
# transaction, the answer recorded in a transaction of its own.
seed "INSERT INTO uw_Curl_io_jobs VALUES (1, 'queued', '');" \
     "INSERT INTO uw_Curl_io_jobs VALUES (2, 'queued', '');" \
     "INSERT INTO uw_Curl_io_jobs VALUES (3, 'queued', '');"
start
wait_for "drained job 3"
rows
