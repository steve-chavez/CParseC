#ifndef CSV_H
#define CSV_H
#include "cparsec.h"

extern CpcResult parse_csv_row(CpcSlice input, CpcArena *A, const char *err);

#endif /* CSV_H */
