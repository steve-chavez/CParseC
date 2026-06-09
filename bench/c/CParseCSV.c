// This tries to imitate bench/haskell/ParseCSV.hs, but the parser is row-based
// instead of file-based as it's simpler to allocate memory this way.
#include "csv.h"

CPC_STRING(p_lf, "\n")
CPC_STRING(p_crlf, "\r\n")
CPC_STRING(p_cr, "\r")
CPC_ALT(p_newline_, p_lf, p_cr)
CPC_ALT(p_newline, p_crlf, p_newline_)
CPC_ALT(lineEnd, p_newline, CPC_EOF_)

static inline bool is_unquoted_field(char c) {
  return c != ',' && c != '\n' && c != '\r' && c != '"';
}

CPC_TAKE_WHILE(unquotedField, is_unquoted_field)

static inline bool is_dquote(char c) {
  return c != '"';
}

CPC_TAKE_WHILE_1(p_til_dquote, is_dquote)
// These are the equivalent of `string "\"\"" >> return "\""`
CPC_STRING(p_ddquote, "\"\"")
CPC_PURE(pure_dquote, cpc_val_slice(cpc_slice_from_cstr("\"")))
CPC_RIGHT(p_to_singlequote, p_ddquote, pure_dquote)
CPC_ALT(insideQuotesPrime, p_til_dquote, p_to_singlequote)

// TODO Find a better way to do equivalent of `T.concat <$> many insideQuotes`
CPC_DEFINE_PARSER(insideQuotes) {
  char    *out = (char *)input.ptr;
  size_t   dst = 0;
  CpcSlice cur = input;

  // TODO we duplicate some of the functionality of CPC_MANY(insideQuotesPrime)
  for (;;) {
    const CpcResult piece = insideQuotesPrime(A, cur);
    if (!piece.ok) {
      break;
    }
    if (!cpc_is_slice(&piece.out))
      return cpc_res_err(cur, "insideQuotes_: not a slice"); // TODO should not happen

    // This is the equivalent of `T.concat`
    for (size_t i = 0; i < piece.out.as.slice.len; ++i)
      out[dst++] = piece.out.as.slice.ptr[i];

    cur = piece.rest;
  }

  return cpc_res_ok(cpc_val_slice((CpcSlice){.ptr = out, .len = dst}), cur);
}

// equivalent of `char '"' *> insideQuotes <* char '"'`
CPC_STRING(p_dquote, "\"")
CPC_BETWEEN(quotedField, p_dquote, insideQuotes, p_dquote)

CPC_ALT(field, quotedField, unquotedField)

CPC_STRING(p_comma, ",")
CPC_SEP_BY_1(record, field, p_comma)

CPC_LEFT(parse_csv_row, record, lineEnd)
