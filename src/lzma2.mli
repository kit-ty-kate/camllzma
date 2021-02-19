type stream

type flag =
  | TELL_NO_CHECK
  | TELL_UNSUPPORTED_CHECK
  | TELL_ANY_CHECK
  | IGNORE_CHECK
  | CONCATENATED

val decoder : bufsize:int -> memlimit:int64 -> flag list -> stream

exception UNSUPPORTED_CHECK

type preset =
  | PRESET_DEFAULT
  | PRESET_CUSTOM of int
  | PRESET_EXTREME of int

val encoder : bufsize:int -> preset -> stream

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

val next : stream -> action -> (res, errors) result

val inbuf_is_empty : stream -> bool
val inbuf_set : stream -> string -> unit

val outbuf_clear : stream -> unit
val outbuf : stream -> string
