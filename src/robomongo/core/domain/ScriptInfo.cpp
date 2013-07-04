#include "ScriptInfo.h"

namespace Robomongo
{
 ScriptInfo::ScriptInfo(const QString &script, bool execute,const CursorPosition &position,const QString &title) :
    _execute(execute),
    _script(script),
    _title(title),
    _cursor(position) {}
}
