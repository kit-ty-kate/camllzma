module Lzma2 = Camllzma.Lzma2

let fmt = Printf.sprintf

let list_of_bin s =
  let len = String.length s in
  let rec aux acc i =
    if i < len then
      aux (int_of_char s.[i] :: acc) (succ i)
    else
      acc
  in
  List.rev (aux [] 0)

let printable_bin content =
  let content = list_of_bin content in
  let content = List.map (fmt "%02X") content in
  let content = String.concat " " content in
  content

let expected_content = "FD 37 7A 58 5A 00 00 04 E6 D6 B4 46 02 00 21 01 \
                        16 00 00 00 74 2F E5 A3 01 00 03 74 65 73 74 00 \
                        A5 75 0C C1 A7 FD 15 FA 00 01 1C 04 6F 2C 9C C1 \
                        1F B6 F3 7D 01 00 00 00 00 04 59 5A"

let () =
  let buf = Buffer.create 1024 in
  print_endline "Creating encoder...";
  let e = Lzma2.encoder ~bufsize:1024 Lzma2.PRESET_DEFAULT Lzma2.CHECK_CRC64 in
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
        let content = Buffer.contents buf in
        let content = printable_bin content in
        print_endline (fmt "Outbuf: %s" content);
        print_endline (fmt "Expect: %s" expected_content);
        if String.equal content expected_content then
          print_endline "Expected content: ok"
        else begin
          print_endline "Expected content: failed";
          exit 2;
        end
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
