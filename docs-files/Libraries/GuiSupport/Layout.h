/*
    Layout.h

    Copyright © 2020 Occipital, Inc. All rights reserved.
    This file is part of the Structure SDK.
    Unauthorized copying of this file, via any medium is strictly prohibited.
    Proprietary and confidential.

    http://structure.io
*/

#pragma once

namespace GuiSupport {
    void insertBlankLine();
    void layoutCurrentWindowAsFullCanvas();

    /** A layout that has a central grid of equally-sized windows and a left
        sidebar containing vertically stacked utility windows. */
    struct GridConfig {
        unsigned numCellsX = 1;
        unsigned numCellsY = 1;
        unsigned numTools = 1;
        unsigned toolAreaWidth = 300;
    };
    /** Position and size the current top-level window (ImGui::Begin) as the
        grid window designated by the given 0-based indices. The cell may
        optionally span multiple cells in the X and Y direction. */
    void layoutCurrentWindowAsGridCell(const GridConfig& cfg, unsigned cellX, unsigned cellY, unsigned spanX = 1u, unsigned spanY = 1u);
    /** Position and size the current top-level window (ImGui::Begin) as the
        utility window designated by the given 0-based index. */
    void layoutCurrentWindowAsGridTool(const GridConfig& cfg, unsigned toolN);
};
