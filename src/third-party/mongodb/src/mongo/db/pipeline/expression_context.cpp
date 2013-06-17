/**
 * Copyright (c) 2011 10gen Inc.
 *
 * This program is free software: you can redistribute it and/or  modify
 * it under the terms of the GNU Affero General Public License, version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "pch.h"

#include "db/interrupt_status.h"
#include "db/pipeline/expression_context.h"

namespace mongo {

    ExpressionContext::~ExpressionContext() {
    }

    inline ExpressionContext::ExpressionContext(InterruptStatus *pS):
        doingMerge(false),
        inShard(false),
        inRouter(false),
        intCheckCounter(1),
        pStatus(pS) {
    }

    void ExpressionContext::checkForInterrupt() {
        /*
          Only really check periodically; the check gets a mutex, and could
          be expensive, at least in relative terms.
        */
        if ((++intCheckCounter % 128) == 0) {
            pStatus->checkForInterrupt();
        }
    }

    ExpressionContext* ExpressionContext::clone() {
        ExpressionContext* newContext = create(pStatus);
        newContext->setDoingMerge(getDoingMerge());
        newContext->setInShard(getInShard());
        newContext->setInRouter(getInRouter());
        return newContext;
    }

    ExpressionContext *ExpressionContext::create(InterruptStatus *pStatus) {
        return new ExpressionContext(pStatus);
    }

}
