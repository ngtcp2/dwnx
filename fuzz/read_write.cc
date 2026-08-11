/*
 * dwnx
 *
 * Copyright (c) 2026 dwnx contributors
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include <cstdint>
#include <array>
#include <limits>

#include <fuzzer/FuzzedDataProvider.h>

#include <dwnx/dwnx.h>

#ifdef __cplusplus
extern "C" {
#endif // defined(__cplusplus)

#ifdef __cplusplus
}
#endif // defined(__cplusplus)

namespace {
int recv_transport_params(dwnx_conn *conn, const dwnx_transport_params *params,
                          void *user_data) {
  auto fdp = static_cast<FuzzedDataProvider *>(user_data);

  return fdp->ConsumeBool() ? DWNX_ERR_CALLBACK_FAILURE : 0;
}
} // namespace

namespace {
int recv_stream_data(dwnx_conn *conn, uint32_t flags, int64_t stream_id,
                     uint64_t offset, const uint8_t *data, size_t datalen,
                     void *user_data, void *stream_user_data) {
  auto fdp = static_cast<FuzzedDataProvider *>(user_data);

  return fdp->ConsumeBool() ? DWNX_ERR_CALLBACK_FAILURE : 0;
}
} // namespace

namespace {
int stream_open(dwnx_conn *conn, int64_t stream_id, void *user_data) {
  auto fdp = static_cast<FuzzedDataProvider *>(user_data);

  return fdp->ConsumeBool() ? DWNX_ERR_CALLBACK_FAILURE : 0;
}
} // namespace

namespace {
int stream_close(dwnx_conn *conn, uint32_t flags, int64_t stream_id,
                 uint64_t rx_app_error_code, uint64_t tx_app_error_code,
                 void *user_data, void *stream_user_data) {
  auto fdp = static_cast<FuzzedDataProvider *>(user_data);

  return fdp->ConsumeBool() ? DWNX_ERR_CALLBACK_FAILURE : 0;
}
} // namespace

namespace {
int stream_reset(dwnx_conn *conn, int64_t stream_id, uint64_t final_size,
                 uint64_t app_error_code, void *user_data,
                 void *stream_user_data) {
  auto fdp = static_cast<FuzzedDataProvider *>(user_data);

  return fdp->ConsumeBool() ? DWNX_ERR_CALLBACK_FAILURE : 0;
}
} // namespace

namespace {
int stream_stop_sending(dwnx_conn *conn, int64_t stream_id,
                        uint64_t app_error_code, void *user_data,
                        void *stream_user_data) {
  auto fdp = static_cast<FuzzedDataProvider *>(user_data);

  return fdp->ConsumeBool() ? DWNX_ERR_CALLBACK_FAILURE : 0;
}
} // namespace

namespace {
int recv_stop_sending(dwnx_conn *conn, int64_t stream_id,
                      uint64_t app_error_code, void *user_data,
                      void *stream_user_data) {
  auto fdp = static_cast<FuzzedDataProvider *>(user_data);

  return fdp->ConsumeBool() ? DWNX_ERR_CALLBACK_FAILURE : 0;
}
} // namespace

namespace {
int extend_max_stream_data(dwnx_conn *conn, int64_t stream_id,
                           uint64_t max_data, void *user_data,
                           void *stream_user_data) {
  auto fdp = static_cast<FuzzedDataProvider *>(user_data);

  return fdp->ConsumeBool() ? DWNX_ERR_CALLBACK_FAILURE : 0;
}
} // namespace

namespace {
int extend_max_streams(dwnx_conn *conn, uint64_t max_streams, void *user_data) {
  auto fdp = static_cast<FuzzedDataProvider *>(user_data);

  return fdp->ConsumeBool() ? DWNX_ERR_CALLBACK_FAILURE : 0;
}
} // namespace

namespace {
dwnx_conn *setup_conn(FuzzedDataProvider &fdp, const dwnx_mem &mem) {
  static constexpr dwnx_callbacks callbacks{
    .recv_transport_params = recv_transport_params,
    .recv_stream_data = recv_stream_data,
    .stream_open = stream_open,
    .stream_close = stream_close,
    .stream_reset = stream_reset,
    .stream_stop_sending = stream_stop_sending,
    .recv_stop_sending = recv_stop_sending,
    .extend_max_stream_data = extend_max_stream_data,
    .extend_max_local_streams_bidi = extend_max_streams,
    .extend_max_local_streams_uni = extend_max_streams,
    .extend_max_remote_streams_bidi = extend_max_streams,
    .extend_max_remote_streams_uni = extend_max_streams,
  };

  dwnx_settings settings;
  dwnx_settings_default(&settings);

  dwnx_transport_params params;
  dwnx_transport_params_default(&params);

  params.initial_max_stream_data_bidi_local =
    fdp.ConsumeIntegralInRange<uint64_t>(0, DWNX_MAX_VARINT);
  params.initial_max_stream_data_bidi_remote =
    fdp.ConsumeIntegralInRange<uint64_t>(0, DWNX_MAX_VARINT);
  params.initial_max_stream_data_uni =
    fdp.ConsumeIntegralInRange<uint64_t>(0, DWNX_MAX_VARINT);
  params.initial_max_data =
    fdp.ConsumeIntegralInRange<uint64_t>(0, DWNX_MAX_VARINT);
  params.initial_max_streams_bidi =
    fdp.ConsumeIntegralInRange<uint64_t>(0, DWNX_MAX_VARINT);
  params.initial_max_streams_uni =
    fdp.ConsumeIntegralInRange<uint64_t>(0, DWNX_MAX_VARINT);
  params.max_idle_timeout =
    fdp.ConsumeIntegralInRange<uint64_t>(0, DWNX_MAX_VARINT);
  params.max_record_size = fdp.ConsumeIntegralInRange<uint64_t>(
    DWNX_DEFAULT_MAX_RECORD_SIZE, DWNX_MAX_VARINT);

  dwnx_conn *conn;

  if (fdp.ConsumeBool()) {
    if (dwnx_conn_server_new(&conn, &callbacks, &settings, &params, &mem,
                             &fdp) != 0) {
      return nullptr;
    }
  } else if (dwnx_conn_client_new(&conn, &callbacks, &settings, &params, &mem,
                                  &fdp) != 0) {
    return nullptr;
  }

  return conn;
}
} // namespace

namespace {
void *fuzzed_malloc(size_t size, void *user_data) {
  auto fdp = static_cast<FuzzedDataProvider *>(user_data);

  return fdp->ConsumeBool() ? nullptr : malloc(size);
}
} // namespace

namespace {
void *fuzzed_calloc(size_t nmemb, size_t size, void *user_data) {
  auto fdp = static_cast<FuzzedDataProvider *>(user_data);

  return fdp->ConsumeBool() ? nullptr : calloc(nmemb, size);
}
} // namespace

namespace {
void *fuzzed_realloc(void *ptr, size_t size, void *user_data) {
  auto fdp = static_cast<FuzzedDataProvider *>(user_data);

  return fdp->ConsumeBool() ? nullptr : realloc(ptr, size);
}
} // namespace

constexpr auto nulldata = std::array<uint8_t, 1 << 20>{};

namespace {
void read_write(dwnx_conn *conn, FuzzedDataProvider &fdp, dwnx_tstamp &ts) {
  std::array<uint8_t, 16384> dest;

  while (fdp.remaining_bytes()) {
    ts = fdp.ConsumeIntegralInRange<dwnx_tstamp>(
      ts, std::numeric_limits<dwnx_tstamp>::max() - 1);

    auto datalen = fdp.ConsumeIntegral<size_t>();
    auto data = fdp.ConsumeBytes<uint8_t>(datalen);

    if (dwnx_conn_read(conn, data.data(), data.size(), ts) != 0) {
      return;
    }

    if (fdp.ConsumeBool()) {
      auto stream_id = fdp.ConsumeIntegral<uint64_t>();
      auto datalen = fdp.ConsumeIntegral<uint64_t>();

      if (auto rv =
            dwnx_conn_extend_max_stream_offset(conn, stream_id, datalen);
          dwnx_err_is_fatal(rv)) {
        return;
      }
    }

    if (fdp.ConsumeBool()) {
      auto datalen = fdp.ConsumeIntegral<uint64_t>();

      dwnx_conn_extend_max_offset(conn, datalen);
    }

    auto chunklen = fdp.ConsumeIntegralInRange<size_t>(0, sizeof(nulldata));

    for (;;) {
      if (fdp.remaining_bytes() == 0) {
        return;
      }

      auto flags = fdp.ConsumeIntegral<uint32_t>();

      int64_t stream_id = -1;

      switch (fdp.ConsumeIntegralInRange<int>(0, 2)) {
      case 0:
        stream_id = fdp.ConsumeIntegralInRange<int64_t>(-1, DWNX_MAX_VARINT);

        break;
      case 1:
        if (auto rv = dwnx_conn_open_bidi_stream(conn, &stream_id, nullptr);
            dwnx_err_is_fatal(rv)) {
          return;
        }

        break;
      case 2:
        if (auto rv = dwnx_conn_open_uni_stream(conn, &stream_id, nullptr);
            dwnx_err_is_fatal(rv)) {
          return;
        }

        break;
      }

      dwnx_ssize ndatalen;

      auto nwrite =
        dwnx_conn_write_stream(conn, dest.data(), dest.size(), &ndatalen, flags,
                               stream_id, nulldata.data(), chunklen, ts);
      if (nwrite < 0) {
        switch (nwrite) {
        case DWNX_ERR_WRITE_MORE:
          chunklen = fdp.ConsumeIntegralInRange<size_t>(0, sizeof(nulldata));

          continue;
        case DWNX_ERR_STREAM_DATA_BLOCKED:
        case DWNX_ERR_STREAM_NOT_FOUND:
        case DWNX_ERR_STREAM_SHUT_WR:
          continue;
        }

        return;
      }

      break;
    }
  }
}
} // namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
  FuzzedDataProvider fdp{data, size};

  auto mem = *dwnx_mem_default();
  mem.user_data = &fdp;
  mem.malloc = fuzzed_malloc;
  mem.calloc = fuzzed_calloc;
  mem.realloc = fuzzed_realloc;

  auto conn = setup_conn(fdp, mem);
  if (!conn) {
    return 0;
  }

  dwnx_tstamp ts{};

  read_write(conn, fdp, ts);

  dwnx_conn_del(conn);

  return 0;
}
