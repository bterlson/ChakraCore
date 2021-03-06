//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "Runtime.h"
#include "JsrtContext.h"
#include "JsrtContextCore.h"

JsrtContext *JsrtContext::New(JsrtRuntime * runtime)
{
    return JsrtContextCore::New(runtime);
}

/* static */
bool JsrtContext::Is(void * ref)
{
    return VirtualTableInfo<JsrtContextCore>::HasVirtualTable(ref);
}

void JsrtContext::OnScriptLoad(Js::JavascriptFunction * scriptFunction, Js::Utf8SourceInfo* utf8SourceInfo, CompileScriptException* compileException)
{
    ((JsrtContextCore *)this)->OnScriptLoad(scriptFunction, utf8SourceInfo, compileException);
}

JsrtContextCore::JsrtContextCore(JsrtRuntime * runtime) :
    JsrtContext(runtime)
{
    EnsureScriptContext();
    Link();
    PinCurrentJsrtContext();
}

/* static */
JsrtContextCore *JsrtContextCore::New(JsrtRuntime * runtime)
{
    return RecyclerNewFinalizedLeaf(runtime->GetThreadContext()->EnsureRecycler(), JsrtContextCore, runtime);
}

void JsrtContextCore::Dispose(bool isShutdown)
{
    if (nullptr != this->GetScriptContext())
    {
        if (this->GetRuntime()->GetJsrtDebugManager() != nullptr)
        {
            this->GetRuntime()->GetJsrtDebugManager()->ClearDebugDocument(this->GetScriptContext());
        }
        this->GetScriptContext()->EnsureClearDebugDocument();
        this->GetScriptContext()->GetDebugContext()->GetProbeContainer()->UninstallInlineBreakpointProbe(NULL);
        this->GetScriptContext()->GetDebugContext()->GetProbeContainer()->UninstallDebuggerScriptOptionCallback();
        this->GetScriptContext()->MarkForClose();
        this->SetScriptContext(nullptr);
        Unlink();
    }
}

Js::ScriptContext* JsrtContextCore::EnsureScriptContext()
{
    Assert(this->GetScriptContext() == nullptr);

    ThreadContext* localThreadContext = this->GetRuntime()->GetThreadContext();

    AutoPtr<Js::ScriptContext> newScriptContext(Js::ScriptContext::New(localThreadContext));

    newScriptContext->Initialize();

    hostContext = HeapNew(ChakraCoreHostScriptContext, newScriptContext);
    newScriptContext->SetHostScriptContext(hostContext);

    this->SetScriptContext(newScriptContext.Detach());

    Js::JavascriptLibrary *library = this->GetScriptContext()->GetLibrary();
    Assert(library != nullptr);

    library->GetEvalFunctionObject()->SetEntryPoint(&Js::GlobalObject::EntryEval);
    library->GetFunctionConstructor()->SetEntryPoint(&Js::JavascriptFunction::NewInstance);

    return this->GetScriptContext();
}

void JsrtContextCore::OnScriptLoad(Js::JavascriptFunction * scriptFunction, Js::Utf8SourceInfo* utf8SourceInfo, CompileScriptException* compileException)
{
    JsrtDebugManager* jsrtDebugManager = this->GetRuntime()->GetJsrtDebugManager();
    if (jsrtDebugManager != nullptr)
    {
        jsrtDebugManager->ReportScriptCompile(scriptFunction, utf8SourceInfo, compileException);
    }
}
