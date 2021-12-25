#include "Entities.hpp"


//  IDs only increase.  We'll never re-use an ID.
//  The first ID is 1 so that 0 can have a special meaning.

static int next_id = 1;

int generate_entity_id()
{
    return next_id++;
}
