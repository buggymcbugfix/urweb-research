# A commit that fails after the database's COMMIT, on a server with an
# onError handler: a transactional's commit callback reports an error, and
# the handler answers the request.  The pruner should go on as before: A,
# made first and then left alone, is expunged once it has had no contact
# for 4 s, which the clientLeaves task logs, and a poll of A's is then
# refused.
start
client A
page /Concurrent/failCommit
show_log
wait_for "a client left"
if curl_msgs A | tidy | grep -q '^R$'; then
  echo "a poll of A: refused"
else
  echo "a poll of A: answered, A is still there"
fi
