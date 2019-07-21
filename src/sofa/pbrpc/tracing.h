#pragma once

#include <yaml-cpp/yaml.h>
#include <jaegertracing/Tracer.h>
#include <jaegertracing/context/context.h>
#include <jaegertracing/context/with_context.h>

namespace sofa {
namespace pbrpc {

class Tracing
{
public:
    static void setTracer(const std::shared_ptr<opentracing::Tracer>& tracer)
    {
        _tracer = tracer;
    }
    static std::unique_ptr<opentracing::Span> startOutboundSpan(const std::string& operation_name)
    {
        if (_tracer)
        {
            // 取出当前线程的span，如果有的话，那就作为新创建的parent，否则当前span就会是一个新的root span
            jaegertracing::Context current = jaegertracing::Context::Current();
            jaegertracing::SpanContext spancontext = current.GetSpanContext();
            opentracing::StartSpanOptions options;
            if (spancontext.isValid())
            {
                options.references.emplace_back(opentracing::SpanReferenceType::ChildOfRef, &spancontext);
            }
            options.tags.emplace_back("span.kind", "client");
            
            // All the bells and whistles:
            auto span = _tracer->StartSpanWithOptions(operation_name, options);
            
            return span;
        }

        return std::unique_ptr<opentracing::Span>();
    }

    static std::unique_ptr<opentracing::Span> startInboundSpan(const std::string& operation_name, const jaegertracing::SpanContext& spancontext)
    {
        if (_tracer)
        {
            // 服务端的parent需要从请求中获取，如果有的话，那就作为新创建的parent，否则当前span就会是一个新的root span
            opentracing::StartSpanOptions options;
            if (spancontext.isValid())
            {
                options.references.emplace_back(opentracing::SpanReferenceType::ChildOfRef, &spancontext);
            }
            options.tags.emplace_back("span.kind", "server");
            
            // All the bells and whistles:
            auto span = _tracer->StartSpanWithOptions(operation_name, options);
            // 服务端创建span，应该放到当前线程上下文中
            return span;
        }

        return std::unique_ptr<opentracing::Span>();
    }

private:
    static std::shared_ptr<opentracing::Tracer> _tracer;
};

}
}
