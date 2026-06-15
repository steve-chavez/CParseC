// A more efficient CSV parser than bench/c/CParseCSV.c, using SIMD parsers
#define CPC_USE_MEMCHR
#include "csv.h"

// Strip the outer quotes and collapse doubled quotes inside one quoted field
static CpcResult unescape_quoted(__attribute__((unused)) CpcArena *A, const CpcValue *v,
                                 CpcSlice rest) {
  // TODO should not happen
  if (!v || v->kind != CPC_SLICE) return cpc_res_err(rest, "csv: expected slice");

  CpcSlice s = v->as.slice;
  // Leave non-quoted slices unchanged, only unescape fully quoted
  if (s.len < 2 || s.ptr[0] != '"' || s.ptr[s.len - 1] != '"')
    return cpc_res_ok(cpc_val_slice(s), rest);

  char  *out = (char *)s.ptr + 1;
  size_t dst = 0;
  size_t src = 1;
  size_t end = s.len - 1;
  while (src < end) {
    char c = s.ptr[src];
    // Rewrite the quoted field in place, collapsing doubled quotes to one char
    if (c == '"' && (src + 1) < end && s.ptr[src + 1] == '"') {
      out[dst++] = '"';
      src += 2;
    } else {
      out[dst++] = c;
      src++;
    }
  }

  return cpc_res_ok(cpc_val_slice((CpcSlice){.ptr = out, .len = dst}), rest);
}

CPC_TAKE_QUOTED_BY(quoted, '"')
CPC_MAP(quotedField, quoted, unescape_quoted)
CPC_TAKE_TILL_ONE_OF(unquotedField, ",\r\n")
CPC_ALT(field, quotedField, unquotedField)
CPC_STRING(p_comma, ",")
CPC_SEP_BY_1(record, field, p_comma)
CPC_ALT(lineEnd, CPC_END_OF_LINE_, CPC_EOF_)
CPC_LEFT(parse_csv_row, record, lineEnd)
