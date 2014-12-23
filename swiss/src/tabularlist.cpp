/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * swiss - your Sifteo utility knife
 *
 * Copyright <c> 2012 Sifteo, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "tabularlist.h"
#include <stdio.h>
#include <ctype.h>
#include <algorithm>


TabularList::TabularList()
{
    // Begin first row
    table.push_back(RowType());
}

std::ostream &TabularList::cell(Flags flags)
{
    if (!cellStream.str().empty())
        table.back().push_back(std::make_pair(cellStream.str(), cellFlags));

    cellFlags = flags;
    cellStream.str("");
    cellStream.flags(std::ios::fmtflags(0));
    cellStream.width(0);
    cellStream.precision(0);

    return cellStream;
}

void TabularList::endRow()
{
    // Flush last cell
    cell();

    // Begin next row
    if (!table.back().empty())
        table.push_back(RowType());
}

void TabularList::end()
{
    // Ignore trailing empty row
    if (table.back().empty())
        table.pop_back();

    // Calculate width of each column
    std::vector<unsigned> widths;
    for (unsigned i = 0; i < table.size(); ++i) {
        for (unsigned j = 0; j < table[i].size(); ++j) {
            while (widths.size() <= j)
                widths.push_back(0);
            widths[j] = std::max<unsigned>(widths[j], table[i][j].first.length());
        }
    }

    // Output the table
    for (unsigned i = 0; i < table.size(); ++i) {
        for (unsigned j = 0; j < table[i].size(); ++j) {
            const char *cell = table[i][j].first.c_str();
            Flags flags = table[i][j].second;
            int w = widths[j];

            if ((flags & RIGHT) == 0)
                w = -w;

            printf("%s%*s", j ? " " : "", w, cell);
        }
        printf("\n");
    }
}
