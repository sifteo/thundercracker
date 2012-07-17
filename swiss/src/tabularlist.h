#ifndef TABULARLIST_H
#define TABULARLIST_H

#include <string>
#include <sstream>
#include <vector>


class TabularList
{
public:
    TabularList();
    
    enum Flags {
        NONE = 0,
        RIGHT = (1 << 0),
    };

    std::ostream &cell(Flags flags=NONE);

    void endRow();
    void end();

private:
    Flags cellFlags;
    typedef std::vector< std::pair< std::string, Flags> > RowType;
    std::ostringstream cellStream;
    std::vector<RowType> table;
};


#endif // TABULARLIST_H
