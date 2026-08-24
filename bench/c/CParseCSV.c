// This tries to imitate bench/haskell/ParseCSV.hs, but the parser is row-based
// instead of file-based as it's simpler to allocate memory this way.
#include "csv.h"

CPC_STRING(p_lf, "\n")
CPC_STRING(p_crlf, "\r\n")
CPC_STRING(p_cr, "\r")
CPC_STRING(p_quote, "\"")
CPC_STRING(p_doublequote, "\"\"")
CPC_STRING(p_comma, ",")

CPC_ALT(p_newline_, p_lf, p_crlf)
CPC_ALT(p_newline, p_newline_, p_cr)
CPC_ALT(lineEnd_, p_newline, CPC_EOF_)
CPC_LABEL(lineEnd, lineEnd_, "expected newline or end of input")

static inline bool is_unquoted_field(char c) {
  return c != ',' && c != '\n' && c != '\r' && c != '"';
}
CPC_TAKE_WHILE(unquotedField, is_unquoted_field)

static inline bool is_dquote(char c) {
  return c != '"';
}

CPC_TAKE_WHILE_1(p_til_dquote, is_dquote)
// These are the equivalent of `string "\"\"" >> return "\""`
CPC_PURE(pure_dquote, cpc_val_slice(cpc_slice_from_cstr("\"")))
CPC_RIGHT(p_to_singlequote, p_doublequote, pure_dquote)
CPC_ALT(insideQuotesPrime, p_til_dquote, p_to_singlequote)

// TODO Find a better way to do equivalent of `T.concat <$> many insideQuotes`
CPC_DEFINE_PARSER(insideQuotes) {
  char    *out = (char *)input.ptr;
  size_t   dst = 0;
  CpcSlice cur = input;

  // TODO we duplicate some of the functionality of CPC_MANY(insideQuotesPrime)
  for (;;) {
    const CpcResult piece = CPC_PARSE(insideQuotesPrime, cur, A);
    if (!piece.ok) {
      break;
    }
    if (!cpc_is_slice(&piece.out))
      return cpc_res_err(cur, "insideQuotes_: not a slice", NULL); // TODO should not happen

    // This is the equivalent of `T.concat`
    for (size_t i = 0; i < piece.out.as.slice.len; ++i)
      out[dst++] = piece.out.as.slice.ptr[i];

    cur = piece.rest;
  }

  return cpc_res_ok(cpc_val_slice((CpcSlice){.ptr = out, .len = dst}), cur);
}

// equivalent of `char '"' *> insideQuotes <* char '"'`
CPC_BETWEEN(quotedField, p_quote, insideQuotes, p_quote)

CPC_ALT(field_, quotedField, unquotedField)
CPC_LABEL(field, field_, "field")

CPC_SEP_BY_1(record_, field, p_comma)
CPC_LABEL(record, record_, "record")

CPC_LEFT(csvRow, record, lineEnd)
