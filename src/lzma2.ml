type stream

type flag =
  | TELL_NO_CHECK
  | TELL_UNSUPPORTED_CHECK
  | TELL_ANY_CHECK
  | IGNORE_CHECK
  | CONCATENATED

external decoder : int -> int64 -> flag list -> stream = "camllzma_decoder"
let decoder ~bufsize ~memlimit flags = decoder bufsize memlimit flags

exception UNSUPPORTED_CHECK

type preset =
  | PRESET_DEFAULT
  | PRESET_CUSTOM of int
  | PRESET_EXTREME of int

type check =
  | CHECK_NONE
  | CHECK_CRC32
  | CHECK_CRC64
  | CHECK_SHA256

external encoder : int -> preset -> check -> exn -> stream = "camllzma_encoder"
let encoder ~bufsize preset check = encoder bufsize preset check UNSUPPORTED_CHECK

type res =
  | OK
  | STREAM_END
  | GET_CHECK

type errors =
  | NO_CHECK
  | MEMLIMIT_ERROR
  | FORMAT_ERROR
  | DATA_ERROR
  | BUF_ERROR

type action =
  | RUN
  | SYNC_FLUSH
  | FULL_FLUSH
  | FULL_BARRIER
  | FINISH

external next : stream -> action -> exn -> (res, errors) result = "camllzma_next"
let next strm action = next strm action UNSUPPORTED_CHECK

external inbuf_is_empty : stream -> bool = "camllzma_inbuf_is_empty"
external inbuf_set : stream -> string -> unit = "camllzma_inbuf_set"

external outbuf_clear : stream -> unit = "camllzma_outbuf_clear"
external outbuf : stream -> string = "camllzma_outbuf"
