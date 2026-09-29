Basis.io_rpc and Basis.io_tryRpc: an RPC whose server function is an io
computation.  The runs send the requests the browser's `rpc` sends (POST,
the arguments in the path, the client named by the UrWeb-Client and
UrWeb-Pass headers), and read the client's messages afterwards.  The C
probes are those of ../io_task.
