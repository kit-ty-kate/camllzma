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
    size_t inbuf_size;
    uint8_t *outbuf;
    size_t outbuf_size;
    lzma_ret last_res;
};

#define DEFAULT_BUF_SIZE 4096
#define LZMA_Stream_val(v) (*((struct camllzma_stream **) Data_custom_val(v)))

static void camllzma_stream_finalize(value stream) {
    struct camllzma_stream *strm = LZMA_Stream_val(stream);
    lzma_end(&strm->strm);
    free(strm->inbuf);
    free(strm->outbuf);
    free(strm);
}

static struct camllzma_stream *camllzma_stream_create(void) {
    struct camllzma_stream *strm = malloc(sizeof(struct camllzma_stream));
    uint8_t *outbuf = malloc(DEFAULT_BUF_SIZE * sizeof(uint8_t));
    uint8_t *inbuf = malloc(DEFAULT_BUF_SIZE * sizeof(uint8_t));
    lzma_stream tmp = LZMA_STREAM_INIT;
    if (strm == NULL || outbuf == NULL || inbuf == NULL) {
        caml_raise_out_of_memory();
    }
    strm->strm = tmp;
    strm->last_res = LZMA_OK;

    strm->inbuf = inbuf;
    strm->inbuf_size = DEFAULT_BUF_SIZE;
    strm->strm.next_in = inbuf;
    strm->strm.avail_in = 0;

    strm->outbuf = outbuf;
    strm->outbuf_size = DEFAULT_BUF_SIZE;
    strm->strm.next_out = outbuf;
    strm->strm.avail_out = DEFAULT_BUF_SIZE;

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

value camllzma_new_decoder(value memlimit, value flags) {
    CAMLparam2(memlimit, flags);
    CAMLlocal1(res);

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

    struct camllzma_stream *strm = camllzma_stream_create();

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

value camllzma_new_encoder(value preset, value check, value unsupported_check_exn) {
    CAMLparam3(preset, check, unsupported_check_exn);
    CAMLlocal1(res);

    uint32_t lzma_preset = 0;
    lzma_check lzma_check = 0;

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

    switch (Int_val(check)) {
    case 0: lzma_check = LZMA_CHECK_NONE; break;
    case 1: lzma_check = LZMA_CHECK_CRC32; break;
    case 2: lzma_check = LZMA_CHECK_CRC64; break;
    case 3: lzma_check = LZMA_CHECK_SHA256; break;
    default:
        caml_failwith("camllzma_encoder/check: There is a bug in camllzma. Please report.");
        break;
    }

    struct camllzma_stream *strm = camllzma_stream_create();

    switch (lzma_easy_encoder(&strm->strm, lzma_preset, lzma_check)) {
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

value camllzma_push(value stream, value str) {
    CAMLparam2(stream, str);

    struct camllzma_stream *strm = LZMA_Stream_val(stream);
    const size_t len = caml_string_length(str);
    uint8_t *next_in = strm->inbuf;
    const size_t old_len = strm->strm.avail_in;
    const size_t avail_in = len + old_len;

    if (avail_in > strm->inbuf_size || next_in == NULL) {
        next_in = malloc(avail_in * sizeof(uint8_t));

        if (next_in == NULL) {
            caml_raise_out_of_memory();
        }
        if (strm->inbuf != NULL) {
            free(strm->inbuf);
        }

        memcpy(next_in, strm->strm.next_in, old_len);
        strm->inbuf = next_in;
        strm->inbuf_size = avail_in;
    }

    strm->strm.next_in = memcpy(next_in + old_len, &Byte(str, 0), len);
    strm->strm.avail_in = avail_in;

    CAMLreturn(Val_unit);
}

value camllzma_pop(value stream) {
    CAMLparam1(stream);
    CAMLlocal1(str);

    struct camllzma_stream *strm = LZMA_Stream_val(stream);
    const size_t len = strm->outbuf_size - strm->strm.avail_out;

    if(strm->strm.avail_out == 0 || strm->last_res == LZMA_STREAM_END) {
        str = caml_alloc_initialized_string(len, strm->outbuf);
        strm->strm.next_out = strm->outbuf;
        strm->strm.avail_out = strm->outbuf_size;
        strm->last_res = LZMA_OK;
    } else {
        str = caml_alloc_initialized_string(0, "");
    }

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

    switch ((strm->last_res = lzma_code(&strm->strm, act))) {
    case LZMA_OK: res = caml_hash_variant("OK"); break;
    case LZMA_STREAM_END: res = caml_hash_variant("STREAM_END"); break;
    case LZMA_GET_CHECK:
        res = caml_alloc(2, 0);
        Store_field(res, 0, caml_hash_variant("GET_CHECK"));
        switch (lzma_get_check(&strm->strm)) {
        case LZMA_CHECK_NONE: Store_field(res, 1, Val_int(0)); break;
        case LZMA_CHECK_CRC32: Store_field(res, 1, Val_int(1)); break;
        case LZMA_CHECK_CRC64: Store_field(res, 1, Val_int(2)); break;
        case LZMA_CHECK_SHA256: Store_field(res, 1, Val_int(3)); break;
        default:
            caml_failwith("camllzma_next/GET_CHECK: There is a bug in camllzma. Please report.");
            break;
        }
        break;
    case LZMA_NO_CHECK: res = caml_hash_variant("NO_CHECK"); break;
    case LZMA_MEMLIMIT_ERROR: res = caml_hash_variant("MEMLIMIT_ERROR"); break;
    case LZMA_FORMAT_ERROR: res = caml_hash_variant("FORMAT_ERROR"); break;
    case LZMA_DATA_ERROR: res = caml_hash_variant("CORRUPT_DATA"); break;
    case LZMA_BUF_ERROR:
        if (strm->strm.avail_in == 0) {
            res = caml_hash_variant("INPUT_NEEDED");
        } else {
            res = caml_hash_variant("OUTPUT_BUFFER_TOO_SMALL");
        }
        break;
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
