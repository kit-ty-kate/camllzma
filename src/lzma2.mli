type stream

type flag =
  | TELL_NO_CHECK
  | TELL_UNSUPPORTED_CHECK
  | TELL_ANY_CHECK
  | IGNORE_CHECK
  | CONCATENATED

val new_decoder : memlimit:int64 -> flag list -> stream

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

val new_encoder : preset -> check -> stream

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

val compress : stream -> action -> compression_status

type decompression_status = [
  | compression_status
  | `GET_CHECK of check
  | `NO_CHECK
  | `MEMLIMIT_ERROR
  | `FORMAT_ERROR
]

val decompress : stream -> action -> decompression_status

val push : stream -> string -> unit
val pop : stream -> string
