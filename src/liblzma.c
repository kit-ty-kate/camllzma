#define CAML_NAME_SPACE

#include <caml/mlvalues.h>
#include <caml/alloc.h>
#include <caml/memory.h>
#include <caml/fail.h>
#include <caml/custom.h>
#include <string.h>
#include <lzma.h>

struct camllzma_stream {
    lzma_stream strm;
    uint8_t *inbuf;
    uint8_t *outbuf;
    size_t outbuf_size;
};

#define LZMA_Stream_val(v) (*((struct camllzma_stream **) Data_custom_val(v)))

static void camllzma_stream_finalize(value stream) {
    CAMLparam1(stream);

    struct camllzma_stream *strm = LZMA_Stream_val(stream);
    lzma_end(&strm->strm);
    if (strm->inbuf != NULL) {
        free(strm->inbuf);
    }
    free(strm->outbuf);
    free(strm);
}

static struct camllzma_stream *camllzma_stream_create(size_t bufsize) {
    struct camllzma_stream *strm = malloc(sizeof(struct camllzma_stream));
    uint8_t *outbuf = malloc(bufsize * sizeof(uint8_t));
    lzma_stream tmp = LZMA_STREAM_INIT;
    if (strm == NULL || outbuf == NULL) {
        caml_raise_out_of_memory();
    }
    strm->strm = tmp;
    strm->inbuf = NULL;
    strm->outbuf = outbuf;
    strm->outbuf_size = bufsize;

    return strm;
}

static struct custom_operations stream_ops = {
  "camllzma.lzma_stream",
  camllzma_stream_finalize,
  custom_compare_default,
  custom_hash_default,
  custom_serialize_default,
  custom_deserialize_default,
  custom_compare_ext_default,
  custom_fixed_length_default
};

value camllzma_decoder(value bufsize, value memlimit, value flags) {
    CAMLparam2(memlimit, flags);
    CAMLlocal1(res);

    const size_t outbuf_size = Int_val(bufsize);
    const uint64_t lzma_memlimit = Int64_val(memlimit);
    uint32_t lzma_flags = 0;

    while (flags != Val_emptylist) {
        switch (Int_val(Field(flags, 0))) {
        case 0: lzma_flags |= LZMA_TELL_NO_CHECK; break;
        case 1: lzma_flags |= LZMA_TELL_UNSUPPORTED_CHECK; break;
        case 2: lzma_flags |= LZMA_TELL_ANY_CHECK; break;
        case 3: lzma_flags |= LZMA_IGNORE_CHECK; break;
        case 4: lzma_flags |= LZMA_CONCATENATED; break;
        default:
          caml_failwith("camllzma_decoder/flags: There is a bug in camllzma. Please report.");
          break;
        }
        flags = Field(flags, 1);
    }

    struct camllzma_stream *strm = camllzma_stream_create(outbuf_size);

    switch (lzma_stream_decoder(&strm->strm, lzma_memlimit, lzma_flags)) {
    case LZMA_OK:
        res = caml_alloc_custom(&stream_ops, sizeof(lzma_stream*), 0, 1);
        LZMA_Stream_val(res) = strm;
        break;
    case LZMA_MEM_ERROR:
        caml_raise_out_of_memory();
        break;
    case LZMA_OPTIONS_ERROR:
        caml_invalid_argument("camllzma_decoder");
        break;
    case LZMA_PROG_ERROR:
        caml_failwith("lzma_stream_decoder/LZMA_PROG_ERROR: There is a bug in camllzma. Please report.");
        break;
    default:
        caml_failwith("lzma_stream_decoder: There is a bug in camllzma. Please report.");
        break;
    }

    CAMLreturn(res);
}

value camllzma_encoder(value outbuf_size, value preset, value unsupported_check_exn) {
    CAMLparam2(preset, unsupported_check_exn);
    CAMLlocal1(res);

    const size_t bufsize = Int_val(outbuf_size);
    uint32_t lzma_preset = 0;

    if (Is_long(preset)) {
        switch (Int_val(preset)) {
        case 0: lzma_preset = LZMA_PRESET_DEFAULT; break;
        default:
            caml_failwith("camllzma_encoder/preset: There is a bug in camllzma. Please report.");
            break;
        }
    } else {
        switch (Tag_val(preset)) {
        case 0: lzma_preset = Field(preset, 0); break;
        case 1: lzma_preset = Field(preset, 0) | LZMA_PRESET_EXTREME; break;
        default:
            caml_failwith("camllzma_encoder/preset: There is a bug in camllzma. Please report.");
            break;

        }
    }

    struct camllzma_stream *strm = camllzma_stream_create(outbuf_size);

    switch (lzma_easy_encoder(&strm->strm, preset, LZMA_CHECK_CRC64)) {
    case LZMA_OK:
        res = caml_alloc_custom(&stream_ops, sizeof(lzma_stream*), 0, 1);
        LZMA_Stream_val(res) = strm;
        break;
    case LZMA_UNSUPPORTED_CHECK:
        caml_raise(unsupported_check_exn);
        break;
    case LZMA_MEM_ERROR:
        caml_raise_out_of_memory();
        break;
    case LZMA_OPTIONS_ERROR:
        caml_invalid_argument("camllzma_encoder");
        break;
    case LZMA_PROG_ERROR:
        caml_failwith("lzma_stream_encoder/LZMA_PROG_ERROR: There is a bug in camllzma. Please report.");
        break;
    default:
        caml_failwith("lzma_stream_encoder: There is a bug in camllzma. Please report.");
        break;
    }

    CAMLreturn(res);
}

value camllzma_inbuf_is_empty(value stream) {
    CAMLparam1(stream);

    const struct camllzma_stream *strm = LZMA_Stream_val(stream);

    CAMLreturn(Val_bool(strm->strm.avail_in == 0));
}

value camllzma_inbuf_set(value stream, value str) {
    CAMLparam2(stream, str);

    struct camllzma_stream *strm = LZMA_Stream_val(stream);
    const size_t avail_in = caml_string_length(str);
    uint8_t *next_in = malloc(avail_in * sizeof(uint8_t));

    if (next_in == NULL) {
        caml_raise_out_of_memory();
    }
    if (strm->inbuf != NULL) {
        free(strm->inbuf);
    }

    strm->inbuf = next_in;
    strm->strm.next_in = memcpy(next_in, &Byte(str, 0), avail_in);
    strm->strm.avail_in = avail_in;

    CAMLreturn(Val_unit);
}

value camllzma_outbuf_clear(value stream) {
    CAMLparam1(stream);

    struct camllzma_stream *strm = LZMA_Stream_val(stream);
    strm->strm.next_out = strm->outbuf;
    strm->strm.avail_out = strm->outbuf_size;

    CAMLreturn(Val_unit);
}

value camllzma_outbuf(value stream) {
    CAMLparam1(stream);
    CAMLlocal1(str);

    const struct camllzma_stream *strm = LZMA_Stream_val(stream);
    const size_t len = strm->outbuf_size - strm->strm.avail_out;

    str = caml_alloc_string(len);
    memcpy(&Byte(str, 0), strm->outbuf, len);

    CAMLreturn(str);
}

value camllzma_next(value stream, value action, value unsupported_check_exn) {
    CAMLparam3(stream, action, unsupported_check_exn);
    CAMLlocal1(res);

    struct camllzma_stream *strm = LZMA_Stream_val(stream);
    lzma_action act = 0;

    switch (Int_val(action)) {
    case 0: act = LZMA_RUN; break;
    case 1: act = LZMA_SYNC_FLUSH; break;
    case 2: act = LZMA_FULL_FLUSH; break;
    case 3: act = LZMA_FULL_BARRIER; break;
    case 4: act = LZMA_FINISH; break;
    default:
        caml_failwith("camllzma_next/action: There is a bug in camllzma. Please report.");
        break;
    }

    switch (lzma_code(&strm->strm, act)) {
    case LZMA_OK: res = caml_alloc(1, 0); Store_field(res, 0, Val_int(0)); break;
    case LZMA_STREAM_END: res = caml_alloc(1, 0); Store_field(res, 0, Val_int(1)); break;
    case LZMA_GET_CHECK: res = caml_alloc(1, 0); Store_field(res, 0, Val_int(2)); break;
    case LZMA_NO_CHECK: res = caml_alloc(1, 1); Store_field(res, 0, Val_int(0)); break;
    case LZMA_MEMLIMIT_ERROR: res = caml_alloc(1, 1); Store_field(res, 0, Val_int(1)); break;
    case LZMA_FORMAT_ERROR: res = caml_alloc(1, 1); Store_field(res, 0, Val_int(2)); break;
    case LZMA_DATA_ERROR: res = caml_alloc(1, 1); Store_field(res, 0, Val_int(3)); break;
    case LZMA_BUF_ERROR: res = caml_alloc(1, 1); Store_field(res, 0, Val_int(4)); break;
    case LZMA_UNSUPPORTED_CHECK:
        caml_raise(unsupported_check_exn);
        break;
    case LZMA_MEM_ERROR:
        caml_raise_out_of_memory();
        break;
    case LZMA_OPTIONS_ERROR:
        caml_invalid_argument("camllzma_next");
        break;
    case LZMA_PROG_ERROR:
        caml_failwith("lzma_code/LZMA_PROG_ERROR: There is a bug in camllzma. Please report.");
        break;
    default:
        caml_failwith("lzma_code: There is a bug in camllzma. Please report.");
        break;
    }

    CAMLreturn(res);
}
