type stream

type flag =
  | TELL_NO_CHECK
  | TELL_UNSUPPORTED_CHECK
  | TELL_ANY_CHECK
  | IGNORE_CHECK
  | CONCATENATED

external new_decoder : int64 -> flag list -> stream = "camllzma_new_decoder"
let new_decoder ~memlimit flags = new_decoder memlimit flags

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

external new_encoder : preset -> check -> exn -> stream = "camllzma_new_encoder"
let new_encoder preset check = new_encoder preset check UNSUPPORTED_CHECK

type action =
  | RUN
  | SYNC_FLUSH
  | FULL_FLUSH
  | FULL_BARRIER
  | FINISH

type compression_status = [
  | `OK
  | `STREAM_END
  | `INPUT_NEEDED
  | `OUTPUT_BUFFER_TOO_SMALL
  | `CORRUPT_DATA
]

type decompression_status = [
  | compression_status
  | `GET_CHECK of check
  | `NO_CHECK
  | `MEMLIMIT_ERROR
  | `FORMAT_ERROR
]

external next : stream -> action -> exn -> decompression_status = "camllzma_next"
let next strm action = next strm action UNSUPPORTED_CHECK

let compress strm action = match next strm action with
  | #compression_status as x -> x
  | _ -> assert false
let decompress strm action = next strm action

external push : stream -> string -> unit = "camllzma_push"
external pop : stream -> string = "camllzma_pop"
