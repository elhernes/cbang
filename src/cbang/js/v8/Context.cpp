/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

                Copyright (c) 2003-2019, Cauldron Development LLC
                   Copyright (c) 2003-2017, Stanford University
                               All rights reserved.

         The C! library is free software: you can redistribute it and/or
        modify it under the terms of the GNU Lesser General Public License
       as published by the Free Software Foundation, either version 2.1 of
               the License, or (at your option) any later version.

        The C! library is distributed in the hope that it will be useful,
          but WITHOUT ANY WARRANTY; without even the implied warranty of
        MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
                 Lesser General Public License for more details.

         You should have received a copy of the GNU Lesser General Public
                 License along with the C! library.  If not, see
                         <http://www.gnu.org/licenses/>.

        In addition, BSD licensing may be granted on a case by case basis
        by written permission from at least one of the copyright holders.
           You may request written permission by emailing the authors.

                  For information regarding this software email:
                                 Joseph Coffland
                          joseph@cauldrondevelopment.com

\******************************************************************************/

#include "Context.h"

#include <cbang/js/JSInterrupted.h>

using namespace cb;
using namespace cb::gv8;
using namespace std;


Context::Context(v8::Isolate *iso) : context(v8::Context::New(iso)) {}


Value Context::eval(const InputSource &src) {
  v8::EscapableHandleScope handleScope(Value::getIso());
  Context::Scope ctxScope(*this);

  // Get script origin
  v8::Local<v8::String> _origin;
  string filename = src.getName();
  if (!filename.empty()) _origin = Value::createString(filename);

  // Get script source
  v8::Local<v8::String> source = Value::createString(src.toString());
  v8::ScriptOrigin origin(_origin);
  // Compile
  v8::TryCatch tryCatch(Value::getIso());
  v8::Handle<v8::Script> script = v8::Script::Compile(context, source, &origin).ToLocalChecked();
  if (tryCatch.HasCaught()) translateException(tryCatch, false);

  // Execute
  v8::Handle<v8::Value> ret = script->Run(context).ToLocalChecked();
  if (tryCatch.HasCaught()) translateException(tryCatch, true);

  // Return result
  return handleScope.Escape(ret);
}


void Context::translateException(const v8::TryCatch &tryCatch, bool useStack) {
  v8::HandleScope handleScope(Value::getIso());

  if (useStack && !tryCatch.StackTrace(Value::getCtx()).IsEmpty())
    throw Exception(Value(tryCatch.StackTrace(Value::getCtx())).toString());

  if (tryCatch.Exception()->IsNull()) throw cb::js::JSInterrupted();

  string msg = Value(tryCatch.Exception()).toString();

  v8::Handle<v8::Message> message = tryCatch.Message();
  if (message.IsEmpty()) throw Exception(msg);

  string filename = Value(message->GetScriptResourceName()).toString();
  int line = message->GetLineNumber(Value::getCtx()).ToChecked();
  int col = message->GetStartColumn();

  throw Exception(msg, FileLocation(filename, line, col));
}
