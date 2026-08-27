The dwnx programmers' guide
===========================

This document describes a basic usage of dwnx library and common
pitfalls which programmers might encounter.

Initialization
--------------

The :type:`dwnx_conn` is an opaque object that corresponds to a single
QMux connection.  If the endpoint is a client, use
`dwnx_conn_client_new` to initialized it.  For a server, use
`dwnx_conn_server_new` instead.

These functions take the following objects:

- :type:`dwnx_callbacks`
- :type:`dwnx_settings`
- :type:`dwnx_transport_params`

The :type:`dwnx_callbacks` stores the callbacks that are called
throughout the lifetime of the connection.  The mandatory callbacks
are:

- :member:`dwnx_callbacks.rand`: This callback is called when the
  library needs the unpredictable bytes of data.

The remaining callbacks are optional.

The :type:`dwnx_settings` stores the configuration of the QMux
connection.  It should be first initialized by
`dwnx_settings_default`.  :member:`dwnx_settings.conn_id` is the
identifier of this connection.  This is not a part of QMux protocol,
and just used in logging purpose only to identify the particular
connection.  :member:`dwnx_settings.initial_ts` is the initial
timestamp passed to :type:`dwnx_conn`.  The timestamp is updated by
calling the functions that take :type:`dwnx_tstamp` during the
connection lifetime.

The :type:`dwnx_transport_params` stores the local QUIC transport
parameters that are sent to the remote endpoint.  It should be
initialized by `dwnx_transport_params_default`.  For HTTP/3 server, at
least the following fields should be nonzero to allow the client to
create a stream and send data:

- :member:`dwnx_transport_params.initial_max_stream_data_bidi_remote`
- :member:`dwnx_transport_params.initial_max_stream_data_uni`
- :member:`dwnx_transport_params.initial_max_data`
- :member:`dwnx_transport_params.initial_max_streams_bidi`
- :member:`dwnx_transport_params.initial_max_streams_uni`

For HTTP/3 client, the following fields should be nonzero to allow the
server to create a stream and send data:

- :member:`dwnx_transport_params.initial_max_stream_data_bidi_local`
- :member:`dwnx_transport_params.initial_max_stream_data_uni`
- :member:`dwnx_transport_params.initial_max_data`
- :member:`dwnx_transport_params.initial_max_streams_uni`

:member:`dwnx_transport_params.max_record_size` is basically ignored,
and the library always uses 16384.

Read the transport data
-----------------------

To pass the data read from the underlying reliable transport, call
`dwnx_conn_read` with the current timestamp.  The application can pass
the byte stream to this function without worrying about the QMux
record boundary.  The function processes the data in streaming
fashion.  If it returns the negative error code, close the connection.

After reading data, try writing data by calling
`dwnx_conn_writev_stream`.  See below.

Write a QMux record
-------------------

To write a QMux record, call `dwnx_conn_writev_stream`.  It writes a
single QMux record to the given buffer.  It can accept the stream ID
and its stream data to send.  The application should handle the
following negative error code gracefully:

- :macro:`DWNX_ERR_STREAM_DATA_BLOCKED`: The stream data cannot be
  sent due to the flow control limit.  Try sending the stream data of
  the another stream if any.
- :macro:`DWNX_ERR_STREAM_SHUT_WR`: The send side of the stream has
  been closed.  Try sending the stream data of the another stream if
  any.
- :macro:`DWNX_ERR_WRITE_MORE`: The application is allowed to write
  more stream data if any.

If the one of the above negative error codes are returned, call
`dwnx_conn_writev_stream` again with the same dest and destlen
parameters.

In general, the application must keep calling
`dwnx_conn_writev_stream` repeatedly until it returns the integer >=
0, or the negative error codes other than the ones listed above.  If 0
is returned, no QMux record is produced, which means that there is no
data to send.  If the integer > 0 is returned, the QMux record is
completely written to the given buffer and its length is the returned
value.  If one of the negative error codes other than the ones listed
above is returned, close the connection.

When the stream data is written, the number of data written is
assigned to the object pointed by pdatalen.  Because
`dwnx_conn_writev_stream` may close the stream, when the function
returns, the stream has already been closed.  To make sure that the
number of the written data is notified to the application, use
:member:`dwnx_callbacks.write_stream_data_offset` callback.

The buffer size should be 16384 for the optimal performance if the
underlying transport is TLS.  The application may choose the smaller
sized buffer while the congestion window is smaller than the maximum
TLS record to avoid the additional round trip for decryption.

Connection close
----------------

The application can close the connection simply by closing the
underlying transport connection.

It can also send ``CONNECTION_CLOSE`` frame with the error code and
the reason phrase.  `dwnx_conn_write_connection_close` writes this
final QMux record containing ``CONNECTION_CLOSE`` frame in the given
buffer.  The application can send this final record, and then close
the underlying connection.

If the connection is closed because of idle timeout, it is strongly
recommended just close the underlying connection without sending
``CONNECTION_CLOSE`` frame.

Timeout
-------

To get the next timeout, call `dwnx_conn_get_expiry`.  It should be
called after reading or writing data.  It is not necessary to call
this function each time after calling `dwnx_conn_read` or
`dwnx_conn_writev_stream`.  In typical scenario, call `dwnx_conn_read`
repeatedly until all data is drained from the socket, and then call
`dwnx_conn_get_expiry`.  Similarly, call `dwnx_conn_writev_stream`
repeatedly to write the data until the write is blocked by the
underlying transport.  Then call `dwnx_conn_get_expiry`.

When the timer has fired, call `dwnx_conn_handle_expiry`.  If it
returns :macro:`DWNX_ERR_IDLE_CLOSE`, the connection has been
quiescent too long.  The underlying transport should be closed without
sending ``CONNECTION_CLOSE`` frame.
