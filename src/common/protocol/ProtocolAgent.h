/*
 * Copyright (c) 2014-2015 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */
/**
 * @file        src/common/protocol/ProtocolAgent.h
 * @author      Adam Malinowski <a.malinowsk2@partner.samsung.com>
 * @version     1.0
 * @brief       This file defines protocol class for communication with agent
 */

#ifndef SRC_COMMON_PROTOCOL_PROTOCOLAGENT_H_
#define SRC_COMMON_PROTOCOL_PROTOCOLAGENT_H_

#include <protocol/ProtocolFrameHeader.h>
#include <request/pointers.h>
#include <response/pointers.h>

#include "Protocol.h"

namespace Cynara {

class ProtocolAgent : public Protocol {
public:
    ProtocolAgent();
    virtual ~ProtocolAgent();

    virtual ProtocolPtr clone(void);

    virtual RequestPtr extractRequestFromBuffer(BinaryQueuePtr bufferQueue);
    virtual ResponsePtr extractResponseFromBuffer(BinaryQueuePtr bufferQueue);

    using Protocol::execute;

    virtual void execute(const RequestContext &context, const AgentActionRequest &request);
    virtual void execute(const RequestContext &context, const AgentActionResponse &request);
    virtual void execute(const RequestContext &context, const AgentRegisterRequest &request);
    virtual void execute(const RequestContext &context, const AgentRegisterResponse &request);

private:
    RequestPtr deserializeActionRequest(void);
    RequestPtr deserializeRegisterRequest(void);
    ResponsePtr deserializeActionResponse(void);
    ResponsePtr deserializeRegisterResponse(void);
};

} // namespace Cynara

#endif /* SRC_COMMON_PROTOCOL_PROTOCOLAGENT_H_ */
