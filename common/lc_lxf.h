#pragma once

#include "lc_array.h"

bool lcImportLDDFile(const QString& FileData, lcArray<lcPiece*>& Pieces, lcArray<lcArray<lcPiece*>>& Groups);

