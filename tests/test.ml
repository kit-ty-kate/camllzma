module Lzma2 = Camllzma.Lzma2

let fmt = Printf.sprintf

let () =
  let buf = Buffer.create 1024 in
  print_endline "Creating encoder...";
  let e = Lzma2.encoder ~bufsize:1024 Lzma2.PRESET_DEFAULT in
  print_endline "Setting inbuf...";
  Lzma2.inbuf_set e "test";
  print_endline "Clearing outbuf...";
  Lzma2.outbuf_clear e;
  let rec aux action =
    print_endline "Processing...";
    match Lzma2.next e action with
    | Ok Lzma2.OK ->
        print_endline "Getting partial outbuf...";
        Buffer.add_string buf (Lzma2.outbuf e);
        Lzma2.outbuf_clear e;
        aux (if Lzma2.inbuf_is_empty e then Lzma2.FINISH else Lzma2.RUN)
    | Ok Lzma2.STREAM_END ->
        print_endline "Stream ended...";
        Buffer.add_string buf (Lzma2.outbuf e);
        print_endline (fmt "Outbuf: '%s'" (Buffer.contents buf));
    | Ok Lzma2.GET_CHECK
    | Error Lzma2.NO_CHECK
    | Error Lzma2.MEMLIMIT_ERROR
    | Error Lzma2.FORMAT_ERROR
    | Error Lzma2.DATA_ERROR
    | Error Lzma2.BUF_ERROR ->
        print_endline "Something went wrong!!";
        exit 1
  in
  aux Lzma2.RUN
