#pragma once

#include "lc_array.h"

bool lcImportLXFMLFile(const QString& FileData, lcArray<lcPiece*>& Pieces, lcArray<lcArray<lcPiece*>>& Groups);

